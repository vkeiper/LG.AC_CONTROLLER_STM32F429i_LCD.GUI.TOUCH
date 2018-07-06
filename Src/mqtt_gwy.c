/**
  ******************************************************************************
  * @file    Src/mqtt_gwy.c 
  * @author  Vince Keiper
  * @version V1.0.0
  * @date    05-July-2018 
  * @brief   To receive commands from MQTT this app parses UART data from an MQTT
  *          format and inserts into the HVAC controller data structures. 
	*          To report status to the outside world we periodically format status
	*          data into MQTT frames and transmit over the UART.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
//#include "mqtt_gwy.h"
#include "usart.h"
#include <string.h>

/** @addtogroup VGK_Apps
  * @{
  */

/** @addtogroup APP
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void MQTT_Init(void);
void MQTT_Receive(uint8_t *pBuff,uint16_t len);
void MQTT_Transmit(uint8_t *pBuff,uint16_t len);
/* Private functions ---------------------------------------------------------*/





#define INITMQTT 0u
#define RXMQTT 1u
#define TXMQTT 2u
#define ERRMQTT 3u


//typedef struct modrc5_frames_t{
//   char title[32];
//   const uint8_t adr1;
//   const uint8_t adr2;
//   const uint8_t cmd;
//   const uint8_t chk;
//   const uint16_t sofOn;
//   const uint16_t sofOff;
//   const uint16_t eofOn;
//   const uint8_t  onmltp;/*A "1" off time is the pulse time multiplied by 3*/    
//} ModRc5Frame_t;

//ModRc5Frame_t FramePwr = {
//    "Power-ON/OFF",
//    0x81,
//    0x66,
//    0x81,/*Power ON\OFF, each tx rotates on\off state*/
//    (uint8_t)~0x81,
//    9000u,/*9mS 9000uS*/
//    4500u,/*4.5mS 4500uS*/
//    564u,/*pulse length*/
//    3u/*on pulse multiplier*/
//}; 

///*Init an array of pointers to all the frame objects */
//ModRc5Frame_t *pFrames[] = {
//    &FramePwr,
//    &FrameMode,
//    &FrameTempUp,
//    &FrameTempDn,
//    &FrameFanSpd,
//    &FrameTimer
//};    



#define FRMPWR   0u
#define FRMMODE  1u
#define FRMTMPUP 2u
#define FRMTMPDN 3u
#define FRMFAN   4u

#define ONPERIODCNT 42u // ONPERIOD / TMRPRD  564uS/13uS
#define ONPERIOD 564u
#define IRFRAMEBYTES 32u


static uint8_t lastState = INITMQTT;
static uint8_t TxState = INITMQTT;
static uint8_t FrmIdx =0;
  	

/**
  * @brief  Run uart to Mqtt gateway 
  * @param  None
  * @retval None
  */
void Do_MqttGwy(void)
{ 
  static uint8_t ADR1, ADR2,CMDID, CHK;
  static uint16_t bitsSent=0;
  static uint8_t offPdMult=1;/* '0' lo = 1 period '1'hi = 3 periods*/ 
  static uint32_t DataToTx = 0u;  
    
  
  switch(TxState){

      case INITMQTT:
          
					
			    /* Start in idle state so APP controls when frame gets sent*/
          TxState = RXMQTT;
			
      
      case RXMQTT:
       
				
    
      break;
      
      case TXMQTT:
				

      break;
      
      
      case ERRMQTT:
         			
        
          
      break;
      
     

      default:
      break;
      
  }      
    
  
  
}



/**
  * @brief  Configures IRLED GPIO.
  *
  */
void MQTT_Init(void)
{
  
}

/**
  * @brief  Receives data from UART5 using ISR
  *    
  */
void MQTT_Receive(uint8_t *pBuff,uint16_t len)
{
		HAL_UART_Receive_IT(&huart5,pBuff,len);
}

/**
  * @brief  Transmits Data over UART using interupt
  *    
  */
void MQQT_Transmit(uint8_t *pBuff,uint16_t len)
{
		HAL_UART_Transmit_IT(&huart5,pBuff,len);
}
/**
  * @}
  */ 

/**
  * @}
  */ 
  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
