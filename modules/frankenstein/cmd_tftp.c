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
#include <stdarg.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <lwip/udp.h>


#include <tftp.h>

struct update_server {
	struct tftp_server *ts;
	char buffer[SPI_FLASH_SEC_SIZE];
	int pos; 
	uint32_t numbytes;
	uint32_t fblock;
	int ready; 
	volatile os_timer_t commit_timer;
};

static struct update_server *u;

static  __attribute__ ((section(".iram0.text"))) isr_dummy(void *a)
{
	ets_uart_printf("%p\n", a);
}

static  __attribute__ ((section(".iram0.text"))) void commit_handler(void* a)
{
	int i; 
	char tmp[SPI_FLASH_SEC_SIZE];

	/* Inhibit any interrupts */

	ets_wdt_disable();
	uint32_t numsect = (u->numbytes >> (ffs(SPI_FLASH_SEC_SIZE) - 1));
	numsect++;

	ets_uart_printf("\nCommiting update, %ld sectors %ld bytes\n", numsect, u->numbytes);
	
	for (i=0; i < numsect; i++) { 
		console_printf("#");
		spi_flash_erase_sector( i );
		spi_flash_read((64 + i) * SPI_FLASH_SEC_SIZE,  (uint32 *) u->buffer, SPI_FLASH_SEC_SIZE);
		spi_flash_write((i * SPI_FLASH_SEC_SIZE), (uint32 *)u->buffer, SPI_FLASH_SEC_SIZE);
		spi_flash_erase_sector( 64 + i );
	}


	ets_uart_printf("\nFirmware update completed, rebooting\n", i);
	ets_wdt_enable();
	while (1);
}


void recv_cb(struct tftp_server *ts, int num_block, char* buf, int len)
{
	bool islast = (len != 512);
	int tocopy = min_t(unsigned int, len, SPI_FLASH_SEC_SIZE - u->pos);
	memcpy(&u->buffer[u->pos], buf, tocopy);
	u->pos+=tocopy; 

	if (islast || (u->pos == SPI_FLASH_SEC_SIZE)) { 
		console_printf("#");
		if (0 == (u->numbytes % (10*4096)))
			console_printf("\n ");
				
		u->pos=0;		
		spi_flash_erase_sector(u->fblock);
		spi_flash_write(u->fblock * SPI_FLASH_SEC_SIZE, (void *) u->buffer, 
				SPI_FLASH_SEC_SIZE);
		u->fblock++;
	}

	len -=tocopy;
	u->numbytes+=tocopy;
 
	/* Unaligned? */
	if (len)
		return recv_cb(ts, num_block, &buf[tocopy], len);
	
	if (islast) { 
		console_printf("\n TFTP done, %d bytes transferred\n", u->numbytes);
		console_printf("Committing update in 2 seconds\n");
		os_timer_disarm(&u->commit_timer);
		os_timer_setfn(&u->commit_timer, (os_timer_func_t *) commit_handler, ts);
		os_timer_arm(&u->commit_timer, 2000, 0);
	}
}

static int  do_tftp(int argc, const char*argv[])
{

	int port = 69;
	int ret; 
	if (u) {
		console_printf("tftp: Update server already started\n");
		return 0;
	}

	u = os_malloc(sizeof(struct update_server));
	if (!u) { 
		console_printf("tftp: out of memory!\n");
		return 0;
	}

	u->ts = os_malloc(sizeof(struct tftp_server));
	if (!u) { 
		console_printf("tftp: out of memory!\n");
		goto errfreeu;
	}
	
	tftp_start(u->ts, IPADDR_ANY, 69);
	tftp_recv(u->ts, recv_cb);
	const char *host = env_get("tftp-server");
	const char *path = env_get("tftp-dir"); 
	const char *file = env_get("tftp-file");
	if (!host) { 
		console_printf("Please set 'tftp-server' var\n");
		goto errfreets;
	}
	if (!path) { 
		console_printf("Please set 'tftp-path' var\n");
		goto errfreets;
	}
	if (!file) { 
		console_printf("Please set 'tftp-file' var\n");
		goto errfreets;
	}

	console_printf("TFTP: Downloading tftp://%s%s%s\n", host, path, file);
	tftp_request(u->ts, host, path, file);
	u->pos = 0;
	u->fblock = 64;
	u->numbytes = 0;
	u->ready = 0;
	console_printf("\n ");
	console_lock(1);
	return;

errfreets:
	os_free(u->ts);
errfreeu:
	os_free(u);
	u=NULL;
}
/*
CONSOLE_CMD(tftp, 1, 1, 
	    do_tftp, NULL, NULL, 
	    "Update firmware over tftp"
	    HELPSTR_NEWLINE "tftp"
	);
*/

