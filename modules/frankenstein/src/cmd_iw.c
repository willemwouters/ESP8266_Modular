#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "uart.h" 
#include "microrl.h"
#include "console.h"

static void  scan_done_cb(void *arg, STATUS status)
{
	scaninfo *c = arg; 
	struct bss_info *inf; 
	if (!c->pbss)
		return;
	STAILQ_FOREACH(inf, c->pbss, next) {
		console_printf("BSSID %02x:%02x:%02x:%02x:%02x:%02x channel %02d rssi %02d auth %-12s %s\n", 
				MAC2STR(inf->bssid),
				inf->channel, 
				inf->rssi, 
				id_to_encryption_mode(inf->authmode),
				inf->ssid
			);
		inf = (struct bss_info *) &inf->next;
	}
	console_lock(0);
}


static int  do_scan(int argc, const char*argv[])
{
	if (wifi_get_opmode() == SOFTAP_MODE)
	{
		console_printf("Can't scan, while in softap mode\n");
		return;
	}

	wifi_station_scan(NULL, &scan_done_cb);
	console_lock(1); /* Lock till we've finished scanning */
}


static int  do_iwmode(int argc, const char*argv[])
{
	int mode, newmode; 
	mode = wifi_get_opmode();
	if (argc == 1) {
		console_printf("Wireless mode: %s", id_to_wireless_mode(mode));
		return 0;
	} else {
		newmode = id_from_wireless_mode(argv[1]);
		if (-1 == newmode) { 
			console_printf("Invalid mode specified: %s\n", argv[1]);
			return 1;
		}

		if (0 == newmode) 
			wifi_station_disconnect();
		
		console_printf("Wireless mode change: %s -> %s", 
				id_to_wireless_mode(mode), id_to_wireless_mode(newmode));
		wifi_set_opmode(newmode);
	}	
}


static int conntimes = 0;
static volatile os_timer_t conn_checker;
static void conn_checker_handler(void *arg)
{
	int state = wifi_station_get_connect_status();
	switch (state) {
	case STATION_CONNECT_FAIL:
		console_printf("Failed\n");
		goto bailout;		
	case STATION_NO_AP_FOUND:
		console_printf("NotFound\n");
		goto bailout;		
	case STATION_GOT_IP:
		console_printf("Connected\n");
		goto bailout;
	}

	conntimes++;
	if (conntimes > 20) {
		console_printf("Timeout, still trying\n");
		goto bailout;
	}

	return;
bailout:
	os_timer_disarm(&conn_checker);
	console_lock(0);
	return;
}

static int  do_iwconnect(int argc, const char*argv[])
{
	int mode, newmode; 
	mode = wifi_get_opmode();
	if ((mode != STATION_MODE) && (mode != STATIONAP_MODE)) {
		console_printf("Cannot connect while in '%s' mode", id_to_wireless_mode(mode));
		return 0;
	} 

	if (argc==1) {
		/* TODO */
		return 0;
	} 

	struct station_config sta_conf;
	os_strncpy(&sta_conf.ssid, argv[1], 32);

	sta_conf.password[0] = 0x0;
	if (argc == 3)
		os_strncpy(&sta_conf.password, argv[2], 32);

	wifi_station_set_config(&sta_conf);		
	wifi_station_disconnect();
	wifi_station_connect();

	os_timer_disarm(&conn_checker);
	os_timer_setfn(&conn_checker, (os_timer_func_t *)conn_checker_handler, NULL);
	os_timer_arm(&conn_checker, 300, 1);
	conntimes = 0;
	console_lock(1);

	return 0;
}

static int  do_apconfig(int argc, const char*argv[])
{
        struct softap_config config;
        wifi_softap_get_config(&config);
        char password[33];
        char macaddr[6];

        wifi_get_macaddr(SOFTAP_IF, macaddr);

	if (argc == 1) { 
		console_printf("SSID: %s AUTH %s BSSID: " MACSTR,
			       config.ssid, id_to_encryption_mode(config.authmode), MAC2STR(macaddr));
		return 0;
	}


	strcpy(config.ssid, argv[1]);

	int authmode = id_from_encryption_mode(argv[2]);
	if (authmode == -1) { 
		console_printf("Invalid encryption mode: %s. See help.\n", argv[2]);
		return 1;
	}

	if ((authmode != AUTH_OPEN) && (argc < 4)) {
		console_printf("This authorisation mode needs a password\n");
		return 1;
	}

	os_memset(config.password, 0, sizeof(config.password));
	
	if ((authmode != AUTH_OPEN) && (argc == 4)) {  
		os_sprintf(password, MACSTR "_%s", MAC2STR(macaddr), argv[3]);
		os_memcpy(config.password, password, os_strlen(password));
	}
	
        config.authmode = authmode;

        wifi_softap_set_config(&config);
}

/*
CONSOLE_CMD(iwscan, -1, -1, 
	    do_scan, NULL, NULL, 
	    "Scan for available stations");

CONSOLE_CMD(iwmode, -1, 2, 
	    do_iwmode, NULL, NULL,
	    "Get/set wireless mode. Available modes: NONE, STA, AP, APSTA"
	    HELPSTR_NEWLINE "iwmode STA"
);

CONSOLE_CMD(iwconnect, -1, 3, 
	    do_iwconnect, NULL, NULL,
	    "Join a network/Display connection status. "
	    HELPSTR_NEWLINE "iwconnect ssid password");

CONSOLE_CMD(apconfig, 3, 4, 
	    do_apconfig, NULL, NULL, 
	    "Setup Access Point. "
	    HELPSTR_NEWLINE "apconfig name OPEN/WEP/WPA_PSK/WPA2_PSK/WPA_WPA2_PSK [password]");
*/
