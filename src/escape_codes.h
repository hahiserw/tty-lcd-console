#ifndef ESCAPE_CODES_H
#define ESCAPE_CODES_H

#include "config.h"


// #define ESCAPE_ARGS_LIMIT 32 // as in urxvt
#define ESCAPE_ARGS_LIMIT 10
// could be smaller, but OSC title setting takes some space...
#define ESCAPE_BUFFER_SIZE 200
struct escape_codes_struct {
	uint16_t argv[ESCAPE_ARGS_LIMIT];
	uint8_t argc;

	uint8_t buffer[ESCAPE_BUFFER_SIZE];
	uint8_t *current;

	// XXX here?
	// struct {
	// 	line_t top;
	// 	line_t bottom;
	// } scroll_region;
};

// no need for that I guess?
extern struct escape_codes_struct escape_codes; // TODO put into console?


extern uint8_t console_escape_code;

void escape_codes_init(void);
void escape_code_handle(uint8_t next_char);
void escape_code_reset(void);
void parse_agruments(uint8_t *start, uint16_t default_value);
void handle_escape_code_open_csi(uint8_t next_char);


#define IS_ARG_CHAR(v) (IS_DIGIT(v) || v == ';')
#define IS_DIGIT(v) ((v) >= '0' && (v) <= '9')

#define POW10(v) \
	((v == 0)? 1: \
	((v == 1)? 10: \
	((v == 2)? 100: \
	((v == 3)? 1000: \
	((v == 4)? 10000: \
	0)))))


#endif
