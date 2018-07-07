/**
  ******************************************************************************
  * @file    Src/modbusrtu.c 
  * @author  Vince Keiper
  * @version V1.0.0
  * @date    05-July-2018 
  * @brief   Based on my MODBUS RTU driver, I just dumped the crc and called 
	*          Text based parser to search for MQTT topics. Receives commands from 
	*          uart which are MQTT frames. This app parses UART data from an MQTT
  *          format and inserts into the HVAC controller data structures. 
	*          To report status to the outside world we periodically format status
	*          data into MQTT frames and transmit over the UART.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
	
	
	
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include "stm32f4xx_hal.h"
#include "modbusrtu.h"
#include "usart.h"
#include "tim.h"
#include <stdio.h>
#include <stdlib.h>
#include "ctype.h"
#include "hvac_ctl.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define TICKGETU32() HAL_GetTick()
#define RXBUFFERSIZE 64u

#define strTRUE  "TRUE"
#define strFALSE "FALSE"
#define strON    "ON"
#define strOFF   "OFF"

#define QTYCMDS 4u
#define MAXCHARS 32u

#define sCMDDELIM " "
#define sVALDELIM "\n"
#define TXINTERVALMS 100u

/*
 * Comment out define below to use with ESP32 MQTT to UART GWY, it does not expect a CR
 */
#define WINDOWSTERMMODE
#ifdef WINDOWSTERMMODE
	#define uartCMDTERM "\r\n"
#else
	#define uartCMDTERM "\n"
#endif

/* Private variables ---------------------------------------------------------*/
typedef struct{
	uint8_t bTxRqd;
	uint32_t RxByteCnt;
	uint8_t bRxdData;
	uint8_t bErr;
  uint8_t bRxErr;
	uint8_t bTxErr;
	uint8_t bInitErr;
	uint32_t t_LstRxChar;
	uint32_t t_BtwnExe;	
	uint8_t bMbTimeout;
  uint16_t TxLen;
  uint8_t bTxCplt;
  uint8_t bRxOvfl;
  uint8_t RetryCnt;
  uint8_t bEOF;/*EndOfFrame elapsed*/
}t_uartctl;

t_uartctl uaCtl_t;

/*enum for state machine*/
typedef enum{
	UAOFF,
	UAINIT,
	UARUN,
  UAWAIT4RX,
  UARXDDATA,
  UAPROCFRAME,
  UATXDATA,
  UAERR,
}e_UartState;

static e_UartState uaState;

uint8_t RxBuffer[128],TxBuffer[128],rxtmp[128];

uint8_t *pRxBuffer = &RxBuffer[0];
uint8_t *pTxBuffer = &TxBuffer[0];
uint16_t uiMbRxdCmdCnt=0;
uint8_t GeneralCmdSyntax[QTYCMDS][64];
void (*FuncCmdArray[QTYCMDS])(uint8_t *cmd);
static uint16_t uiCmdCount;
static uint8_t sPayload[64];

/* Private function prototypes -----------------------------------------------*/
static void LoadSyntax(void);
static uint8_t  GetPayload(uint8_t *pBuff);
static void ParseMqttText(uint8_t *pBuff,uint16_t len);
static void CommandFunctionSetPwrState(uint8_t *pBuff);
static void CommandFunctionSetTempDmd(uint8_t *pBuff);
static void CommandFunctionSetCtlMode(uint8_t *pBuff);
static void CommandFunctionSetOpMode(uint8_t *pBuff);
static void TransmitStatus(void);
/* Private functions ---------------------------------------------------------*/

static void _MBDumpFrame(void)
{
    /*Clear this to reinit for RX mode*/
    uaCtl_t.t_LstRxChar =0;
    pRxBuffer = &RxBuffer[0];
    memset(pRxBuffer,0,sizeof(RxBuffer));
}
/**
  * @brief  DoModbusRtu handles connection, control, & operation of UART layer
  *         of the modbus rtu server.
  * @param  None
  * @retval None
  */
