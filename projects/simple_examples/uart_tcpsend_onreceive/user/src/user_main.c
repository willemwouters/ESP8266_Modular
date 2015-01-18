#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "TcpServer.h"


#include "uart.h"

static ETSTimer startupTimer;

static struct softap_config apconf;

void ReceiveTcpUdp(void * data) {
	os_printf("\r\nTCP CALLBACK====\r\n");
	uart0_tx_buffer(data, strlen(data)); // will data back to uart
	os_printf("\r\n====END_DATA\r\n");
}

void ReceiveUART(char * data) {
	os_printf("\r\nUART CALLBACK====\r\n");
	os_printf(data);
	err_t err = SendUdpData(data);
	os_printf("\r\n====%dEND_DATA\r\n", err);
}

void SetupWifiAp() {
	char * ap = "WWWWW";
	char * pass = "00000000";

	wifi_set_opmode(2);
	wifi_softap_get_config(&apconf);
	os_strncpy((char*)apconf.ssid, ap, 32);
	os_strncpy((char*)apconf.password, pass, 64);
	apconf.authmode = AUTH_WPA_WPA2_PSK;
	apconf.max_connection = 20;
	apconf.ssid_hidden = 0;
	wifi_softap_set_config(&apconf);
}

void StartupCb(void * arg) {
	SetupTcpUdpServer(1, 60000, 2, ReceiveTcpUdp); // enabled, port, mode, callback
	err_t err = SendUdpData("TEST");
	os_printf("Send out data: %d", err);
}

void user_init(void) {
	uart_init(BIT_RATE_115200, ReceiveUART, true); // baudrate, callback, eolchar, printftouart
	SetupWifiAp();

	os_timer_disarm(&startupTimer);
	os_timer_setfn(&startupTimer, StartupCb, NULL);
	os_timer_arm(&startupTimer, 5000, 0);
}

