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

#define QTYCMDS 5u
#define MAXCHARS 32u
#define MAXBUFFLEN 128u

#define sCMDDELIM " "
#define sVALDELIM "\n"
#define TXINTERVALMS 1000u

#define STRMATCHED 0u

/*
 * Comment out define below to use with ESP32 MQTT to UART GWY, it does not expect a CR
 */
//#define WINDOWSTERMMODE
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
	uint16_t uiBufferRollerCnt;
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

uint8_t RxBuffer[MAXBUFFLEN],CmdStringBuffer[MAXBUFFLEN/2],rxtmp[MAXBUFFLEN];

uint8_t *pRxBuffer = &RxBuffer[0];
uint8_t *pRxBufferEnd = &RxBuffer[0]+ (MAXBUFFLEN-1);

uint16_t uiMbRxdCmdCnt=0;
uint8_t GeneralCmdSyntax[QTYCMDS][MAXBUFFLEN/2];
void (*FuncCmdArray[QTYCMDS])(uint8_t *cmd);
static uint16_t uiCmdCount;
static uint8_t sPayload[MAXBUFFLEN/2];

static uint8_t *pRxdCmds[10];
static uint8_t RxdCmdLen[10];

static uint8_t CmdIdx=0;
/* Private function prototypes -----------------------------------------------*/
static void LoadSyntax(void);
static uint8_t  GetPayload(uint8_t *pBuff);
static void SetCmdString(uint8_t *pCmdStr,uint8_t *pUartRxBuffer ,uint8_t *pThisCmd, uint8_t len);
static void ParseMqttText(uint8_t *pBuff,uint16_t len);
static void CommandFunctionSetPwrState(uint8_t *pBuff);
static void CommandFunctionSetTempDmd(uint8_t *pBuff);
static void CommandFunctionSetCtlMode(uint8_t *pBuff);
static void CommandFunctionSetOpMode(uint8_t *pBuff);
static void CommandFunctionSetTempDmd(uint8_t *pBuff);
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
  uint8_t retval = 0x00;
//	uint32_t t_RxIeOff=0;
  int i=0;  
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
			LoadSyntax();
			memset(pRxdCmds,0u,sizeof(pRxdCmds));
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
						i=0;
						while( i <10){
								if(pRxdCmds[i] != 0u){
										/* rx pbuff to MODBUS processor */
										ParseMqttText(pRxdCmds[i], RxdCmdLen[CmdIdx-1]);
										pRxdCmds[i] = 0u;
								}
								i++;
						}
						CmdIdx=0;//we've executed all the cmds in the buffer so reset
						/* Transmit status msg's in round robin */
						TransmitStatus();
						/*Go Back to RX mode*/
						uaState = UAWAIT4RX;
								
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
			static uint16_t uiLastBufferRollerCnt=0;
			uint8_t lenSecondChunk,lenFirstChunk;
			uint8_t *pFirst,*pSecond;
			
							
       
			/* Only get here if 1. In UAWAIT4RX state
				 2. Gap between char <1.5 char time
			*/
			/* Data received will be stored in buffer via the pointer passed in*/
		 /*Store next char in next location*/
			if(pRxBuffer > pRxBufferEnd){
					/* Reset  pointer to beginning of buffer*/
					pRxBuffer = &RxBuffer[0];
					*pRxBuffer = rxtmp[0];
					uaCtl_t.RxByteCnt++;
					uaCtl_t.uiBufferRollerCnt++;
					
			}else{
					/*Store uart buff to modbus rxbuffer at location pointed to, also inc pointer for next time*/
					*pRxBuffer = rxtmp[0];
					uaCtl_t.RxByteCnt++;
					
			}
			
			/* look for cmd terminator */
			if(rxtmp[0] == '\n'){
						if(CmdIdx >= 10){
								CmdIdx=0;
						}
						RxdCmdLen[CmdIdx] = uaCtl_t.RxByteCnt;
						/* if we rolled over we have to calculate the cmd start address in the buffer taking the rollover into account */
						if(uiLastBufferRollerCnt == uaCtl_t.uiBufferRollerCnt){
							
								pRxdCmds[CmdIdx++] = ((pRxBuffer+1) - (uaCtl_t.RxByteCnt));
						}else{
								//subtract the current ptr to start of buffer to get qty bytes of the last chunk of the string
								lenSecondChunk = (pRxBuffer+1) - &RxBuffer[0];
								//subtract length of 2nd chunk from total length of cmd string
								lenFirstChunk = uaCtl_t.RxByteCnt - lenSecondChunk;
							
								//calculate 1st chunk start address, subtract the length 1St chunk from the RxBuffer end address
								pFirst = pRxBufferEnd+1 - lenFirstChunk;
								/* Set Start Address  */
								pRxdCmds[CmdIdx++] = pFirst;
							
								uiLastBufferRollerCnt = uaCtl_t.uiBufferRollerCnt;
						}
						uaCtl_t.RxByteCnt=0;
			}
			
			/* Increase buffer pointer */
			pRxBuffer++;
			
			
	
    /*## Re-Start UART reception IT process ##*/  
    /* Any data received will be stored buffer via the pointer passed in : the number max of 
     data received is 1 char */
    if(HAL_UART_Receive_IT(&huart5, &rxtmp[0], 1) != HAL_OK)
    {
        /* Transfer error in reception process */
        uaCtl_t.bRxErr =1;     
    }
    

		
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
		if(strncmp((char*)pBuff,"ON",strlen("ON"))==STRMATCHED){
				ctldata_s.dmdmode_e = EDMDMD_COOL;
		}else{
				ctldata_s.dmdmode_e = EDMDMD_NONE;
		}
}