uint8_t DoUartServer(void)
{
  uint8_t retval = 0x00, bReady4Tx;
	uint32_t t_RxIeOff=0;
    
    /*Main State Machine*/
    switch (uaState){

		case UAOFF:
			uaState = UAINIT;
			  
		break;

		case UAINIT:
			uaCtl_t.bRxErr = 0;
			uaCtl_t.bTxErr = 0;
			uaCtl_t.bInitErr = 0;
			uaCtl_t.bTxRqd = 0;
			uaCtl_t.bRxdData = 0;
      uaCtl_t.bRxOvfl = 0;
      uaCtl_t.bEOF = 0;
			/*Start off with pointer at start of buffer*/
			pRxBuffer = &RxBuffer[0];
			pTxBuffer = &TxBuffer[0];
			LoadSyntax();
			
			MX_UART5_Init();

			
			/*Initialiize the end of frame HW timer based on uart baud & qty bits in char*/
			//MX_TIM4_Init(huart5.Init.BaudRate,10u);
			
			/*## Start UART reception process ##*/  
			/* Any data received will be stored buffer via the pointer passed in : the number max of 
			 data received is 1 char */
			if(HAL_UART_Receive_IT(&huart5, &rxtmp[0], 1) != HAL_OK)
			{
					/* Transfer error in reception process */
					uaCtl_t.bRxErr =1;     
			}
		 
			/*Set state to start receiving data*/
			uaState = UAWAIT4RX;
			
			
		break;

		case UAWAIT4RX:
						
						if(uaCtl_t.bRxdData){
								uaCtl_t.bRxdData =0;
								uaCtl_t.t_LstRxChar = 0;
								uaCtl_t.bEOF = 1;
						}
            /*Wait for 3.5 char time EOF timer*/
            /* 1st bytes must be received in the uart ISR*/
            if(uaCtl_t.bEOF ==1)
            {
                /*Reset pointer to start of rx buff*/
                pRxBuffer = &RxBuffer[0];
                /*Allow UART RX ISR to poppulate buffer*/
                uaCtl_t.bEOF =0;
                uaState = UAPROCFRAME;
								t_RxIeOff=0;//must have just rxd a frame so clear timer
                break;
            }else{
								/* Transmit status msg in round robin */
								TransmitStatus();
							
								/* Trap if RX ISR off */
//								if(t_RxIeOff ==0 && (__HAL_UART_GET_FLAG(&huart5,UART_FLAG_RXNE) == RESET)){
//										t_RxIeOff = TICKGETU32();
//										
//								}
								
								if(TICKGETU32() > t_RxIeOff + 1000){
										
										if(HAL_UART_Receive_IT(&huart5, &rxtmp[0], 1) != HAL_OK)
										{
												t_RxIeOff =0;
											
										}else{
												t_RxIeOff = TICKGETU32();
										
										}
								}
								
						}
           				
		break;	
	  
		case UARXDDATA:
				uaState = UAPROCFRAME;               
		break;
	  
		case UAPROCFRAME:
            
            /* rx pbuff to MODBUS processor */
            ParseMqttText(&RxBuffer[0], uaCtl_t.RxByteCnt);
						/*ensure we dont TX and that rxbuffer is cleared*/
						uaCtl_t.TxLen =0;
		
            /* Only TX response if TX len >0 */
            if(uaCtl_t.TxLen >0)
            {
                /*TODO: Debug Code, until we wire this to the modbus handler*/
                uaCtl_t.bTxRqd =1;
                uaState = UATXDATA;
                
            }else{
                /*Clears RxBuffer, LastCharTime, Resets ptr to rxbuffer[0]*/
                _MBDumpFrame();
                
                /*Go BAck to RX mode*/
                uaState = UAWAIT4RX;
            }
						
						/*## Re-Start UART reception IT process ##*/  
						/* Any data received will be stored buffer via the pointer passed in : the number max of 
						 data received is 1 char */
						if(HAL_UART_Receive_IT(&huart5, &rxtmp[0], 1) != HAL_OK)
						{
								/* Transfer error in reception process */
							 // uaCtl_t.bRxErr =1;     
						}
						
		break;
	  
		case UATXDATA:
			
			/*Wait for MB server to set flag to send data back*/
			/*ps, this flag is cleaared with the TXCMPLT ISR cb*/
			if(uaCtl_t.bTxRqd ==1)
			{
						uaCtl_t.bTxRqd =0;
            /*## Ensure UART is not busy ##*/ 
                bReady4Tx =0;
				do {
							 if(HAL_UART_GetState(&huart5) == HAL_UART_STATE_READY ||													 
									HAL_UART_GetState(&huart5) == HAL_UART_STATE_BUSY_RX)
							 {
									 bReady4Tx =1;
							 }
                       
				}while(bReady4Tx == 0);
				
				/*Start the transmission process */  
				if(HAL_UART_Transmit_IT(&huart5, pTxBuffer, uaCtl_t.TxLen)!= HAL_OK)
				{
						/* Transfer error in transmission process */
						uaCtl_t.bTxErr =1;
				}else{
						uaCtl_t.bTxCplt =0;
        }
				
			}else{
						/*Wait for TX to finish*/
						if(uaCtl_t.bTxCplt ==1)
						{
								/*back to waiting 4 receive state*/
								uaState = UAWAIT4RX;
						}
			}			
		break;
	  
	  
		case UAERR:
			retval = 0xFF;
			if(uaCtl_t.RetryCnt++< 10)
			{
					/*Restart UART and retry*/
					MX_USART5_UART_DeInit();
					uaState = UAINIT;
					
					/*Clear any error that caused the reset*/
					uaCtl_t.bErr = 0;
					uaCtl_t.bRxErr =0;
					uaCtl_t.bTxErr =0;
					uaCtl_t.bInitErr =0;
					uaCtl_t.bRxOvfl =0;
			}
		break;
	  
		default:
			retval = 0xFF;
		break;	  
	};
	
	/*Error Trap*/
	if(uaCtl_t.bErr == 1 || uaCtl_t.bRxErr == 1 || uaCtl_t.bTxErr == 1 ||
		uaCtl_t.bInitErr ==1 || uaCtl_t.bRxOvfl ==1){
			
		uaState = UAERR;
	}
        
    return retval;
}


