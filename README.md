# LG.AC_CONTROLLER_STM32F429i_LCD.GUI.TOUCH
Controls an LG AC unit over IR using an STM32F429i Discovery board with 2.4" LCD display for the UI and w/ touch screen for local control. The IR commands were reverse engineered from the original LG remote control. 


#define MAJVER 0u
#define MINVER 0u
#define BLDVER 4u
#define YERVER 18u
#define MONVER 07u
#define DAYVER 16u
#define HRSVER 01u
#define MNSVER 03u
#define APPNUM "2018_A021"
#define APPNAME "LG AC Controller\IR emulator w/ autodefrost w/MQTT-SER GWY"
#define AUTHRFULL "Vince Keiper"
#define AUTHRABR "V.K. "

//v0.0.4  1807160103  1. Connected to Uart-Wifi gateway 
//                    2. Changed LCD background to black as well as images
//										3. Added OP mode and software demand Set point text on LCD
//										4. Added handler for ESP32 opening uart wakeup transmission, was breaking uart parser
//										5. Fixed sending the cond coil temp 2x, no room temp was being sent
//										6. Default to AC ON on power up or reset
//v0.0.3  1807051930  1. Moved IR led to PD7 from PD2
//                    2. Addded UART5 Rx,TX for phy to talk to ESP3 MQTT to UART gateway 
//v0.0.2  1805282335  1. Addded AcCooling sensing based on delta between condensor and room temps.
//                    2. Added hide of snowflake if not AcCooling (already hid if FrostError 
//                    3. Added uptime count to overwrite condensor temp label. Until I have a better place


I created the dialog screen for the HVAC default status and control view.
I just merged the IR Remote code, ADC\NTC routines and the main DoHvacSimple() from the doner project where I developed the IR remote emulator.
IR Remote Emulator project.. https://github.com/vkeiper/LG.AC.Ir.Remote.Txr.git

The next release will add more GUI control from the LCD. 
Then I will add a TCP and web server and add the ethernet hardware.
Or I may use my ESP32 project and just shrink the scope of that to be a Wifi to UART gateway.



Until I move all the STCube related files into the project folder the project must run from the following folder.
C:\Users\keipe_v\Documents\TDI\firmware\STM32F4\STM32Cube_FW_F4_V1.16.0\Projects\STM32F429I-Discovery\Applications\STemWin
 

@note The following files may need to be updated as add more display screens:
  LCDConf_stm32f429i_disco_MB1075.c
  GUIConf.c
**More GUI allocated memory may be needed **
***You can accordingly modify either the Heap and Stack of the application or the GUI_NUMBYTES define in the GUIConf.c file***

@note SysTick interrupt must have higher priority (numerically lower)
      than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
      To change the SysTick interrupt priority you have to use HAL_NVIC_SetPriority() function.
      
@note The application needs to ensure that the SysTick time base is always set to 1 millisecond
      to have correct HAL operation.


@par Project contents 
SYSTEMS DRIVERS LCD DISPLAY\SDRAM\LTDC\emWIN STACK
  - STemWin/STemWin_Hvac/Inc/GUIConf.h                        Header for GUIConf.c
  - STemWin/STemWin_Hvac/Inc/LCDConf.h                        Header for LCDConf*.c
  - STemWin/STemWin_Hvac/Inc/main.h                           Main program header file
  - STemWin/STemWin_Hvac/Inc/stm32f4xx_hal_conf.h             Library Configuration file
  - STemWin/STemWin_Hvac/Inc/stm32f4xx_it.h                   Interrupt handlers header file
  - STemWin/STemWin_Hvac/Src/GUIConf.c                        Display controller initialization
  - STemWin/STemWin_Hvac/Src/LCDConf_stm32f429i_disco_MB1075.c  Configuration file for the GUI library (MB1075 LCD)
  - STemWin/STemWin_Hvac/Src/main.c                           Main program file
  - STemWin/STemWin_Hvac/Src/stm32f4xx_it.c                   STM32F4xx Interrupt handlers
  - STemWin/STemWin_Hvac/Src/system_stm32f4xx.c               STM32F4xx system file
  


APPLICATION RELATED 
   - STemWin/STemWin_Hvac/Src/WindowDialog.c                   Simple HVAC status screen.
   - STemWin/STemWin_Hvac/Src/ir_rmt_txr.c                     Implements a 38Khz carrier freq. Manchester encoded IR LED driver. 
   - STemWin/STemWin_Hvac/Src/hvac_ctl.c                       Simple frost monitor and autodefrost control by sending power off for 
   - STemWin/STemWin_Hvac/Src/10kntc.c                         Driver to report the temperature by passing in 12bit ADC reading. Implemented for US Sensors 10K-25degC  
  
@par Hardware and Software environment  

  - This application runs on STM32F429xx devices.
  - This application was built to run on STM32F429I-Discovery RevB 
  - This application was built using Keil v5 Level-0 optimizations
    

@par How to use it ? 

In order to make the program work, you must do the following :
  - Connect an IR LED to PD2 with a series resitor for about 2mA at 3V. You must mount this at LG AC unit front panel control unit. 
  - Connect NTC temp sensor 10K at 25degC in series with a 10K resistor to Vdd(3V). The condensing coil temp monitor to PA5 and the room sensor is PF6.
  - The blue buttn sends power on\off commands over IR LED.
 
 <h3><center>&copy; COPYRIGHT VGK AMFT Soft 2018</center></h3>
 

