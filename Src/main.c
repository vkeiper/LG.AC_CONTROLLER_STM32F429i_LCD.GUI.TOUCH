/**
  ******************************************************************************
  * @file    STemWin/STemWin_HelloWorld/Src/main.c
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    17-February-2017
  * @brief   This file provides main program functions
  ******************************************************************************
  * @attention
  *
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "WM.h"
#include "ir_rmt_txr.h"
#include "tim.h"
#include "adc.h"
#include "gpio.h"
#include "10kntc.h"
#include "hvac_ctl.h"
#include "usart.h"
#include "modbusrtu.h"
#include <string.h>

/* Global variables -----------------------------------------------------------*/
extern WM_HWIN CreateWindow(void);


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
uint8_t GUI_Initialized = 0;
char dbglog[64];
__IO uint8_t ubKeyPressed = RESET; 
uint8_t i;


/* Private function prototypes -----------------------------------------------*/
static void BSP_Config(void);
static void SystemClock_Config(void);
void BSP_Pointer_Update(void);
void BSP_Background(void);

extern void MainTask(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */ 
int main(void)
{  
	static uint32_t t_gui=0;
	static uint32_t t_run=0, t_txmode=0;
  static uint32_t pwrtxcnt =0;
	static bool bSendPwr = FALSE;
	
  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();
  
  /* Initialize LCD and LEDs */
  BSP_Config();
  
  /* Configure the system clock to 180 MHz */
  SystemClock_Config();
  
	
		
	MX_ADC1_Init();
  MX_ADC3_Init();
  MX_GPIO_Init();
	
  TIM3_Init();

  /* Init the STemWin GUI Library */
  GUI_Init();
  GUI_Initialized = 1;
  
  /* Activate the use of memory device feature */
  WM_SetCreateFlags(WM_CF_MEMDEV);

	/*VK, create default HVAC dialog screen*/
	CreateWindow();
  
	/* Infinite loop */  
  while (1) 
  {
			if(HAL_GetTick() > t_gui +50){
				t_gui = HAL_GetTick();
				GUI_Exec();// one call to load main screen
			}
			
			/* Run HVAC control\monitoring\auto-defrost */
			DoHvacSimpleMode();
		
			/* Run UART server to receive commands from MQTT or other remote device */
			DoUartServer();
			
			
			/* Transmit IR Remote command has been requested              */
			/* Mode change method async of the DoHvac...                  */
			/* Mode change register is set in DoHvac with qty tx's needed */
			
			if(t_txmode == 0){
				/* If AC should be ON but is OFF trigger Mode button press event */
				if( ctldata_s.bModeCool == TRUE && HAL_GPIO_ReadPin(DI_ACMODELED_GPIO_Port,DI_ACMODELED_Pin) == GPIO_PIN_SET){
						bSendPwr =TRUE;
				  	sprintf(dbglog,"Sent Power ON Cnt %d bCool %d ACLED%s",++pwrtxcnt,ctldata_s.bModeCool,HAL_GPIO_ReadPin(DI_ACMODELED_GPIO_Port,DI_ACMODELED_Pin) == GPIO_PIN_RESET ? "AC ON" : "AC OFF");
				}
				/* If AC should be OFF but is ON trigger Mode button press event */
				else if( ctldata_s.bModeCool == FALSE && HAL_GPIO_ReadPin(DI_ACMODELED_GPIO_Port,DI_ACMODELED_Pin) == GPIO_PIN_RESET){
						bSendPwr = TRUE;
						sprintf(dbglog,"Sent Power OFF Cnt %d bCool %d ACLED%s",++pwrtxcnt,ctldata_s.bModeCool,HAL_GPIO_ReadPin(DI_ACMODELED_GPIO_Port,DI_ACMODELED_Pin) == GPIO_PIN_RESET ? "AC ON" : "AC OFF");
				}
										
			}else{
						/* Keep in this state for 5000 Seconds after PWR BTN command tx, stops AC ON\OFF from being checked */
						if(t_txmode !=0 && (HAL_GetTick() > t_txmode + 5000)){
							t_txmode=0;
						}
			}
			
			if(bSendPwr == TRUE){
						bSendPwr = FALSE;
						t_txmode = HAL_GetTick();
						SendFrame(BTN_PWR);
			}
				
			
			/* Stuff to do every 1 second */
			if(HAL_GetTick() > t_run + 500){
					t_run = HAL_GetTick();
			}else{
				/* Send PWR if button clicked*/
				if(ubKeyPressed == SET){
						SendFrame(BTN_PWR);
						ubKeyPressed = RESET;
					
						/* changed demanded state */
						if(ctldata_s.dmdmode_e != EDMDMD_COOL){
							ctldata_s.dmdmode_e =  EDMDMD_COOL;
						}else{
							ctldata_s.dmdmode_e =  EDMDMD_NONE;
						}
				}
			}
	}
}


/**
  * @brief  Initializes the STM32F429I-DISCO's LCD and LEDs resources.
  * @param  None
  * @retval None
  */
static void BSP_Config(void)
{
  /* Initialize STM32F429I-DISCO's LEDs */
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);
  
  /* Initializes the SDRAM device */
  BSP_SDRAM_Init();
  
  /* Initialize the Touch screen */
  BSP_TS_Init(240, 320);
  
  /* Enable the CRC Module */
	/* VK, this unlocks STemWin to be used under free ST license */
  __HAL_RCC_CRC_CLK_ENABLE();
	
	  /* Configure USER Button */
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
}

/**
  * @brief  BSP_Background.
  * @param  None
  * @retval None
  */ 
void BSP_Background(void)
{
	/*Toggle the HB led*/
  //BSP_LED_Toggle(LED3);
  
  /* Capture input event and update cursor */
  if(GUI_Initialized == 1)
  {
    BSP_Pointer_Update();
  }
}

/**
  * @brief  Provide the GUI with current state of the touch screen
  * @param  None
  * @retval None
  */
void BSP_Pointer_Update(void)
{
  GUI_PID_STATE TS_State;
  static TS_StateTypeDef prev_state;
  TS_StateTypeDef  ts;
  uint16_t xDiff, yDiff;  
  
  BSP_TS_GetState(&ts);
  
  TS_State.Pressed = ts.TouchDetected;
  
  xDiff = (prev_state.X > ts.X) ? (prev_state.X - ts.X) : (ts.X - prev_state.X);
  yDiff = (prev_state.Y > ts.Y) ? (prev_state.Y - ts.Y) : (ts.Y - prev_state.Y);
  
  if(ts.TouchDetected)
  {
    if((prev_state.TouchDetected != ts.TouchDetected )||
       (xDiff > 3 )||
         (yDiff > 3))
    {
      prev_state = ts;
      
      TS_State.Layer = 0;
      TS_State.x = ts.X;
      TS_State.y = ts.Y;
      
      GUI_TOUCH_StoreStateEx(&TS_State);
    }
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
  clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}



/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
 if (GPIO_Pin == KEY_BUTTON_PIN)
 {
   ubKeyPressed = SET;
 }
}

/**
  * @author  Vince Keiper
  * @brief  Systick callback for my application use (non system)
  * @param  None
  * @retval None
  */
void HAL_SYSTICK_Callback(void){
	static uint16_t ticks=1000;
	
	if(--ticks==0){
		ticks=1000;
		calc_uptime(time_s.time++);
	}
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