/**
  * @brief  Tx Transfer completed callback
  * @param  UartHandle: UART handle. 
  * @note   Report end of IT Tx transfer 
  * @retval None
  */
void SER_TmrCpltCallback(void)
{
	/* Reception complete*/
	uaCtl_t.bEOF =1;
}



/**
  * @brief  Tx Transfer completed callback
  * @param  UartHandle: UART handle. 
  * @note   Report end of IT Tx transfer 
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	/* Transmission complete*/
	uaCtl_t.bTxCplt =1;
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   report end of IT Rx transfer
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
       
        /* Only get here if 1. In UAWAIT4RX state
           2. Gap between char <1.5 char time
        */
        /* Data received will be stored in buffer via the pointer passed in*/
       /*Store next char in next location*/
        if(uaCtl_t.RxByteCnt +1 < (sizeof(RxBuffer)+1)){
            /*Store uart buff to modbus rxbuffer at location pointed to, also inc pointer for next time*/
            *pRxBuffer = rxtmp[0];
            pRxBuffer++;
            uaCtl_t.RxByteCnt++;
					  if(rxtmp[0] == '\n'){
									uaCtl_t.bRxdData = 1;
						}else{
							  /* Get more chars */
						    if(HAL_UART_Receive_IT(&huart5, &rxtmp[0], 1) != HAL_OK)
								{
										/* Transfer error in reception process */
									 // uaCtl_t.bRxErr =1;     
								}
						}
        }else{
            /* RxBuffer Overflow error*/
           // uaCtl_t.bRxOvfl =1;      
        }
    //}
    
    /*## Re-Start UART reception IT process ##*/  
    /* Any data received will be stored buffer via the pointer passed in : the number max of 
     data received is 1 char */
