/**
  ******************************************************************************
  * @file    Src/ir_rmt_txr.c 
  * @author  Vince Keiper
  * @version V1.0.0
  * @date    12-May-2018 
  * @brief   This app transmits commands to an LG AC unit
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
//#include "lcd_log.h"
#include "ir_rmt_txr.h"
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
void IRLED_Init(void);
void IRLED_On(void);
void IRLED_Off(void);
void IRLED_Toggle(void);
/* Private functions ---------------------------------------------------------*/




//typedef enum Ranks {FIRST, SECOND} Order;
//Order data = (Order)20;  // Must cast to defined type to prevent error

#define INITTX 0u
#define SOFTX 1u
#define TXDATA 2u
#define WAITTX 3u
#define EOFTX   4u
#define DONETX 5u
#define IDLESTA 6u

#define CARFQY (38000u)
#define TMRFQY (CARFQY * 2u)
#define TMRPRD  (1/TMRFQY)

#define MSCNT_9 (684) //count of how many cycles in 9mS 1 cycle is 26uS (timer ticks at 13uS)
#define MSCNT_4p5 (342) //count of how many cycles 4.5mS/564uS


char dispstr[512];
uint16_t charcnt=0;

uint32_t tickcnt;
uint8_t pinmask=0;
uint8_t bSync=0;

typedef struct modrc5_frames_t{
   char title[32];
   const uint8_t adr1;
   const uint8_t adr2;
   const uint8_t cmd;
   const uint8_t chk;
   const uint16_t sofOn;
   const uint16_t sofOff;
   const uint16_t eofOn;
   const uint8_t  onmltp;/*A "1" off time is the pulse time multiplied by 3*/    
} ModRc5Frame_t;

ModRc5Frame_t FramePwr = {
    "Power-ON/OFF",
    0x81,
    0x66,
    0x81,/*Power ON\OFF, each tx rotates on\off state*/
    (uint8_t)~0x81,
    9000u,/*9mS 9000uS*/
    4500u,/*4.5mS 4500uS*/
    564u,/*pulse length*/
    3u/*on pulse multiplier*/
}; 
ModRc5Frame_t FrameMode = {
    "Mode +",
    0x81,
    0x66,
    0xD9,/*Mode, each tx rotates mode */
    (uint8_t)~0xD9,
    9000u,/*9mS 9000uS*/
    4500u,/*4.5mS 4500uS*/
    564u,/*pulse length*/
    3u/*on pulse multiplier*/
}; 
ModRc5Frame_t FrameTempUp = {
    "Temp +",
    0x81,
    0x66,
    0xA1,/*Temperature setpoint increase 0b10101110*/  
    (uint8_t)~0xA1,/*0b01010001*/
    9000u,/*9mS 9000uS*/
    4500u,/*4.5mS 4500uS*/
    564u,/*pulse length*/
    3u/*on pulse multiplier*/
}; 
ModRc5Frame_t FrameTempDn = {
    "Temp -",
    0x81,
    0x66,
    0x51,/*Temperature setpoint decrease */
    (uint8_t)~0x51,
    9000u,/*9mS 9000uS*/
    4500u,/*4.5mS 4500uS*/
    564u,/*pulse length*/
    3u
}; 
ModRc5Frame_t FrameFanSpd = {
    "Fan Speed ",
    0x81,
    0x66,
    0x99,/*Fan Speed rotate */
    (uint8_t)~0x99,
    9000u,/*9mS 9000uS*/
    4500u,/*4.5mS 4500uS*/
    564u,/*pulse length*/
    3u/*on pulse multiplier*/
}; 

ModRc5Frame_t FrameTimer = {
    "NOT SET",
    0x81,
    0x66,
    0x00,
    (uint8_t)~0x00,
    9000u,/*9mS 9000uS*/
    4500u,/*4.5mS 4500uS*/
    564u,/*pulse length*/
    3u/*on pulse multiplier*/
}; 

/*Init an array of pointers to all the frame objects */
ModRc5Frame_t *pFrames[] = {
    &FramePwr,
    &FrameMode,
    &FrameTempUp,
    &FrameTempDn,
    &FrameFanSpd,
    &FrameTimer
};    



