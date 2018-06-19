/**
  ******************************************************************************
  * @file    main.h 
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    17-February-2017
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright © 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "GUI.h"

#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_ts.h"
#include "stm32f429i_discovery_sdram.h"
#include "../Components/ili9341/ili9341.h" 

#define MAJVER 0u
#define MINVER 0u
#define BLDVER 2u
#define YERVER 18u
#define MONVER 05u
#define DAYVER 28u
#define HRSVER 23u
#define MNSVER 35u
#define APPNUM "2018_A021"
#define APPNAME "LG AC Controller\IR emulator w/ autodefrost"
#define AUTHRFULL "Vince Keiper"
#define AUTHRABR "V.K. "

//v0.0.2  1805282335  1. Addded AcCooling sensing based on delta between condensor and room temps.
//                    2. Added hide of snowflake if not AcCooling (already hid if FrostError 
//                    3. Added uptime count to overwrite condensor temp label. Until I have a better place

#define AIN_TEMP_ROOM_Pin GPIO_PIN_6
#define AIN_TEMP_ROOM_GPIO_Port GPIOF
#define AIN_TEMP_ACOIL_Pin GPIO_PIN_5
#define AIN_TEMP_ACOIL_GPIO_Port GPIOA


#define DI_ACMODELED_Pin GPIO_PIN_2
#define DI_ACMODELED_GPIO_Port GPIOE
#define DI_DMD_Pin GPIO_PIN_3
#define DI_DMD_GPIO_Port GPIOE
#define DO_MAINS_SSR_Pin GPIO_PIN_4
#define DO_MAINS_SSR_GPIO_Port GPIOE
#define DO_CIRC_SSR_Pin GPIO_PIN_5
#define DO_CIRC_SSR_GPIO_Port GPIOE
#define DO_EVAP_SSR_Pin GPIO_PIN_6
#define DO_EVAP_SSR_GPIO_Port GPIOE
#define DO_IRLED_Pin GPIO_PIN_2
#define DO_IRLED_GPIO_Port GPIOD
#define IRLED_GPIO_CLK_ENABLE()                  __HAL_RCC_GPIOD_CLK_ENABLE()  
#define IRLED_GPIO_CLK_DISABLE()                 __HAL_RCC_GPIOD_CLK_DISABLE()  

/* Exported variables --------------------------------------------------------*/
extern char dbglog[64];
/* Exported types ------------------------------------------------------------*/
typedef enum
{
    FALSE,
    TRUE
} bool;
/* Exported constants --------------------------------------------------------*/


/* Exported functions --------------------------------------------------------*/
void BSP_Background(void);


#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
