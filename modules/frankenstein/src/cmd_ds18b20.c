/*
 * Adaptation of Paul Stoffregen's One wire library to the ESP8266 and
 * Necromant's Frankenstein firmware by Erland Lewin <erland@lewin.nu>
 * 
 * Paul's original library site:
 *   http://www.pjrc.com/teensy/td_libs_OneWire.html
 * 
 * See also http://playground.arduino.cc/Learning/OneWire
 * 
 */
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

#include <stdlib.h>


#ifdef CONFIG_CMD_DS18B20_DEBUG
#define dbg(fmt, ...) console_printf(fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...)
#endif

/*
 * static function prototypes
 */
static int ds_search( uint8_t *addr );
static void reset_search();
static void write_bit( int v );
static void write( uint8_t v, int parasitePower );
static inline int read_bit(void);
static inline void write_bit( int v );
static void select(const uint8_t rom[8]);
void ds_init( int gpio );
static uint8_t reset(void);
static uint8_t read();
static uint8_t crc8(const uint8_t *addr, uint8_t len);

/*
 * Parameter: <gpio>
 */
static int do_ds18b20(int argc, const char* argv[])
{
	const char *tmp = argv[1];
	int gpio; // = skip_atoi(&tmp);
	int r, i;
	uint8_t addr[8], data[12];
	
	gpio = skip_atoi( &tmp );

	// We will need to use microsecond timer
	// wait 500 us
	// system_timer_reinit();

	ds_init( gpio );

	dbg( "After init.\n" );

	r = ds_search( addr );
	if( r )
	{
		dbg( "Found Device @ %02x %02x %02x %02x %02x %02x %02x %02x\n", 
				addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], 
				addr[6], addr[7] );
		if( crc8( addr, 7 ) != addr[7] )
			console_printf( "CRC mismatch, crc=%xd, addr[7]=%xd\n",
					crc8( addr, 7 ), addr[7] );

		switch( addr[0] )
		{
		case 0x10:
			dbg( "Device is DS18S20 family.\n" );
			break;

		case 0x28:
			dbg( "Device is DS18B20 family.\n" );
			break;

		default:
			console_printf( "Device is unknown family.\n" );
			return 1;
		}
	}
	else { 
		console_printf( "No DS18B20 detected, sorry\n" );
		return 1;
	}
	// perform the conversion
	reset();
	select( addr );

	write( 0x44, 1 ); // perform temperature conversion

	os_delay_us( 1000000 );

	dbg( "Scratchpad: " );
	reset();
	select( addr );
	write( 0xbe, 0 ); // read scratchpad
	
	for( i = 0; i < 9; i++ )
	{
		data[i] = read();
		dbg( "%2x ", data[i] );
	}
	dbg( "\n" );

	int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;  // test most sig bit
	if (SignBit) // negative
		TReading = (TReading ^ 0xffff) + 1; // 2's comp
	
	Whole = TReading >> 4;  // separate off the whole and fractional portions
	Fract = (TReading & 0xf) * 100 / 16;

	console_printf( "Temperature: %c%d.%d Celsius\n", SignBit ? '-' : '+', 
			Whole, Fract < 10 ? 0 : Fract);
	return r;
}

// global search state
static unsigned char ROM_NO[8];
static uint8_t LastDiscrepancy;
static uint8_t LastFamilyDiscrepancy;
static uint8_t LastDeviceFlag;
static int gpioPin;

void ds_init( int gpio )
{
	//set gpio2 as gpio pin
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	  
	//disable pulldown
	PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO2_U);
	  
	//enable pull up R
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO2_U);  
	  
	// Configure the GPIO with internal pull-up
	// PIN_PULLUP_EN( gpio );

	GPIO_DIS_OUTPUT( gpio );

	gpioPin = gpio;

	reset_search();
}

