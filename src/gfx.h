/*
 * A simple port of the AdaFruit minimal graphics code to my
 * demo code.
 */
#ifndef _GFX_H
#define _GFX_H
#include <stdint.h>

// #define color_t uint16_t
#define color_t uint8_t

#define swap(a, b) { int16_t t = a; a = b; b = t; }

void gfx_drawPixel(int x, int y, color_t color);
void gfx_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		  color_t color);
void gfx_drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color);
void gfx_drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color);
void gfx_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color);
void gfx_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color);
void gfx_fillScreen(color_t color);

void gfx_drawCircle(int16_t x0, int16_t y0, int16_t r, color_t color);
void gfx_drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
			  uint8_t cornername, color_t color);
void gfx_fillCircle(int16_t x0, int16_t y0, int16_t r, color_t color);
// void gfx_init(void (*draw)(int, int, color_t), int, int);
void gfx_init(void (*draw)(uint16_t, uint16_t, color_t), uint16_t, uint16_t);

void gfx_fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
			  uint8_t cornername, int16_t delta, color_t color);
void gfx_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		      int16_t x2, int16_t y2, color_t color);
void gfx_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		      int16_t x2, int16_t y2, color_t color);
void gfx_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
		       int16_t radius, color_t color);
void gfx_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
		       int16_t radius, color_t color);
void gfx_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
		    int16_t w, int16_t h, color_t color);
void gfx_drawChar(int16_t x, int16_t y, unsigned char c, color_t color,
		  color_t bg, uint8_t size);
void gfx_setCursor(int16_t x, int16_t y);
void gfx_setTextColor(color_t c, color_t bg);
void gfx_setTextSize(uint8_t s);
void gfx_setTextWrap(uint8_t w);
void gfx_setRotation(uint8_t r);
void gfx_update_screen(void);
void gfx_puts(char *);
void gfx_write(uint8_t);

uint16_t gfx_height(void);
uint16_t gfx_width(void);

uint8_t gfx_getRotation(void);

// XXX
// #define GFX_WIDTH   400
// #define GFX_HEIGHT  160*5
#define GFX_WIDTH   (LCD_WIDTH*8)
#define GFX_HEIGHT  (LCD_HEIGHT*(SCREEN_LINES/FONT_CHAR_HEIGHT))

struct gfx_state {
	int16_t _width, _height, cursor_x, cursor_y;
	color_t textcolor, textbgcolor;
	uint8_t textsize, rotation;
	uint8_t wrap;
	// void (*drawpixel)(int, int, color_t);
	void (*drawpixel)(uint16_t, uint16_t, color_t);
};

extern struct gfx_state __gfx_state;

#define GFX_COLOR_WHITE          0
#define GFX_COLOR_BLACK          1
// #define GFX_COLOR_WHITE          0xFFFF
// #define GFX_COLOR_BLACK          0x0000
// #define GFX_COLOR_GREY           0xF7DE
// #define GFX_COLOR_BLUE           0x001F
// #define GFX_COLOR_BLUE2          0x051F
// #define GFX_COLOR_RED            0xF800
// #define GFX_COLOR_MAGENTA        0xF81F
// #define GFX_COLOR_GREEN          0x07E0
// #define GFX_COLOR_CYAN           0x7FFF
// #define GFX_COLOR_YELLOW         0xFFE0

#endif /* _ADAFRUIT_GFX_H */
