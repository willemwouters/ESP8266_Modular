#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "TcpServer.h"

#include "config.h"
#include "uart.h"

static ETSTimer tickTimer;

static struct softap_config apconf;
//uart0_tx_buffer(data, strlen(data)); // will data back to uart
//err_t err = SendUdpData(data);;

void ReceiveUART(char * val) {
	os_printf("Received stuff: %s \r\n", val);
}

void tickCb(void * arg) {
	//SetupTcpUdpServer(1, 60000, 2, ReceiveTcpUdp); // enabled, port, mode, callback
	//err_t err = SendUdpData("TEST");
	switch (config_mode) {
		case MODE_CONNECTING:
			tick_connecting();
			os_timer_arm(&tickTimer, TICK_SPEED_CONNECTING, 0);
			break;
		case MODE_MASTER_C:
		case MODE_MASTER_S:
			tick_master();
			os_timer_arm(&tickTimer, TICK_SPEED_MASTER, 0);
			break;
		case MODE_SLAVE:
			tick_slave();
			os_timer_arm(&tickTimer, TICK_SPEED_SLAVE, 0);
			break;
		default:
			break;
	}
	//os_printf("Send out data: %d", err);
	//os_printf("Timer tick\r\n");

}

void StartupCb(void * arg) {
	config_mode = MODE_CONNECTING;
	wifi_set_opmode(STATION_MODE);
	os_printf("WiFi AP up\r\n");
	os_timer_disarm(&tickTimer);
	os_timer_setfn(&tickTimer, tickCb, NULL);
	os_printf("Main timer running\r\n");
	os_timer_arm(&tickTimer, 1000, 0);
}

void user_init(void) {
	uart_init(BIT_RATE_115200, ReceiveUART, true); // baudrate, callback, eolchar, printftouart
	os_printf("Starting up....\r\n");
	os_timer_disarm(&tickTimer);
	os_timer_setfn(&tickTimer, StartupCb, NULL);
	os_timer_arm(&tickTimer, 5000, 0);
}