#define FRMPWR   0u
#define FRMMODE  1u
#define FRMTMPUP 2u
#define FRMTMPDN 3u
#define FRMFAN   4u

#define ONPERIODCNT 42u // ONPERIOD / TMRPRD  564uS/13uS
#define ONPERIOD 564u
#define IRFRAMEBYTES 32u


static uint8_t lastState = INITTX;
static uint8_t TxState = INITTX;
static uint8_t FrmIdx =0;
  	


uint8_t SendFrame(uint8_t fIdx){
	/*  */
	if(TxState == INITTX || TxState == DONETX){
		FrmIdx = fIdx;
		/* Trigger the tier to start */
		if(TxState != INITTX){
			TxState = IDLESTA;
		}
		/*if starting in Idle state the timer gets enabled*/
		Do_Ir_Rmt_Txr();
		return 1u;
	}else{
		return 0u;
	}
	
	
}

/**
  * @brief  Run IR rmt state machine gets called by Timer1 period elapsed callback
  * @param  None
  * @retval None
  */
void Do_Ir_Rmt_Txr(void)
{ 
  static uint8_t ADR1, ADR2,CMDID, CHK;
  static uint16_t bitsSent=0;
  static uint8_t offPdMult=1;/* '0' lo = 1 period '1'hi = 3 periods*/ 
  static uint32_t DataToTx = 0u;  
    
  
  switch(TxState){

      case INITTX:
          
          //sprintf(dispstr,"%4s Ver %d.%d.%d %d/%d/%d %d:%d",AUTHRABR,MAJVER,
          //          MINVER,BLDVER,YERVER,MONVER,DAYVER,HRSVER,MNSVER);
          //LCD_LOG_SetFooter((uint8_t*)dispstr);
          //LCD_UsrLog ("Author %s \n",AUTHRFULL);
			
					IRLED_Init();
      
			    /* Start in idle state so APP controls when frame gets sent*/
          TxState = SOFTX;
			
			    /* sets up timer for 38Khz 50% duty cycle wave */
					tickcnt=0;
				  pinmask = 1u;
          TIM1_Init();
          HAL_TIM_Base_Start_IT(&htim1);
      
      
      case SOFTX:
        //LCD_UsrLog ("TX SOF \n" );
        //LCD_UsrLog ("DATA= 0x%8x \n",DataToTx );
        
      //TODO:/* Setup TMR to send 38Khz carrier for 9mS then pause for 4.5mS */
				if(tickcnt <2){
					IRLED_Off();//set low to start
					
					charcnt=0;
					ADR1 = (uint8_t)pFrames[FrmIdx]->adr1;
					ADR2 = (uint8_t)pFrames[FrmIdx]->adr2;
					CMDID = (uint8_t)pFrames[FrmIdx]->cmd;
					CHK = (uint8_t)pFrames[FrmIdx]->chk;


					/*build data tx frame*/
					DataToTx |= ADR1<<24; 
					DataToTx |= ADR2<<16;
					DataToTx |= CMDID<<8; 
					DataToTx |= CHK;
					//memset(dispstr,0u,sizeof(dispstr));
					//sprintf(dispstr+charcnt,"TX SOF \n DATA= 0x%8x \n",DataToTx );
			    //charcnt += strlen(dispstr);
				}/*start if carrier ON period*/
				else if(tickcnt >1 && (tickcnt <=(MSCNT_9))){
				  IRLED_Toggle();
				}/* End of OFF period*/
				
				/* End of CARRIER ON period*/
				else if((tickcnt >MSCNT_9) && (tickcnt <=(MSCNT_9 + MSCNT_4p5))){
				  IRLED_Off();
				}/* End of OFF period*/
				else{
					//unlock pin control
					TxState = TXDATA;
					tickcnt=0;
				}
				
				
    
      break;
      
      case TXDATA:
				if(tickcnt <2){
						IRLED_Off();//set low to start CARRIER ON period
				}
				
				// Send carrier for 564uS then test bit and if 1 pause 564uS else pause 564uS*3  
			  if(tickcnt <ONPERIODCNT){
						IRLED_Toggle();
				}/*564uS carrier freq ON period over*/	
				else if(tickcnt >=ONPERIODCNT && tickcnt < ONPERIODCNT+1){
						IRLED_Off();//set low to start CARRIER ON period
						
						// if bit to be sent is 1, off period (pinmask = 0x00) for 
						if(DataToTx<<bitsSent & 0x80000000){
									offPdMult = 3u;/* '0' lo = 3 period */
						}else{
									offPdMult = 1u;/* '1' hi = 1 period */
						}
				}/* keep LED off for 1 or 3, 564uS periods based on state of bit being tx'd */	
				else if(tickcnt >=(ONPERIODCNT + (ONPERIODCNT * offPdMult))){
						//LCD_UsrLog ("BITs Sent:%d  BIT=%d Mult:%d \n",bitsSent,(DataToTx<<bitsSent & 0x80000000) ? 1 : 0,offPdMult );
						tickcnt = 0u;/*set to get back into tx next bit*/
						bitsSent++;
						
						/* Always exit through here, frame is always 32bits*/
						if(bitsSent > IRFRAMEBYTES){
								TxState = EOFTX;
								tickcnt =0;/*reset for EOF to count out time it needs to send carrier freq*/
						}
				}

      break;
      
      
      case EOFTX:
          if(tickcnt <2){
						IRLED_Off();//set low to start CARRIER ON period
					  
						//sprintf(dispstr+charcnt,"TX EOF\n");
						//charcnt += strlen(dispstr);
					}
				
					
					// Send carrier for one ON period of 564uS 
					if(tickcnt <ONPERIODCNT){
						//IRLED_Toggle();
					/*564uS carrier freq ON period over*/	
					}else{
						TxState = DONETX;
						IRLED_Off();
					}						
        
          
      break;
      
      case DONETX:
        HAL_TIM_Base_Stop_IT(&htim1);
        //LCD_UsrLog ("%s",dispstr);
        IRLED_Off();
			  bitsSent=0;
        tickcnt=0;
			  
      break;
      
			case IDLESTA:
        //LCD_UsrLog ("Sending %s  Idx:%d \n",pFrames[FrmIdx]->title,FrmIdx);
        //LCD_UsrLog ("CMID 0x%2x CHK 0x%2x \n",pFrames[FrmIdx]->cmd,pFrames[FrmIdx]->chk);
        lastState = IDLESTA;
				TxState = SOFTX;
        bitsSent=0;
        tickcnt=0;
			  IRLED_Off();
			  HAL_TIM_Base_Start_IT(&htim1);
        
      break;
      

      default:
      break;
      
  }      
    
  
  
}



