/**
  @page STemWin_HVAC HVAC Controller using STM32F429i Disco board running emWin Readme file
 
  @verbatim
  ******************** (C) COPYRIGHT 2018 VGK AMFT Software  *******************
  * @file    STemWin_/readme.txt 
  * @author  Vince Keiper 
  * @version V0.0.2  (See main.h for controlled version reporting )
  * @date    28-May-2018    
  * @brief   Based off the STemWin template for the STM32F429i disco board. 
  ******************************************************************************
  * @attention
  *
  *
@par Application Description

This directory contains a set of source files that implement an application based on STemWin for STM32F4xx devices.
I created the dialog screen for the HVAC default status and control view.
I have yet to merge the IR Remote code, ADC reading NTC routines and the main DoHvacSimple() from the main project.
Once that is merged then I just need write a TCP and web server and add the ethernet parts.
Or I may use my ESP32 project and just shrink the scope of that to be a Wifi to UART gateway.


README!!!!!
Until I move all the STCube related files into the project folder the project must run from the following folder.
C:\Users\keipe_v\Documents\TDI\firmware\STM32F4\STM32Cube_FW_F4_V1.16.0\Projects\STM32F429I-Discovery\Applications\STemWin
 

Note that the following user files may need to be updated:
  LCDConf_stm32f429i_disco_MB1075.c
  GUIConf.c
(if for example more GUI allocated memory is needed)

@note Care must be taken when using HAL_Delay(), this function provides accurate delay (in milliseconds)
      based on variable incremented in SysTick ISR. This implies that if HAL_Delay() is called from
      a peripheral ISR process, then the SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.
      
@note The application needs to ensure that the SysTick time base is always set to 1 millisecond
      to have correct HAL operation.

@note If the application is not running normally as mentionned above , you can accordingly modify either
      the Heap and Stack of the application or the GUI_NUMBYTES define in the GUIConf.c file


@par Directory contents 

  - STemWin/STemWin_Hvac/Inc/GUIConf.h                        Header for GUIConf.c
  - STemWin/STemWin_Hvac/Inc/LCDConf.h                        Header for LCDConf*.c
  - STemWin/STemWin_Hvac/Inc/main.h                           Main program header file
  - STemWin/STemWin_Hvac/Inc/stm32f4xx_hal_conf.h             Library Configuration file
  - STemWin/STemWin_Hvac/Inc/stm32f4xx_it.h                   Interrupt handlers header file
  - STemWin/STemWin_Hvac/Src/BASIC_HelloWorld.c               Simple demo drawing "Hello world"
  - STemWin/STemWin_Hvac/Src/GUIConf.c                        Display controller initialization
  - STemWin/STemWin_Hvac/Src/LCDConf_stm32f429i_disco_MB1075.c  Configuration file for the GUI library (MB1075 LCD)
  - STemWin/STemWin_Hvac/Src/main.c                           Main program file
  - STemWin/STemWin_Hvac/Src/stm32f4xx_it.c                   STM32F4xx Interrupt handlers
  - STemWin/STemWin_Hvac/Src/system_stm32f4xx.c               STM32F4xx system file


@par Hardware and Software environment  

  - This application runs on STM32F429xx devices.

  - This application has been tested with STMicroelectronics STM32F429I-Discovery RevB 
    boards and can be easily tailored to any other supported device 
    and development board.


@par How to use it ? 

In order to make the program work, you must do the following :
  - Open your preferred toolchain 
  - Rebuild all files and load your image into target memory
  - Run the application
 
 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */
