// #include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <stdint.h>
// #include <math.h>
#include "escape_codes.h"
#include "console.h"
#include "gfx.h"
#include "itm.h"
#include <string.h>



struct console_struct console;
struct view_struct *current_view;


// XXX should work on columns and lines instead of pixels
// some abstraction layer needed here!
// functions to convert?

// black box
// > char
// < data in buffer
// parse character: printable, non printable / escape sequence
// - write another character (if last char in line go to another one / if last is \n skip)
// - move cursor / clear screen




void console_color_reset(void) {
	console.foreground_color = LCD_BLACK;
	console.background_color = LCD_WHITE;

	console.text_mode   = 0;
}

void console_reset() { // XXX what should it do exactly?
	console_color_reset();

	console.wrap_around = 1;
	console.show_cursor = 1;

	console.character_set = CHARACTER_DEFAULT_SET;

	console.current_mode = CONSOLE_MODE_TEXT;

	escape_codes_init();
}

void console_init(void) {
	for (uint8_t i = 0; i < CONSOLE_VIEWS; i++) {
		struct view_struct *v = &console.view[i];

		// v->screen_buffer[SCREEN_BUFFER_SIZE] = {0}; // make it work like that
		memset(v->screen_buffer, 0, SCREEN_BUFFER_SIZE);

		v->screen_start = v->screen_buffer;

		v->screen_x = 0;
		v->screen_y = 0;

		v->last_cursor_x = 0;
		v->last_cursor_y = 0;

		v->saved_cursor_x = 0;
		v->saved_cursor_y = 0;

		v->screen_continue = 0; // :1

		for (uint8_t y = 0; y < MARKS_BUFFER_SIZE; y++)
			v->clear_marks[y] = MAX_COLUMN;
	}

	// flags
	// console.bell          = 0; // :1
	console.reverse          = 0; // :1
	console.show_cursor      = 1; // :1
	console.text_mode        = 0;
	console.foreground_color = 0;
	console.background_color = 0;

	console.image_width   = 0;
	console.image_height  = 0;
	console.image_receive = 0;

	// don't think it's the right place for it
	console.scroll_region_top    = 0;
	console.scroll_region_bottom = MAX_LINE - 1;

	current_view = &console.view[0];

	console_reset();
	cursor_reverse();
}

void screen_scroll(uint16_t height_offset) {
	if (height_offset >= SCREEN_BUFFER_SIZE - LCD_WIDTH)
		return;

	current_view->screen_start = (uint8_t *)current_view->screen_buffer + height_offset * LCD_WIDTH;
}

void console_update(uint8_t c) {
	// switch (c)?
	// show title? first row with inverted colors :D
	console_write(c);
}

void console_work(void) {
	// clear screen where needed when not busy
	// get parts marked for erasal
	// erase it
	// 650/8 = 81.25 bytes for marker table
	// it's now characterwise, should it be linewise? clear one
	// rectangle instead of one for each character?
	// for (uint16_t i = 0; i < SCREEN_COLUMNS * SCREEN_LINES; i++) {
	// 	if (MARKER_TABLE(i) & MARKER_BYTE(i)) {
	// 		gfx_fillRect((i % SCREEN_COLUMNS) * FONT_CHAR_WIDTH, i / SCREEN_COLUMNS, FONT_CHAR_WIDTH,
	// 					 FONT_CHAR_HEIGHT, console_foreground_color); // or background color?
	// 		// erased, remove mark for this character
	// 		MARKER_TABLE(i) &= ~MARKER_BYTE(i);
	// 	}
	// }

	// if (!console_dirty)
	// 	return;

	// marker buffer holds the column number of \n
	for (uint8_t line = 0; line < MARKS_BUFFER_SIZE; line++) {
		if (current_view->clear_marks[line] >= 0 && current_view->clear_marks[line] < MAX_COLUMN) {
			gfx_fillRect(current_view->clear_marks[line] * FONT_CHAR_WIDTH, line * FONT_CHAR_HEIGHT,
						 LCD_WIDTH*8 - current_view->clear_marks[line] * FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT,
						 console.background_color);

			current_view->clear_marks[line] = MAX_COLUMN;
		}

		if (current_view->clear_marks[line] < 0 && -current_view->clear_marks[line] < MAX_COLUMN) {
			gfx_fillRect(0, line * FONT_CHAR_HEIGHT,
						 -current_view->clear_marks[line] * FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT,
						 console.background_color);

			current_view->clear_marks[line] = MAX_COLUMN;
		}
	}

	if (current_view->screen_x != current_view->last_cursor_x || current_view->screen_y != current_view->last_cursor_y)
		cursor_position_update();

	// console_dirty = 0;
}