/**
  * @brief  Configures IRLED GPIO.
  *
  */
void IRLED_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  /* Enable the GPIO_LED Clock */
  IRLED_GPIO_CLK_ENABLE();
	
  /* Configure the GPIO_LED pin */
  GPIO_InitStruct.Pin =   DO_IRLED_Pin;
  GPIO_InitStruct.Mode =  GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull =  GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  
  HAL_GPIO_Init(DO_IRLED_GPIO_Port, &GPIO_InitStruct);
  
	/*start with LED off*/
  HAL_GPIO_WritePin(DO_IRLED_GPIO_Port, DO_IRLED_Pin, GPIO_PIN_RESET); 
}

/**
  * @brief  Turns IRLED On.
  *    
  */
void IRLED_On(void)
{
  HAL_GPIO_WritePin(DO_IRLED_GPIO_Port, DO_IRLED_Pin, GPIO_PIN_SET); 
}

/**
  * @brief  Turns IRLED Off.
  *    
  */
void IRLED_Off(void)
{
  HAL_GPIO_WritePin(DO_IRLED_GPIO_Port, DO_IRLED_Pin, GPIO_PIN_RESET); 
}

/**
  * @brief  Toggles IRLED state.
  *    
  */
void IRLED_Toggle(void)
{
  HAL_GPIO_TogglePin(DO_IRLED_GPIO_Port, DO_IRLED_Pin); 
}
/**
  * @}
  */ 

/**
  * @}
  */ 
  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