//    if(HAL_UART_Receive_IT(&huart5, &rxtmp[0], 1) != HAL_OK)
//    {
//        /* Transfer error in reception process */
//       // uaCtl_t.bRxErr =1;     
//    }
    
    /*Start end of frame timer*/
    //MX_Timer4_StartStop(TMRRUN);
}

/**
  * @brief  UART error callbacks
  * @param  UartHandle: UART handle
  * @note   report transfer error
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
    if(HAL_UART_ERROR_NONE != HAL_UART_GetError(&huart5))
    {
        /* Transfer error in reception/transmission process */
        uaCtl_t.bErr = 1;
    }
}

void SetMbRtuEndOfFrame(void)
{
    /*Since end f frame no more bytes expected*/
    /*Stop end of frame watchdog timer*/
    MX_Timer4_StartStop(TMRSTOP);
    
    if( uaState == UAWAIT4RX)
    {   
        uaCtl_t.bEOF =1;
    }
}

/**
  * @brief  Converts all chars to upper case to ease commparison 
  * @param  *s: pointer to char buffer
  * @param  N\A
  * @note   None
  * @retval None
  */void ConvertToUpper(char *s)
{
	int len,i;

	len = strlen(s);
	for(i=0;i<len;i++)
		s[i] = toupper(s[i]);
}

static void CommandFunctionSetPwrState(uint8_t *pBuff)
{
		if(strncmp((char*)pBuff,"ON",strlen("ON"))){
				ctldata_s.dmdmode_e = EDMDMD_COOL;
		}else{
				ctldata_s.dmdmode_e = EDMDMD_NONE;
		}
}


static void CommandFunctionSetTempDmd(uint8_t *pBuff)
{
		ctldata_s.acCooTemps.dmd = atof((char*)pBuff);
}


static void CommandFunctionSetCtlMode(uint8_t *pBuff)
{
		if(strncmp((char*)pBuff,"OFF",strlen("OFF"))){
				ctldata_s.ctlmode_e = ECTLMD_OFF;
		}else if(strncmp((char*)pBuff,"TSTAT",strlen("TSTAT"))){
				ctldata_s.ctlmode_e = ECTLMD_TSTAT;
		}else if(strncmp((char*)pBuff,"REMOTE",strlen("REMOTE"))){
				ctldata_s.ctlmode_e = ECTLMD_REMOTE;
		}else if(strncmp((char*)pBuff,"MANUAL",strlen("MANUAL"))){
				ctldata_s.ctlmode_e = ECTLMD_MANUAL;
		}else{
				ctldata_s.ctlmode_e = ECTLMD_OFF;
		}
}


static void CommandFunctionSetOpMode(uint8_t *pBuff)
{
		if(strncmp((char*)pBuff,"OFF",strlen("OFF"))){
				ctldata_s.dmdmode_e = EDMDMD_NONE;
		}else if(strncmp((char*)pBuff,"COOL",strlen("COOL"))){
				ctldata_s.dmdmode_e = EDMDMD_COOL;
		}else if(strncmp((char*)pBuff,"HEAT",strlen("HEAT"))){
				ctldata_s.dmdmode_e = EDMDMD_HEAT;
		}else{
				ctldata_s.dmdmode_e = EDMDMD_NONE;
		}
}

/******************************************************************************/
/*                         LoadSyntax  					 		  												*/
/******************************************************************************/
static void LoadSyntax(void)
{
	int i=0;
	strcpy((char*)GeneralCmdSyntax[i]		, "/FROMHVAC/SET/PWR");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetPwrState;

	strcpy((char*)GeneralCmdSyntax[i]		, "/FROMHVAC/SET/TEMP/DMD");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetTempDmd;

	strcpy((char*)GeneralCmdSyntax[i]		, "/FROMHVAC/SET/CTLMODE");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetCtlMode;
 
	strcpy((char*)GeneralCmdSyntax[i]		, "/FROMHVAC/SET/OPMODE");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetOpMode;
	
	uiCmdCount = i;
}

