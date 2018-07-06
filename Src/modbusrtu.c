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
#include "usart.h"
#include "tim.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define TICKGETU32() HAL_GetTick()
#define RXBUFFERSIZE 64u

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

uint8_t RxBuffer[64],TxBuffer[64],rxtmp[16];
uint8_t *pRxBuffer;
uint8_t *pTxBuffer;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static void ParseMqttText(uint8_t *pBuff,uint16_t len);

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
	  
  uaCtl_t.t_BtwnExe = !uaCtl_t.t_BtwnExe;
    
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
			
			MX_UART5_Init();

			
			/*Initialiize the end of frame HW timer based on uart baud & qty bits in char*/
			MX_TIM4_Init(huart5.Init.BaudRate,10u);
			
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
            /*Wait for 3.5 char time EOF timer*/
            /* 1st bytes must be received in the uart ISR*/
            if(uaCtl_t.bEOF ==1)
            {
                /*Reset pointer to start of rx buff*/
                pRxBuffer = &RxBuffer[0];
                /*Allow UART RX ISR to poppulate buffer*/
                uaCtl_t.bEOF =0;
                uaState = UARXDDATA;
                break;
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
    /* indicate we received a Data */
	uaCtl_t.bRxdData = 1;
	//uaCtl_t.t_LstRxChar = TICKGETU32();
    
    /*Byte Received,  Stop EOF timer*/
    MX_Timer4_StartStop(TMRSTOP);
    
    
    /*
        1. NOT in UAWAIT4RX mode?
        2. Last Frame not processed yet?
        = Dump char & keep rx ptr at 0
    */
    if(uaState != UAWAIT4RX || uaCtl_t.bEOF ==1)
    {
        memset(&rxtmp,0,sizeof(rxtmp));
        /*Reset pointer to start of rx buff*/
        pRxBuffer = &RxBuffer[0];
        uaCtl_t.RxByteCnt =0;
        
        /* do not indicate we received data */
        uaCtl_t.bRxdData = 0;
    } 
    
    /*1. In UAWAIT4RX 
      2. No CHAR timeout
       = Stuff buffer, handle those operations
    */
    else{
    
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
        }else{
            /* RxBuffer Overflow error*/
            uaCtl_t.bRxOvfl =1;      
        }
    }
    
    /*## Re-Start UART reception IT process ##*/  
    /* Any data received will be stored buffer via the pointer passed in : the number max of 
     data received is 1 char */
    if(HAL_UART_Receive_IT(&huart5, &rxtmp[0], 1) != HAL_OK)
    {
        /* Transfer error in reception process */
        uaCtl_t.bRxErr =1;     
    }
    
    /*Start end of frame timer*/
    MX_Timer4_StartStop(TMRRUN);
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

void ParseMqttText(uint8_t *pBuff,uint16_t len)
{
	
	
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
				hSta = HAL_BUSY;
				return hSta;
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
/************************ (C) COPYRIGHT Astrodyne Tdi *****END OF FILE****/