void console_append_new_lines(uint8_t count) {
	for (uint8_t i = 0; i < count; i++) {
		console_new_line(1);
	}
}

// XXX breaks sometimes (when screen_continue?)
// http://cvs.schmorp.de/rxvt-unicode/src/screen.C?revision=1.457&view=markup
void console_new_line(uint8_t return_cursor)
{
	if (return_cursor)
		console_set_column(0);

	current_view->screen_y += FONT_CHAR_HEIGHT;

	// if (CURRENT_LINE >= SCREEN_LINES)
	if (console_get_line() >= SCREEN_LINES)
		current_view->screen_continue = 1;

	if (CURRENT_LINE >= BUFFER_LINES)
		current_view->screen_y = 0;

	// && cursor_visible
	if (current_view->screen_continue) {
		// change 0 to number of characters in this line? would be faster
		// clear the next line in the buffer
		current_view->clear_marks[(CURRENT_LINE + 1) % BUFFER_LINES] = 0;
		// console_dirty = 1;
		console_work();

		// debug2(" \r\nclear", (CURRENT_LINE + 1) % BUFFER_LINES);

		if (CURRENT_LINE < SCREEN_LINES) {
			current_view->screen_start = current_view->screen_buffer + LINE_HEIGHT_SIZE *
				(BUFFER_LINES + CURRENT_LINE - (SCREEN_LINES - 1));
		} else {
			current_view->screen_start = current_view->screen_buffer + LINE_HEIGHT_SIZE *
				(CURRENT_LINE - (SCREEN_LINES - 1));
		}
	}
}



uline_t console_get_column(void) {
	return current_view->screen_x / FONT_CHAR_WIDTH;
}

// XXX seems broken in some cases?
#define SCREEN_START_LINE \
	((current_view->screen_start - current_view->screen_buffer) \
	 / (LCD_WIDTH * FONT_CHAR_HEIGHT))
#define CONSOLE_CURRENT_LINE (current_view->screen_y / FONT_CHAR_HEIGHT)
uline_t console_get_line(void) {
	return (BUFFER_LINES - SCREEN_START_LINE + CONSOLE_CURRENT_LINE) % BUFFER_LINES;
}

void console_set_column(line_t c) {
	if (c < 0)
		c = 0;

	if (c >= MAX_COLUMN)
		c = MAX_COLUMN - 1;

	current_view->screen_x = c * FONT_CHAR_WIDTH;
}

void console_set_line(line_t l) {
	if (l < 0)
		l = 0;

	if (l >= MAX_LINE)
		l = MAX_LINE - 1;

	current_view->screen_y = ((SCREEN_START_LINE + l) % BUFFER_LINES) * FONT_CHAR_HEIGHT;

	current_view->screen_continue = l == MAX_LINE - 1;
	// if (l == MAX_LINE - 1)
	// 	current_view->screen_continue = 1;
}

// todo
// void console_set_column_relative(int8_t c) {
// 	if (c < 0)
// 		c = 0;
//
// 	if (c >= MAX_COLUMN)
// 		c = MAX_COLUMN - 1;
//
// 	current_view->screen_x = c * FONT_CHAR_WIDTH;
// }

/*
 * @description move characters in line
 * @param count  by given number of characters based on signess to the
 * left/right
 */
