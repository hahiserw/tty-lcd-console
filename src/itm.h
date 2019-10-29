#ifndef _ITM_H
#define _ITM_H

#include "config.h"
#include <libopencm3/stm32/gpio.h>
#include <stdint.h>

// #define LAME_RENDER


// values for 1 byte (2 nibbles)
#define LCD_WIDTH  50
#define LCD_HEIGHT 160

#define LCD_WHITE 0
#define LCD_BLACK 1

#define FIRST_NIBBLE(d)  ((d) >> 4)
#define SECOND_NIBBLE(d) ((d) & 0x0f)

// #define DELAY __asm__("nop") // if wires to display are short
#define DELAY __asm__("nop\nnop\nnop") // if your wires are shitty
// #define DELAY ;


#define M_PI 3.14159265358979323846


// #define lcd_pixel(data, x, y) data[(LCD_HEIGHT - 1 - (y)) * LCD_WIDTH + ((x) >> 3)] |= 1 << ((x) & 7)
#define lcd_pixel(data, x, y) data[(y) * LCD_WIDTH + ((x) >> 3)] |= 1 << (7 - ((x) & 7))
// #define lcd_pixel(data, x, y) data[(LCD_HEIGHT - 1 - (y)) * LCD_WIDTH/2 + ((x) >> 3)] |= 1 << ((x) & 7) // for gif

extern uint8_t screen[LCD_WIDTH * LCD_HEIGHT];

void lcd_draw_pixel(upos_t x, upos_t y, color_t color);
uint8_t lcd_get_pixel(upos_t x, upos_t y);

void lcd_init(void);
// #ifdef LAME_RENDER
// void lcd_loop(void);
// #else
// void lcd_line(void);
// #endif
void lcd_line(void);

void lcd_clear(uint8_t *data);
void lcd_clear_horizontal_line(uint8_t *data, uint8_t x);

void lcd_circle(uint8_t *data, uint32_t r, uint32_t x0, uint32_t y0);
void lcd_draw(uint8_t *data);

void lcd_write_char(uint8_t *data, int16_t x0, int16_t y0, unsigned char c);
void lcd_write_text(uint8_t *data, int16_t x0, int16_t y0, char *text);


#endif
