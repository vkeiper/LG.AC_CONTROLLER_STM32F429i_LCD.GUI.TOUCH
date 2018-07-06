/**
  ******************************************************************************
  * File Name          : modbusrtu.h
  * Description        : This file provides code for the configuration
  *                      of the MODBUS RTU interface.
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __mbrtu
#define __mbrtu_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
uint8_t DoUartServer(void);
HAL_StatusTypeDef AsyncTransmit(uint8_t *pBuff,uint16_t len);
#ifdef __cplusplus
}
#endif
#endif /*__ mbrtu_H */

/**
  * @}
  */

/**
  * @}
  */


