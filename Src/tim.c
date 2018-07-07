/**
  ******************************************************************************
  * File Name          : TIM.c
  * Description        : This file provides code for the configuration
  *                      of the TIM instances.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "ir_rmt_txr.h"
#include "main.h"
#include "modbusrtu.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;


/* TIM1 init function */
void TIM1_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 2360;//755;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    //_Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    //_Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    //_Error_Handler(__FILE__, __LINE__);
  }

}


/* TIM3 init function */
void TIM3_Init(void)
{
	uint32_t uwPrescalerValue = 0;

   /* Compute the prescaler value to have TIM3 counter clock equal to 10 KHz */
  uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 10000) - 1;
  
  /* Set TIMx instance */
  htim3.Instance = TIM3;
   
  /* Initialize TIM3 peripheral as follows:
       + Period = 500 - 1
       + Prescaler = ((SystemCoreClock/2)/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  htim3.Init.Period = 500 - 1;
  htim3.Init.Prescaler = uwPrescalerValue;
  htim3.Init.ClockDivision = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  if(HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    while(1) 
    {
    }
  }
  
  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if(HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
  {
    while(1) 
    {
    }
  }

}




void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{
  if(tim_baseHandle->Instance==TIM1)
  {
			/* Peripheral clock enable */
			__HAL_RCC_TIM1_CLK_ENABLE();

					/* Peripheral interrupt init*/
			HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 6, 0);
			HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

			/* Enable the timer clock */
			//HAL_TIM_Base_Start_IT(tim_baseHandle);
  }else if(tim_baseHandle->Instance==TIM3)
	{
		  /*##-1- Enable peripherals and GPIO Clocks #################################*/
			/* TIMx Peripheral clock enable */
			__HAL_RCC_TIM3_CLK_ENABLE();

			/*##-2- Configure the NVIC for TIMx ########################################*/
			/* Set the TIMx priority */
			HAL_NVIC_SetPriority(TIM3_IRQn, 0, 1);
			
			/* Enable the TIMx global Interrupt */
			HAL_NVIC_EnableIRQ(TIM3_IRQn);
	}else if(tim_baseHandle->Instance==TIM4)
  {
    /* Peripheral clock enable */
    __TIM4_CLK_ENABLE();

    /* Peripheral interrupt init*/
    HAL_NVIC_SetPriority(TIM4_IRQn, 0, 2);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
 
		/* Enable the timer clock */
		HAL_TIM_Base_Start_IT(tim_baseHandle);

  }
}


void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM10)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM10_CLK_DISABLE();

    /* TIM10 interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
  }else if(tim_baseHandle->Instance==TIM4)
  {
    /* Peripheral clock disable */
    __TIM4_CLK_DISABLE();

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(TIM4_IRQn);
  }
} 


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	  // This callback is automatically called by the HAL on the UEV event
	
	  if(htim->Instance == TIM1){
				tickcnt++;
				/* run state machine*/
				Do_Ir_Rmt_Txr();
	  }else if(htim->Instance==TIM3)
		{
				BSP_Background();
		}else if(htim->Instance==TIM4)
    {
				MX_Timer4_StartStop(0);//shutoff the timer
				SER_TmrCpltCallback();
    }
}

//TODO: timer 4 not working, crashes system
//static uint16_t tmr4pd;
/* TIM4 init function */
void MX_TIM4_Init(uint32_t baud, uint8_t bits)
{
//  /*frame timeout*/
//  float frmto;
//  /*Timer tick 10uS*/
//  float tick = .000010;
// 
//  
//  /*Frame Seperation Time (3.5 chars) = 1/baud * (qty bits) * 3.5*/
//  /*Qty Bits = 1start + 8 data + qty stop + qty parity Default = 10*/
//  frmto = ((1.00f/(float)baud) * bits) * 3.5f;
//  tmr4pd = (uint16_t)(frmto/tick);
//  //tmr4pd = UINT16_MAX - tmr4pd;
//    
//  TIM_ClockConfigTypeDef sClockSourceConfig;
//  TIM_MasterConfigTypeDef sMasterConfig;

//  htim4.Instance = TIM4;
//  htim4.Init.Prescaler = 1679;/*10uS tick*/
//  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
//  htim4.Init.Period = tmr4pd;
//  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//  HAL_TIM_Base_Init(&htim4);
//    
//  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
//  HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig);
//  
//  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
//  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
//  HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig);

	uint32_t uwPrescalerValue = 0;

   /* Compute the prescaler value to have TIM3 counter clock equal to 10 KHz */
  uwPrescalerValue = (uint32_t) ((SystemCoreClock /2) / 10000) - 1;
  
  /* Set TIMx instance */
  htim4.Instance = TIM4;
   
  /* Initialize TIM3 peripheral as follows:
       + Period = 500 - 1
       + Prescaler = ((SystemCoreClock/2)/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  htim4.Init.Period = 1000;
  htim4.Init.Prescaler = uwPrescalerValue;
  htim4.Init.ClockDivision = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  if(HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    while(1) 
    {
    }
  }
  
  /*##-2- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if(HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
  {
    while(1) 
    {
    }
  }
}


void MX_Timer4_StartStop(uint8_t on)
{
    if(on == 1)
    {
        /*Clear counter*/
        __HAL_TIM_SET_COUNTER(&htim4,0);
        HAL_TIM_Base_Start_IT(&htim4);
    }else{
        /*Shutdown timer*/
         HAL_TIM_Base_Stop_IT(&htim4);
        /*Clear counter*/
        __HAL_TIM_SET_COUNTER(&htim4,0);
    }
}



void MX_Timer4_Clear(void)
{
			/*Clear counter*/
			__HAL_TIM_SET_COUNTER(&htim4,0);
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
