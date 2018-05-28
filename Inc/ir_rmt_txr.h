#ifndef __IRRMTTXR_H
#define __IRRMTTXR_H

#include "main.h"
#include "tim.h"

#define BTN_PWR 0u
#define BTN_MOD 1u
#define BTN_TUP 2u
#define BTN_TDN 3u
#define BTN_TMR 4u




extern uint32_t tickcnt;
extern uint8_t pinmask;
extern uint8_t bSync;

void Do_Ir_Rmt_Txr(void);
uint8_t SendFrame(uint8_t fIdx);

#endif

