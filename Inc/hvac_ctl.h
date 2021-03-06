/**
  ******************************************************************************
  * File Name          : hvac_ctl.h
  * Description        : This file provides code for the control and monitor 
  *                      of LGAC unit via the reverse engineered IR remote codes
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 VGK AMFT Software Inc.  
  *
  *
  ******************************************************************************
  */
#ifndef __HVAC_CTL_H
#define __HVAC_CTL_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#define GetTick() HAL_GetTick()


enum E_FROSTCHK{
  EFROSTCHK_PASS,
  EFROSTCHK_FAIL,
};


struct s_timetype {
	uint32_t time; //in seconds
	uint32_t tm_secs;
	uint32_t tm_mins;
	uint32_t tm_hrs;
	uint32_t tm_days;
  uint32_t tm_yrs;
	char str[64];
};

enum e_ctlmode{
  ECTLMD_OFF,
  ECTLMD_TSTAT,
  ECTLMD_REMOTE,
  ECTLMD_MANUAL,
};
enum e_dmdmode{
  EDMDMD_NONE,
  EDMDMD_COOL,
  EDMDMD_HEAT,
};

struct s_manual{
  bool acpump;
  bool auxfan;
};

//in deg F
struct s_temp{
  float dmd;
  float rdb;
  float rdbC;
  uint16_t rdbraw;
  uint8_t rnghi;
  uint8_t rnglo;
};

struct s_control{
  enum e_ctlmode ctlmode_e;
  enum e_dmdmode dmdmode_e;
  enum e_dmdmode tstatmode_e;
  enum e_dmdmode rmtmode_e;
  struct s_manual manstate_s;
  bool bAcCooling;
  bool bHtHeating;//not implemented yet
  bool bAuxFan;
  bool bEvappump;
	bool bFrostCheck;
  bool bFrostErr;
  bool bTstatCoolDmd;
  struct s_temp acCoolTemps;
  struct s_temp condCoil;
	int8_t tempint;
	int8_t vrefint;
	uint8_t bModeChg;
	uint32_t ulWarmupSec;/* seconds remaining until warmup period complete*/
	uint8_t ucWarmPcnt;/*0-100 percntage remaining until warmup period complete*/
	bool bModeCool;
	bool bWifiReady;
};

extern struct s_control ctldata_s;
extern struct s_timetype time_s;

void DoHvacSimpleMode(void);
void calc_uptime(uint32_t time);

#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT VGK AMFT Software Inc *****END OF FILE****/
