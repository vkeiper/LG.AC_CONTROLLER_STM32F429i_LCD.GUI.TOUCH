
/*DECLARATIONS start-- copy top of Dialog screen*/
enum E_DISPLAYMODE{
  EDSPMD_STATUS,
  EDSPMD_NUMPAD,
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


struct s_dlgdata{
  enum E_DISPLAYMODE DISPMODE_E; /*  */
  struct s_timetype uptime;
  uint32_t t_reloadtmr;
  uint32_t t_blinkobj;
  char dbgstr[64];
};

struct s_psudata{
  struct s_manual manstate_s;
  double vrdb;
  double vset;
  uint32_t bOutputOn;/* 1= Output of PSUs ON */
};



struct s_dlgdata dlgdata_s;
struct s_psudata psudata_s;


static void calc_uptime(uint32_t time);
/*DECLARATIONS END******************************/


/*TIMER start-- paste below last function*/
/**
  * @Author Vince Keiper
  * @brief  Gets called at 1 second interval and stores application runtime
  *         
  * @param  time : uint32_t time in 1 second per tick resolution
  * @retval void 
  *
  */
static void calc_uptime(uint32_t time)
{
	time_s.tm_secs = time % 60UL;
	time /= 60UL;
	time_s.tm_mins = time % 60UL;
	time /= 60UL;
	time_s.tm_hrs = time % 24UL;
	time /= 24UL;
	time_s.tm_days = time;

	sprintf(time_s.str, "Dd:Hh:Mm:Ss %2d:%2d:%2d:%2d", time_s.tm_days, time_s.tm_hrs, time_s.tm_mins, time_s.tm_secs);
}
/*TIMER END****************************/




/* put this in the hidden form that timer where the "reload" happens*/
static var ticks100ms=0; 

if(ticks100ms++ >=9){
	dlgdata_s.uptime.time++;
	
}

/*put this in your WM_PAINT to display it*/
calc_uptime(uint32_t time);


/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT VGK AMFT Software Inc *****END OF FILE****/