void console_move_chars_in_line(uline_t line, ucolumn_t start_column,
								column_t count)
{
	if (!count)
		return;

	pos_t column_s;
	pos_t column_mod   = 0;
	upos_t column_src  = 0;
	upos_t column_dest = 0;

#define PIXEL_COUNT ((MAX_COLUMN - start_column - count) * FONT_CHAR_WIDTH)

	if (count > 0) {
		column_mod  = PIXEL_COUNT - 1;
		column_s    = -1;
		column_dest = count;
	} else {
		count = -count;

		column_s    = 1;
		column_src  = count;
	}

	// xxx check me
	if (count + start_column > MAX_COLUMN)
		count = MAX_COLUMN - start_column;

	// XXX one seems to bee broken
#define X(x, v) (column_mod + x * column_s + (start_column + v) * FONT_CHAR_WIDTH)
#define Y(y) (y + line * FONT_CHAR_HEIGHT)

	// FIXME wtf, also changes some buffer fixed line
	// looks like it also changes line at an absolute position
	// seems it breakes because of console_get_line

	// static uint8_t lolo = 1;
// if (lolo)
	for (upos_t y = 0; y < FONT_CHAR_HEIGHT; y++) {
		for (upos_t x = 0; x < PIXEL_COUNT; x++)
			lcd_draw_pixel(X(x, column_dest), Y(y),
						   lcd_get_pixel(X(x, column_src), Y(y)));
		// break;
	}
	// lolo = 0;
	// or just copy memory but remember about character padding, right?

	upos_t clear_x;

	// xxx check me
	if (column_s < 0)
		clear_x = current_view->screen_x;
	else
		clear_x = (MAX_COLUMN - count) * FONT_CHAR_WIDTH;

	// erase
	gfx_fillRect(clear_x, current_view->screen_y,
				 count * FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT,
				 console.background_color);
}

// top = 1, bottom = 5, count = 3
// 0  zero  =>  zero
// 1  one   =>  four
// 2  two   =>  five
// 3  three =>  .
// 4  four  =>  .
// 5  five  =>  .
// 6  six   =>  six

// top = 1, bottom = 5, count = -3 | top = 2, bottom = 6, count = -3
// 0  zero  =>  zero                   zero  =>  zero
// 1  one   =>  .      |               one   =>  one
// 2  two   =>  .      |               two   =>  .      |
// 3  three =>  .      |               three =>  .      |
// 4  four  =>  one    |               four  =>  .      |
// 5  five  =>  two    |               five  =>  two    |
// 6  six   =>  six                    six   =>  three  |

/*
 * @description move lines in given range up or down depending on count
 * signess by specified number of lines
 * @param top    region start
 * @param bottom region end
 * @param count  direction and number of lines to move
 */
void console_region_scroll_lines(uline_t top, uline_t bottom, line_t count) {
	if (top >= bottom || !count)
		return;

	uline_t line_src  = top;
	uline_t line_dest = top;
	uline_t line_count;
	uline_t line_erase;

	line_t line_mod = 0;
	line_t line_s;

#define LINE_COUNT (bottom - top - count + 1)

	if (count > 0) {
		line_src  += count;
		line_erase = bottom - count + 1;
		line_s = 1;
	} else {
		count = -count; // to not use abs below

		line_dest += count;
		line_erase = top;

		line_mod = LINE_COUNT - 1;
		line_s = -1;
	}

	line_count = LINE_COUNT;

	// will break if at the end of screen_buffer
	// memmove(PTR_SCREEN_LINE(line_dest),
	// 		PTR_SCREEN_LINE(line_src),
	// 		LINE_HEIGHT_SIZE * (line_count));


	for (uline_t l = 0; l < line_count; l++)
		memcpy(PTR_BUFFER_LINE_FROM_SCREEN(line_dest + l * line_s + line_mod),
			   PTR_BUFFER_LINE_FROM_SCREEN(line_src  + l * line_s + line_mod),
			   LINE_HEIGHT_SIZE);

	// erasing lines - it might be just zero out buffer memory (it's not characterwise)
	// memset(PTR_SCREEN_LINE(line_erase), 0, LINE_HEIGHT_SIZE * count);

	for (uline_t l = 0; l < count; l++)
		memset(PTR_BUFFER_LINE_FROM_SCREEN(line_erase + l), 0,
			   LINE_HEIGHT_SIZE);
}

