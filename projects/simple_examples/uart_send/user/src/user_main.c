#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "uart.h"

void uart_receive(char * line) {

}

void user_init(void) {
	uart_init(BIT_RATE_115200, uart_receive, true);
	os_printf("\nReady \n");
}