static void CommandFunctionSetTempDmd(uint8_t *pBuff)
{
		ctldata_s.acCoolTemps.dmd = atof((char*)pBuff);
}


static void CommandFunctionSetCtlMode(uint8_t *pBuff)
{
		if(strncmp((char*)pBuff,"OFF",strlen("OFF"))== STRMATCHED){
				ctldata_s.ctlmode_e = ECTLMD_OFF;
		}else if(strncmp((char*)pBuff,"TSTAT",strlen("TSTAT"))==STRMATCHED){
				ctldata_s.ctlmode_e = ECTLMD_TSTAT;
		}else if(strncmp((char*)pBuff,"REMOTE",strlen("REMOTE"))==STRMATCHED){
				ctldata_s.ctlmode_e = ECTLMD_REMOTE;
		}else if(strncmp((char*)pBuff,"MANUAL",strlen("MANUAL"))==STRMATCHED){
				ctldata_s.ctlmode_e = ECTLMD_MANUAL;
		}else{
				ctldata_s.ctlmode_e = ECTLMD_OFF;
		}
}


static void CommandFunctionSetOpMode(uint8_t *pBuff)
{
		if(strncmp((char*)pBuff,"OFF",strlen("OFF"))==STRMATCHED){
				ctldata_s.dmdmode_e = EDMDMD_NONE;
		}else if(strncmp((char*)pBuff,"COOL",strlen("COOL"))==STRMATCHED){
				ctldata_s.dmdmode_e = EDMDMD_COOL;
		}else if(strncmp((char*)pBuff,"HEAT",strlen("HEAT"))==STRMATCHED){
				ctldata_s.dmdmode_e = EDMDMD_HEAT;
		}else{
				ctldata_s.dmdmode_e = EDMDMD_NONE;
		}
}


static void CommandFunctionSetCommsRdy(uint8_t *pBuff)
{
		ctldata_s.bWifiReady = TRUE;
}

/******************************************************************************/
/*                         LoadSyntax  					 		  												*/
/******************************************************************************/
static void LoadSyntax(void)
{
	int i=0;
	strcpy((char*)GeneralCmdSyntax[i]		, "/TOHVAC/SET/PWR");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetPwrState;

	strcpy((char*)GeneralCmdSyntax[i]		, "/TOHVAC/SET/TEMP/DMD");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetTempDmd;

	strcpy((char*)GeneralCmdSyntax[i]		, "/TOHVAC/SET/CTLMODE");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetCtlMode;
 
	strcpy((char*)GeneralCmdSyntax[i]		, "/TOHVAC/SET/OPMODE");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetOpMode;
	
	strcpy((char*)GeneralCmdSyntax[i]		, "VK3/HVAC1/COMMS/READY");				
	FuncCmdArray[i++] 	=	&CommandFunctionSetCommsRdy;
	
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
  * @brief  The Uart reception buffer is a ring buffer and the commands could roll over the end of the buffer.
  *         This function function extracts the complete command string even acroos that barrier. 
  * @param  *pCmdStr: pointer to command string buffer used to test aganist command syntax
  * @param  *pUartRxBuffer, pointer to the actual uart rxbuffer.
  * @param  *pThisCmd: pointer to the address this command starts at within the uart rx buffer
  * @param  len: Length of the data for this command
  * @note   None
  * @retval None
  */
static void SetCmdString(uint8_t *pCmdStr,uint8_t *pUartRxBuffer ,uint8_t *pThisCmd, uint8_t len)
{
		uint8_t len2=0,len3=0,length=0;
	  uint8_t *pChr;
	
		pChr = (uint8_t*)strchr((char*)pThisCmd,'\n');
		
		length = (pChr+1) - pThisCmd;
		
	/*TODO: starnge bug where len gets cleared, so for work around I do the chat searc looking for \n deliminator*/
		if(len==0){
			len = length;
		}
		
		/* If this cmd crosses the end of the buffer and rolls back to the start */
		if((pThisCmd + length) > pRxBufferEnd){
				/* 1st copy partial string from current pointer to end of buffer */
				len2 = (((pUartRxBuffer) + MAXBUFFLEN) - pThisCmd);
				memcpy(pCmdStr,pThisCmd,len2);
				/* Calc the remaining bytes left */
				len3 = length - len2;
				/* Copy reminaing bytes to command string buffer offset by the qty bytes copied in the 1st copy operation */
				memcpy(pCmdStr+len2,pUartRxBuffer,len3);
		}else{
				memcpy(pCmdStr,pThisCmd,length);
		
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
    int nChars=0;
	
		ConvertToUpper((char*)pBuff);
	
		SetCmdString(&CmdStringBuffer[0],&RxBuffer[0] ,pBuff, len);
    
    //cycle through cmd syntax array looking for syntax match,  match= execute function ptr at that index
    for(i=0;i<uiCmdCount;i++)
    {
				nChars = strlen((char*)GeneralCmdSyntax[i]);
			
				//TODO: Add function to roll past the end of the buffer and wrap back through to the begining.
        if(strncmp((char*)CmdStringBuffer,(char*)GeneralCmdSyntax[i],strlen((char*)GeneralCmdSyntax[i])) == 0)
        {
						/* If payload found set value in data structure */
						if(GetPayload(&CmdStringBuffer[0])){
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
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/TEMP/ROOM %2.0f%s",ctldata_s.acCoolTemps.rdb,uartCMDTERM);
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
							sprintf((char*)&mqttbuff,"/FROMHVAC/GET/TEMP/DMD %2.0f%s",ctldata_s.acCoolTemps.dmd,uartCMDTERM);
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