uint8_t console_line_overflow = 0;
uint8_t console_special_character = 0;


// move to console_image.h
#define IMAGE_BITS_PER_CHARACTER 6 // base64

// use sixels?
void console_write(uint8_t c)
{
	// 6 pixels at a time
	// XXX move to console_image.c
	if (console.image_width) {
		// xxx make it into a fuction
		uint8_t v = 0; // '=' or wrong char
		if (c >= 'A' && c <= 'Z')
			v = c - 'A';
		else if (c >= 'a' && c <= 'z')
			v = 26 + c - 'a';
		else if (c >= '0' && c <= '9')
			v = 52 + c - '0';
		else if (c == '+' || c == '-')
			v = 62;
		else if (c == '/' || c == '_')
			v = 63;
		else if (c == '=') // XXX is being shown on the screen, exit condition wrong?
			;
		else {
			// break
			console.image_width = 0;
			// return;
		}

		for (uint8_t p = 0; p < IMAGE_BITS_PER_CHARACTER; p++) {
			// now pixels are drawn and erased
			// maybe only black should be drawn?
			gfx_drawPixel(
						 (current_view->screen_x + ((console.image_receive + IMAGE_BITS_PER_CHARACTER - p - 1) % console.image_width)),
						 ((current_view->screen_y + (console.image_receive + IMAGE_BITS_PER_CHARACTER - p - 1) / console.image_width)) % (SCREEN_BUFFER_SIZE/LCD_WIDTH), // buffer wrap
						 (v & (1 << p))? console.foreground_color: console.background_color);
		}

		console.image_receive += IMAGE_BITS_PER_CHARACTER;

		// xxx receive should be padded to IMAGE_BITS_PER_CHARACTER so it catch all '='
		if (console.image_width == 0 || console.image_receive >= console.image_width * console.image_height) {
			// xxx or check only height? or only width? or make it configurable?
			if (console.image_width <= FONT_CHAR_WIDTH && console.image_height <= FONT_CHAR_HEIGHT) {
				console_char_move_cursor();
			} else {
				// same as ceil((float)image_height / FONT_CHAR_HEIGHT)
				uint8_t image_lines = ((console.image_height - 1) / FONT_CHAR_HEIGHT + 1);
				console_append_new_lines(image_lines);
			}

			console.image_width  = 0;
			console.image_height = 0;

			// cursor
			cursor_position_update();
			cursor_reverse();
		}

		return;
	}

	if (c == 0x1b) {
		console.current_mode = CONSOLE_MODE_ESCAPE_CODE;
		return;
	}

	if (console.current_mode == CONSOLE_MODE_ESCAPE_CODE) {
		escape_code_handle(c);
		return;
	}


	// FIXME should have 4 bytes buffer, 'cause it's utf-8, right?
	if (c > 0x7f && console_special_character == 0) {
		console_special_character = c;
		return;
	}


	// xxx no need for that if writing another char
	cursor_reverse();

	switch (c) {
	// case '': // ^E return terminal status?
	case '\a': // ^G bell
		// right? can't be in any escape sequence? (except as a terminator for OSC)
		// if (console.current_mode == CONSOLE_MODE_ESCAPE_CODE)
		// 	break;

		// if not already pending
		if (TIM_SR(FRAME_TIMER_CONST) & TIM_SR_UIF)
			break;

		// console_puts((uint8_t *)"<bell>");
		// console.reverse = ~console.reverse;
		console.reverse = 0xff; // for now?
		timer_enable_counter(FRAME_TIMER_CONST);
		break;

	case '\b': // ^H backspace
	case 0x7f: { // DEL
		line_t column = console_get_column();
		if (column > 0) {
			console_set_column(column - 1);

			if (c == 0x7f)
				gfx_drawChar(current_view->screen_x, current_view->screen_y, ' ',
							 console.foreground_color, console.background_color, 1);
		}
	}
		break;

	case '\t':
		for (uint8_t i = TAB_SIZE - (console_get_column() % TAB_SIZE); i; i--)
			console_char(' ');
		break;

	case '\n':
		// TODO animate scrolling 1 pixel at a time
		if (!console_line_overflow) {
			// TODO change to console_scroll and on default range scroll entire buffer
			if ((console.scroll_region_top != 0
				|| console.scroll_region_bottom != MAX_LINE - 1)
				// && CURRENT_LINE_VISIBLE == console.scroll_region_bottom
				) {
				cursor_reverse();
				console_region_scroll_lines(console.scroll_region_top,
											console.scroll_region_bottom,
											1);
				// cursor_position_update();
				return;
			} else {
				console_new_line(1);
				// console_line_overflow = 0;
			}
		}
		break;

	case '\v': // ^K vertical tab
	case '\f': // ^L behaves the same in urxvt?
		console_new_line(0);
		break;

	case '\r':
		console_set_column(0);
		break;

	// custom stuff
	case 0x12: // ^R
		console.reverse = ~console.reverse;
		break;

	case 0x13: { // ^S (or when char 0x33c3 is written?) 33c3 2016
		// xxx breaks when console_continue?
		// add some random timer? make sure line is not reversed when writing another char on it
		// reverse bits order in all bytes in current line
		uint8_t *r = current_view->screen_start + console_get_line() * LCD_WIDTH * FONT_CHAR_HEIGHT;
		uint8_t *re = current_view->screen_start + (console_get_line() + 1) * LCD_WIDTH * FONT_CHAR_HEIGHT;
		while (r < re) { // 600 times
#ifndef FONT_SMALL
			*r = (*r & 0xF0) >> 4 | (*r & 0x0F) << 4;
#endif
			*r = (*r & 0xCC) >> 2 | (*r & 0x33) << 2;
			*r = (*r & 0xAA) >> 1 | (*r & 0x55) << 1;
			r++;
		}
	}
		break;

	default:
		console_char(c);
	}

	cursor_position_update();
	cursor_reverse();
}