/**
  * @brief Extract the payload from the command buffer by using the 
  *        space 0x20 between the command and the payload data
  * @param *pBuff: pointer to received buffer
  * @return TRUE if payload found FALSE if none found
  */
static uint8_t  GetPayload(uint8_t *pBuff)
{
		uint8_t *ptr,*ptrStart;
    
		// look for ' ' indicating end of command, starting payload
		ptrStart = ptr = (uint8_t*)strtok((char*)pBuff, sCMDDELIM);	
		ptr = (uint8_t*)strtok(NULL, sVALDELIM);	
		
	  sprintf((char*)sPayload,"%s",ptr);//extract payload starting from tokenized cmd 
		
		/* Means no space between cmd & payload, error */
		if(ptr == ptrStart){
					ptr = NULL;
		}
		
		if(ptr != NULL){
					return TRUE;
		}else{
					return FALSE;
		}
}

/**
  * @brief  Parse received frame from UART and set respective HVAC control data 
  * @param  *pBuff: pointer to uint8_t rx buffer
  * @param  len: Length of the data received
  * @note   None
  * @retval None
  */
void ParseMqttText(uint8_t *pBuff,uint16_t len)
{
    int i;
    
		ConvertToUpper((char*)pBuff);
    
    //cycle through cmd syntax array looking for syntax match,  match= execute function ptr at that index
    for(i=0;i<uiCmdCount;i++)
    {
        if(strncmp((char*)pBuff,(char*)GeneralCmdSyntax[i],strlen((char*)GeneralCmdSyntax[i])) == 0)
        {
						/* If payload found set value in data structure */
						if(GetPayload(pBuff)){
									(*FuncCmdArray[i])((uint8_t*)&sPayload);
									uiMbRxdCmdCnt++;
						}
						return;
        }
    }
}

/**
  * @brief  Transmit frame out of UART5 
  * @param  *pBuff: pointer to uint8_t transmit buffer
  * @param  len: Length of the data to send
  * @note   RETVAL should be tested for HAL_OK to be sure the data was sent
  * @retval HAL_StatusTypeDef: HAL_OK, HAL_BUSY,HAL_ERROR,HAL_TIMEOUT 
  */
 HAL_StatusTypeDef AsyncTransmit(uint8_t *pBuff,uint16_t len)
{
		HAL_StatusTypeDef hSta = HAL_ERROR;
		uint8_t bReady4Tx = 0;
		uint32_t tTickStart = TICKGETU32();
	
	
		//test if still sending last message
		if(uaCtl_t.bTxCplt == 0){
			//TODO: Bypass this error until I can debug it
				//hSta = HAL_BUSY;
				//return hSta;
		}
	
		//test to make sure uart is ready
		do {
			 //if not busy TX then we can stuff more in the buffer
			 if(HAL_UART_GetState(&huart5) == HAL_UART_STATE_READY ||													
					HAL_UART_GetState(&huart5) == HAL_UART_STATE_BUSY_RX)
			 {
					 bReady4Tx =1;
			 }else{// if it takes longer to TX than 100mS then something is wrong
				   if(TICKGETU32() > tTickStart + 100){
								/* Transfer error in transmission process */
								uaCtl_t.bTxErr =1;
								hSta = HAL_TIMEOUT;
					 }
			 }										 
		}while(bReady4Tx == 0);
				
		/*Start the transmission process */  
		if(HAL_UART_Transmit_IT(&huart5, pBuff, len)!= HAL_OK)
		{
				/* Transfer error in transmission process */
				uaCtl_t.bTxErr =1;
				hSta = HAL_ERROR;
		}else{
				uaCtl_t.bTxCplt =0;
				hSta = HAL_OK;
		}
		
		return hSta;
}