static void reset_search()
{
	int i;
	// reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag = FALSE;
	LastFamilyDiscrepancy = 0;
	for(i = 7; i > 0 ; i--) {
		ROM_NO[i] = 0;
		if ( i == 0) break;
	}
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
static uint8_t reset(void)
{
	//	IO_REG_TYPE mask = bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
	uint8_t retries = 125;

	// noInterrupts();
	// DIRECT_MODE_INPUT(reg, mask);
	GPIO_DIS_OUTPUT( gpioPin );
	
	// interrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		os_delay_us(2);
	} while ( !GPIO_INPUT_GET( gpioPin ));

	// noInterrupts();
	GPIO_OUTPUT_SET( gpioPin, 0 );
	// DIRECT_WRITE_LOW(reg, mask);
	// DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
	// interrupts();
	os_delay_us(480);
	// noInterrupts();
	GPIO_DIS_OUTPUT( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// allow it to float
	os_delay_us(70);
	// r = !DIRECT_READ(reg, mask);
	r = !GPIO_INPUT_GET( gpioPin );
	// interrupts();
	os_delay_us(410);

	return r;
}

/* pass array of 8 bytes in */
static int ds_search( uint8_t *newAddr )
{
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number;
	uint8_t id_bit, cmp_id_bit;
	int search_result;
	int i;
	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	// if the last call was not the last one
	if (!LastDeviceFlag)
	{
		// 1-Wire reset
		if (!reset())
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}

		// issue the search command
		write(0xF0, 0);

		// loop to do the search
		do
		{
			// read a bit and its complement
			id_bit = read_bit();
			cmp_id_bit = read_bit();
	 
			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1))
				break;
			else
			{
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit)
					search_direction = id_bit;  // bit write value for search
				else
				{
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy)
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					else
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);

					// if 0 was picked then record its position in LastZero
					if (search_direction == 0)
					{
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1)
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				else
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;

				// serial number search direction write bit
				write_bit(search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0)
				{
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		}
		while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = TRUE;

			search_result = TRUE;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}
	for (i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];
	return search_result;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
static void write( uint8_t v, int power ) {
	uint8_t bitMask;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		write_bit( (bitMask & v)?1:0);
	}
	if ( !power) {
		// noInterrupts();
		GPIO_DIS_OUTPUT( gpioPin );
		GPIO_OUTPUT_SET( gpioPin, 0 );
		// DIRECT_MODE_INPUT(baseReg, bitmask);
		// DIRECT_WRITE_LOW(baseReg, bitmask);
		// interrupts();
	}
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline void write_bit( int v )
{
	// IO_REG_TYPE mask=bitmask;
	//	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;

	GPIO_OUTPUT_SET( gpioPin, 0 );
	if( v ) {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(10);
		GPIO_OUTPUT_SET( gpioPin, 1 );
		// DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		// interrupts();
		os_delay_us(55);
	} else {
		// noInterrupts();
		//	DIRECT_WRITE_LOW(reg, mask);
		//	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		os_delay_us(65);
		GPIO_OUTPUT_SET( gpioPin, 1 );
		//	DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		//		interrupts();
		os_delay_us(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
static inline int read_bit(void)
{
	//IO_REG_TYPE mask=bitmask;
	//volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	int r;
  
	// noInterrupts();
	GPIO_OUTPUT_SET( gpioPin, 0 );
	// DIRECT_MODE_OUTPUT(reg, mask);
	// DIRECT_WRITE_LOW(reg, mask);
	os_delay_us(3);
	GPIO_DIS_OUTPUT( gpioPin );
	// DIRECT_MODE_INPUT(reg, mask);	// let pin float, pull up will raise
	os_delay_us(10);
	// r = DIRECT_READ(reg, mask);
	r = GPIO_INPUT_GET( gpioPin );
	// interrupts();
	os_delay_us(53);

	return r;
}

//
// Do a ROM select
//
static void select(const uint8_t *rom)
{
	uint8_t i;

	write(0x55, 0);           // Choose ROM

	for (i = 0; i < 8; i++) write(rom[i], 0);
}

//
// Read a byte
//
static uint8_t read() {
	uint8_t bitMask;
	uint8_t r = 0;

	for (bitMask = 0x01; bitMask; bitMask <<= 1) {
		if ( read_bit()) r |= bitMask;
	}
	return r;
}

//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
static uint8_t crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	uint8_t i;
	while (len--) {
		uint8_t inbyte = *addr++;
		for (i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
/*
CONSOLE_CMD(ds18b20, 2, 2, 
	    do_ds18b20, NULL, NULL, 
	    "Read temperature from DS18B20 chip."
	    HELPSTR_NEWLINE "ds18b20 <gpio>"
	);
*/
