#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "config.h"
#include "itm.h"

// should be somewhere else?
#ifdef FONT_SMALL
#include "font-4x6.c"
#else
#include "font-7x12.c"
#endif



// #define BUFFER_LINES 11
// #define BUFFER_LINES 16
#define BUFFER_LINES 30

#define SCREEN_COLUMNS ((LCD_WIDTH*8)  / FONT_CHAR_WIDTH)
#define SCREEN_LINES   (LCD_HEIGHT / FONT_CHAR_HEIGHT)
// #define SCREEN_COLUMNS 50 // (LCD_WIDTH) (LCD_WIDTH / FONT_CHAR_WIDTH) // LCD_WIDTH is actually in bytes...
// #define SCREEN_LINES   13
// #define SCREEN_COLUMNS 100 // (LCD_WIDTH) (LCD_WIDTH / FONT_CHAR_WIDTH) // LCD_WIDTH is actually in bytes...
// #define SCREEN_LINES   26 // XXX compute
#define MAX_COLUMN     SCREEN_COLUMNS
#define MAX_LINE       SCREEN_LINES

#define LINE_HEIGHT_SIZE (LCD_WIDTH * FONT_CHAR_HEIGHT)

#define CURRENT_LINE   (current_view->screen_y / FONT_CHAR_HEIGHT)
#define CURRENT_COLUMN (current_view->screen_x / FONT_CHAR_WIDTH)
#define CURRENT_LINE_VISIBLE ((current_view->screen_y - (current_view->screen_start - current_view->screen_buffer)) / FONT_CHAR_HEIGHT)

// screen buffer size
// about 40KB, fits in STM32F4
// #define SCREEN_BUFFER_SIZE LCD_WIDTH * LCD_HEIGHT * 5
// #define SCREEN_BUFFER_SIZE LCD_WIDTH * FONT_CHAR_HEIGHT * 70 // 70 lines
#define SCREEN_BUFFER_SIZE (LCD_WIDTH * FONT_CHAR_HEIGHT * BUFFER_LINES)
#define MARKS_BUFFER_SIZE SCREEN_BUFFER_SIZE / LCD_WIDTH / FONT_CHAR_HEIGHT // buffer lines


// #define PTR_BUFFER_LINE(v) (current_view->screen_buffer + (v) * LINE_HEIGHT_SIZE)
// #define PTR_SCREEN_LINE(v) (current_view->screen_start + (v) * LINE_HEIGHT_SIZE)
#define PTR_BUFFER_LINE_FROM_SCREEN(v) \
	(uint8_t *)(((current_view->screen_start - current_view->screen_buffer \
				  + (v) * LINE_HEIGHT_SIZE) % SCREEN_BUFFER_SIZE) + current_view->screen_buffer)


#define CONSOLE_VIEWS 2

extern uint8_t console_special_character;

struct view_struct {
	uint8_t screen_buffer[SCREEN_BUFFER_SIZE];
	uint8_t *screen_start;
	column_t  clear_marks[MARKS_BUFFER_SIZE];

	uint16_t screen_x;
	uint16_t screen_y;

	uint16_t last_cursor_x;
	uint16_t last_cursor_y;

	uint16_t saved_cursor_x;
	uint16_t saved_cursor_y;

	// flags
	uint8_t screen_continue; // :1
	// uint8_t console_dirty;
// } view = {
// 	0,
// 	0,
// 	0,
//
// 	0,
// 	0,
//
// 	0,
// 	0,
//
// 	0,
// 	0,
//
// 	0
};

struct console_struct {
	struct view_struct view[CONSOLE_VIEWS];

	// flags
	// uint8_t bell;     // :1
	uint8_t reverse;     // :1
	uint8_t show_cursor; // :1
	uint8_t wrap_around; // :1

	uint8_t current_mode;

	uint8_t character_set;

	uint8_t text_mode;
	uint8_t foreground_color;
	uint8_t background_color;

	uint16_t image_width;
	uint16_t image_height;
	uint16_t image_receive;

	line_t scroll_region_top;
	line_t scroll_region_bottom;
};


extern struct console_struct console;
extern struct view_struct *current_view;

void console_color_reset(void);
void console_init(void);
void console_reset(void);
void screen_scroll(uint16_t height_offset);
void console_update(uint8_t c);
void console_work(void);

void console_append_new_lines(uint8_t count);
void console_new_line(uint8_t return_cursor);
void console_char_move_cursor(void);
void console_char(uint8_t s);
void console_puts(uint8_t *s);
void console_write(uint8_t c);

uline_t console_get_column(void);
uline_t console_get_line(void);
void console_set_column(line_t c);
void console_set_line(line_t l);
void console_region_scroll_lines(uline_t top, uline_t bottom, line_t lines);
void console_move_chars_in_line(uline_t line, ucolumn_t start_column,
								column_t count);

void console_puts_debug(char *s, uint32_t v); // debug


#define CONSOLE_TEXT_EFFECT_DEFAULT     0
#define CONSOLE_TEXT_EFFECT_BOLD        1
#define CONSOLE_TEXT_EFFECT_ITALIC      2
#define CONSOLE_TEXT_EFFECT_UNDERLINE   4
#define CONSOLE_TEXT_EFFECT_REVERSE     8
#define CONSOLE_TEXT_EFFECT_CROSSED_OUT 16

#define CONSOLE_MODE_TEXT        0
#define CONSOLE_MODE_ESCAPE_CODE 1
#define CONSOLE_MODE_IMAGE       2
// #define CONSOLE_MODE_TITLE       2

#define CHARACTER_DEFAULT_SET 0
#define CHARACTER_SPECIAL_SET 1


#endif