// XXX broken
void console_char_move_cursor() {
	current_view->screen_x += FONT_CHAR_WIDTH;

	if (current_view->screen_x > (LCD_WIDTH*8 - FONT_CHAR_WIDTH)) {
		if (console.wrap_around) {
			console_new_line(1);
		}
		else
			console_set_column(0);
		// console_line_overflow = 1;
	} else {
		console_line_overflow = 0;
	}
}

void console_char(uint8_t c)
{
	gfx_drawChar(current_view->screen_x, current_view->screen_y, c, console.foreground_color,
				 console.background_color, 1); // size doensn't work

	if (console_special_character) {
		console_special_character = 0;
	}

	console_char_move_cursor();
}

void console_puts(uint8_t *s)
{
	while (*s) {
		console_write(*s);
		s++;
	}
}

void console_puts_debug(char *s, uint32_t v)
{
	uint8_t previous_mode = console.current_mode;
	console.current_mode = CONSOLE_MODE_TEXT;

	uint8_t lol[6];
	lol[0] = '0' + ((v / 10000) % 10);
	lol[1] = '0' + ((v / 1000) % 10);
	lol[2] = '0' + ((v / 100) % 10);
	lol[3] = '0' + ((v / 10) % 10);
	lol[4] = '0' + ((v / 1) % 10);
	lol[5] = 0;

	console_puts((uint8_t *)"\e[80C");
	console_puts((uint8_t *)s);
	console_puts((uint8_t *)": ");
	console_puts(lol);
	console_puts((uint8_t *)"\e[91D");

	// console.current_mode = CONSOLE_MODE_ESCAPE_CODE;
	console.current_mode = previous_mode;
}
