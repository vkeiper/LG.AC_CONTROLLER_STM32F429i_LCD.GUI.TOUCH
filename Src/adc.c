/**
  ******************************************************************************
  * File Name          : ADC.c
  * Description        : This file provides code for the configuration
  *                      of the ADC instances.
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
#include "adc.h"


/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc3;

/* ADC1 init function */
void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    //_Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
    */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    //_Error_Handler(__FILE__, __LINE__);
  }

}
/* ADC3 init function */
void MX_ADC3_Init(void)
{
  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
    */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

	
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
		HAL_Delay(10);//error
  }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
    */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    HAL_Delay(10);//error
  }

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();
  
    /**ADC1 GPIO Configuration    
    PA5     ------> ADC1_IN5 
    */
    GPIO_InitStruct.Pin = AIN_TEMP_ACOIL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(AIN_TEMP_ACOIL_GPIO_Port, &GPIO_InitStruct);

		/* USER CODE BEGIN ADC1_MspInit 1 */
		/* Start the conversion process #######################################*/  
		if(HAL_ADC_Start(adcHandle) != HAL_OK)
		{
			/*Error*/
		}
		
  /* USER CODE END ADC1_MspInit 1 */
  }
  else if(adcHandle->Instance==ADC3)
  {
  /* USER CODE BEGIN ADC3_MspInit 0 */

  /* USER CODE END ADC3_MspInit 0 */
    /* ADC3 clock enable */
    __HAL_RCC_ADC3_CLK_ENABLE();
  
    /**ADC3 GPIO Configuration    
    PF6     ------> ADC3_IN4 
    */
    GPIO_InitStruct.Pin = AIN_TEMP_ROOM_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(AIN_TEMP_ROOM_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN ADC3_MspInit 1 */
		/* Start the conversion process #######################################*/  
		if(HAL_ADC_Start(adcHandle) != HAL_OK)
		{
			/*Error*/
		}

  /* USER CODE END ADC3_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();
  
    /**ADC1 GPIO Configuration    
    PA5     ------> ADC1_IN5 
    */
    HAL_GPIO_DeInit(AIN_TEMP_ACOIL_GPIO_Port, AIN_TEMP_ACOIL_Pin);

  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
  else if(adcHandle->Instance==ADC3)
  {
  /* USER CODE BEGIN ADC3_MspDeInit 0 */

  /* USER CODE END ADC3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC3_CLK_DISABLE();
  
    /**ADC3 GPIO Configuration    
    PF6     ------> ADC3_IN4 
    */
    HAL_GPIO_DeInit(AIN_TEMP_ROOM_GPIO_Port, AIN_TEMP_ROOM_Pin);

  /* USER CODE BEGIN ADC3_MspDeInit 1 */

  /* USER CODE END ADC3_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */
uint16_t GetAdcConversion(ADC_HandleTypeDef AdcHandle)
{
		uint16_t retval=0xFFFF;
	if(HAL_ADC_Start(&AdcHandle) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}

  
  /*##-4- Wait for the end of conversion #####################################*/  
   /*  Before starting a new conversion, you need to check the current state of 
        the peripheral; if it’s busy you need to wait for the end of current
        conversion before starting a new one.
        For simplicity reasons, this example is just waiting till the end of the 
        conversion, but application may perform other tasks while conversion 
        operation is ongoing. */
  HAL_ADC_PollForConversion(&AdcHandle, 10);
  
  /* Check if the continuous conversion of regular channel is finished */
  if((HAL_ADC_GetState(&AdcHandle) & HAL_ADC_STATE_EOC_REG) == HAL_ADC_STATE_EOC_REG)
  {
    /*##-5- Get the converted value of regular channel #######################*/
    retval = HAL_ADC_GetValue(&AdcHandle);
  }
    
	return retval;
}

uint16_t GetVrefConversion(void)
{
		uint16_t retval=0xFFFF;
	
	ADC_ChannelConfTypeDef sConfig;

	if(HAL_ADC_Stop(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    HAL_Delay(10);//error
  }
	
	if(HAL_ADC_Start(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	

  
  /*##-4- Wait for the end of conversion #####################################*/  
   /*  Before starting a new conversion, you need to check the current state of 
        the peripheral; if it’s busy you need to wait for the end of current
        conversion before starting a new one.
        For simplicity reasons, this example is just waiting till the end of the 
        conversion, but application may perform other tasks while conversion 
        operation is ongoing. */
  HAL_ADC_PollForConversion(&hadc1, 10);
  
  /* Check if the continuous conversion of regular channel is finished */
  if((HAL_ADC_GetState(&hadc1) & HAL_ADC_STATE_EOC_REG) == HAL_ADC_STATE_EOC_REG)
  {
    /*##-5- Get the converted value of regular channel #######################*/
    retval = HAL_ADC_GetValue(&hadc1);
  }
    
	if(HAL_ADC_Stop(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    HAL_Delay(10);//error
  }
	
	if(HAL_ADC_Start(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	
	
	return retval;
}

uint16_t GetTempIntConversion(void)
{
		uint16_t retval=0xFFFF;
	
	ADC_ChannelConfTypeDef sConfig;

	if(HAL_ADC_Stop(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    HAL_Delay(10);//error
  }
	
	if(HAL_ADC_Start(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	

  
  /*##-4- Wait for the end of conversion #####################################*/  
   /*  Before starting a new conversion, you need to check the current state of 
        the peripheral; if it’s busy you need to wait for the end of current
        conversion before starting a new one.
        For simplicity reasons, this example is just waiting till the end of the 
        conversion, but application may perform other tasks while conversion 
        operation is ongoing. */
  HAL_ADC_PollForConversion(&hadc1, 10);
  
  /* Check if the continuous conversion of regular channel is finished */
  if((HAL_ADC_GetState(&hadc1) & HAL_ADC_STATE_EOC_REG) == HAL_ADC_STATE_EOC_REG)
  {
    /*##-5- Get the converted value of regular channel #######################*/
    retval = HAL_ADC_GetValue(&hadc1);
  }
    
	if(HAL_ADC_Stop(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    HAL_Delay(10);//error
  }
	
	if(HAL_ADC_Start(&hadc1) != HAL_OK)
	{
			HAL_Delay(10);/*Error*/
	}
	
	
	return retval;
}
/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
