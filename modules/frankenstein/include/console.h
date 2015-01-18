#ifndef CONSOLE_H
#define CONSOLE_H

#define CONFIG_ENV_OFFSET 0x7f000
#define CONFIG_ENV_LEN 0x1000
#define CONFIG_ADDR_BLOBSETTING1 0x7d000
#define CONFIG_ADDR_BLOBSETTING2 0x7e000

struct console_cmd {
	const char *name; 
	const char *help;
	int required_args;
	int maximum_args;
	int (*handler)(int argc, const char * const * argv);
	char ** (*get_completion) (int argc, const char * const * argv );
	void (*interrupt)(void);
} ;


extern int (*console_printf)(const char *fmt, ...);

#define HELPSTR_NEWLINE "\n             "



#endif

