/*
 * slave_main.c
 *
 *  Created on: Mar 14, 2015
 *      Author: wouters
 */

#include "config.h"
#include "user_interface.h"
#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
static uint8_t chackTime = 0;
os_timer_t at_japDelayChack;
char init_slave = 0;
int connect_attempts = 0;

void wifi_connect(char * ssid, char * password);
void reset_slave() {
	init_slave = 0;
	chackTime = 0;
	os_timer_disarm(&at_japDelayChack);
}
void tick_slave() {
	os_printf("SLAVE TICK \r\n");
	if(!init_slave) {
	  wifi_connect("MASTER_AP", "00000000");
	  init_slave = 1;
	}
	if(connect_attempts > 2) {
		os_printf("Switching to MASTER \r\n");
		config_mode = MODE_MASTER_S;
		connect_attempts = 0;
		reset_slave();
	}
}


/**
  * @brief  Transparent data through ip.
  * @param  arg: no used
  * @retval None
  */
void ICACHE_FLASH_ATTR
at_japChack(void *arg)
{

  uint8_t japState;
  char temp[32];
  os_printf("Checking if we have ip...\r\n");
  os_timer_disarm(&at_japDelayChack);
  chackTime++;
  japState = wifi_station_get_connect_status();
  if(japState == STATION_GOT_IP)
  {
	os_printf("YEAHHH we have ip...\r\n");
    chackTime = 0;
    connect_attempts = 0;
  }
  else if(chackTime >= 5)
  {
    wifi_station_disconnect();
    os_sprintf(temp,"+CWJAP:%d\r\n",japState);
    os_printf(temp);
    os_printf("\r\nFAIL\r\n");
    reset_slave();
    return;
  }
  os_timer_arm(&at_japDelayChack, 2000, 0);

}


int8_t ICACHE_FLASH_ATTR
at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen)
{
//  assert(pDest!=NULL && pSrc!=NULL);

  char *pTempD = pDest;
  const char *pTempS = pSrc;
  int8_t len;

  if(*pTempS != '\"')
  {
    return -1;
  }
  pTempS++;
  for(len=0; len<maxLen; len++)
  {
    if(*pTempS == '\"')
    {
      *pTempD = '\0';
      break;
    }
    else
    {
      *pTempD++ = *pTempS++;
    }
  }
  if(len == maxLen)
  {
    return -1;
  }
  return len;
}


/**
  * @brief  Setup commad of join to wifi ap.
  * @param  id: commad id number
  * @param  pPara: AT input param
  * @retval None
  */
void ICACHE_FLASH_ATTR wifi_connect(char * ssid, char * password)
{
	char temp[64];
	struct station_config stationConf;
	int8_t len;
	connect_attempts++;
	os_bzero(&stationConf, sizeof(struct station_config));
	os_memcpy(&stationConf.ssid, ssid, os_strlen(ssid));
	os_memcpy(&stationConf.password, password, os_strlen(password));
    wifi_station_disconnect();
    os_printf("stationConf.ssid: %s\r\n", stationConf.ssid);
    os_printf("stationConf.password: %s\r\n", stationConf.password);
    ETS_UART_INTR_DISABLE();
    wifi_station_set_config(&stationConf);
    ETS_UART_INTR_ENABLE();
    wifi_station_connect();
    os_timer_disarm(&at_japDelayChack);
    os_timer_setfn(&at_japDelayChack, (os_timer_func_t *)at_japChack, NULL);
    os_timer_arm(&at_japDelayChack, 3000, 0);

}