static void TransmitStatus(void)
{
		static uint32_t t_run=0;
		static uint8_t uMsgIdx=0;
		uint8_t sCtlMode[12];
		static uint8_t mqttbuff[64];
	
		/* Stuff to do every x mS */
		if(TICKGETU32() > t_run + TXINTERVALMS){
				t_run = TICKGETU32();
				if(HAL_UART_GetState(&huart5) == HAL_UART_STATE_READY ||													 
								HAL_UART_GetState(&huart5) == HAL_UART_STATE_BUSY_RX)
				 {
						if(uMsgIdx ==0){
							uMsgIdx++;
							/*Format and transmit MQTT frame for ROOM TEMP */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/TEMP/ROOM %2.0f%s",ctldata_s.condCoil.rdb,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==1){
							uMsgIdx++;
							/*Format and transmit MQTT frame for Condensing Coil TEMP */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/TEMP/COND %2.0f%s",ctldata_s.condCoil.rdb,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==2){
							uMsgIdx++;
							/*Format and transmit MQTT frame for PWR ON state */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/PWR %s%s",
									HAL_GPIO_ReadPin(DI_ACMODELED_GPIO_Port,DI_ACMODELED_Pin) == GPIO_PIN_RESET ? "ON" : "OFF",
									uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==3){
							uMsgIdx++;
							/*Format and transmit MQTT frame for Frost Error state */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/FROSTERR %s%s",ctldata_s.bFrostCheck == TRUE ? "TRUE" : "FALSE",uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==4){
							uMsgIdx++;
							/*Format and transmit MQTT frame for demand temperature */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/TEMP/DMD %2.0f%s",ctldata_s.acCooTemps.dmd,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==5){
							uMsgIdx++;
							if(ctldata_s.ctlmode_e == ECTLMD_OFF){
									memcpy(&sCtlMode,"OFF",strlen("OFF"));
							}else if(ctldata_s.ctlmode_e == ECTLMD_TSTAT){
									memcpy(&sCtlMode,"TSTAT",strlen("TSTAT"));
							}else if(ctldata_s.ctlmode_e == ECTLMD_REMOTE){
									memcpy(&sCtlMode,"REMOTE-SW",strlen("REMOTE-SW"));
							}else if(ctldata_s.ctlmode_e == ECTLMD_MANUAL){
									memcpy(&sCtlMode,"MANUAL-IOPIN",strlen("MANUAL-IOPIN"));
							} 
							/*Format and transmit MQTT frame for Demand mode */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/CTLMODE %s%s",sCtlMode,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==6){
							uMsgIdx++;
							if(ctldata_s.dmdmode_e == EDMDMD_NONE  ){
									memcpy(&sCtlMode,"OFF",strlen("OFF"));
							}else if(ctldata_s.dmdmode_e == EDMDMD_COOL){
									memcpy(&sCtlMode,"COOL",strlen("COOL"));
							}else if(ctldata_s.dmdmode_e == EDMDMD_HEAT){
									memcpy(&sCtlMode,"HEAT",strlen("HEAT"));
							} 
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/DMDMODE %s%s",sCtlMode,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==7){
							uMsgIdx++;
							/*Format and transmit MQTT frame for warm-up period remaining time */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/TIME/WRM %d%s",ctldata_s.ulWarmupSec,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==8){
							uMsgIdx++;
							/*Format and transmit MQTT frame for up-time */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/TIME/RUN %s%s",time_s.str,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}else if(uMsgIdx ==9){
							uMsgIdx++;
							/*Format and transmit MQTT frame for quantity received commandse */
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/CMD/RXD %d%s",uiMbRxdCmdCnt,uartCMDTERM);
							AsyncTransmit(&mqttbuff[0],strlen((char*)&mqttbuff));
						}			
						else{
							uMsgIdx =0;
						}	
				}
		}
}
/************************ (C) COPYRIGHT Astrodyne Tdi *****END OF FILE****/
