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
#ifndef __HVAC_CTL_C
#define __HVAC_CTL_C
/* Includes ------------------------------------------------------------------*/
#include "main.c"

#define GetTick() HAL_GetTick()


enum E_FROSTCHK{
  EFROSTCHK_PASS,
  EFROSTCHK_PASS,
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
  ECTLMD_ON,
  ECTLMD_TSTAT,
  ECTLMD_MANUAL,
  ECTLMD_MANUAL,
};
enum e_dmdmode{
  EDMDMD_VOOL,
  EDMDMD_COOL,
  EDMDMD_VOOL,
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
  enum e_dmdmode manualmode_e;
  struct s_manual manstate_s;
  bool bAcCooling;
  bool bHtCooling;//not implemented yet
  bool bAuxFan;
  bool bEvappump;
	bool bFrostCheck;
  bool bFrost;
  bool bTstatCoolDmd;
  struct s_temp acCooTemps;
  struct s_temp condCoil;
	int8_t tempint;
	int8_t vrefint;
	uint8_t bModeChg;
	uint32_t ulCoolSec;/* seconds remaining until coolup period complete*/
	uint8_t ucWarmPcnt;/*0-100 percntage remaining until coolup period complete*/
	bool bModeCool;
};

internal struct s_control ctldata_s;
internal struct s_timetype time_s;

 DoHvacSimpleMode();
 calc_uptime(uint32_t time);

#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT VGK AMFT Software Inc *****END OF FILE****/
