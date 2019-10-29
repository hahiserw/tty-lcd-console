/*
 * This is the core graphics library for all our displays, providing a common
 * set of graphics primitives (points, lines, circles, etc.).  It needs to be
 * paired with a hardware-specific library for each display device we carry
 * (to handle the lower-level functions).
 *
 * Adafruit invests time and resources providing this open source code, please
 * support Adafruit & open-source hardware by purchasing products from Adafruit!
 *
 * Copyright (c) 2013 Adafruit Industries.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Modified the AdaFruit library to be a C library, changed the font and
 * generally munged it in a variety of ways, creating a reasonably quick
 * and dirty way to put something "interesting" on the LCD display.
 * --Chuck McManis (2013, 2014)
 *
 */

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include "gfx.h"

#include "config.h"

#ifdef FONT_SMALL
#include "font-4x6.c"
#else
#include "font-7x12.c"
#endif

#include "console.h"

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

struct gfx_state __gfx_state;

void
gfx_drawPixel(int x, int y, color_t color)
{
	if ((x < 0) || (x >= __gfx_state._width) ||
	    (y < 0) || (y >= __gfx_state._height)) {
		return; /* off screen so don't draw it */
	}
	(__gfx_state.drawpixel)(x, y, color);
}
#define true 1

void
// gfx_init(void (*pixel_func)(int, int, color_t), int width, int height)
gfx_init(void (*pixel_func)(uint16_t, uint16_t, color_t), uint16_t width, uint16_t height)
{
	__gfx_state._width    = width;
	__gfx_state._height   = height;
	__gfx_state.rotation  = 0;
	__gfx_state.cursor_y  = __gfx_state.cursor_x    = 0;
	__gfx_state.textsize  = 1;
	__gfx_state.textcolor = GFX_COLOR_BLACK;
	__gfx_state.textbgcolor = GFX_COLOR_WHITE;
	__gfx_state.wrap      = true;
	__gfx_state.drawpixel = pixel_func;
}

/* Draw a circle outline */
void gfx_drawCircle(int16_t x0, int16_t y0, int16_t r,
		    color_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	gfx_drawPixel(x0  , y0+r, color);
	gfx_drawPixel(x0  , y0-r, color);
	gfx_drawPixel(x0+r, y0  , color);
	gfx_drawPixel(x0-r, y0  , color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		gfx_drawPixel(x0 + x, y0 + y, color);
		gfx_drawPixel(x0 - x, y0 + y, color);
		gfx_drawPixel(x0 + x, y0 - y, color);
		gfx_drawPixel(x0 - x, y0 - y, color);
		gfx_drawPixel(x0 + y, y0 + x, color);
		gfx_drawPixel(x0 - y, y0 + x, color);
		gfx_drawPixel(x0 + y, y0 - x, color);
		gfx_drawPixel(x0 - y, y0 - x, color);
	}
}

void gfx_drawCircleHelper(int16_t x0, int16_t y0,
			  int16_t r, uint8_t cornername, color_t color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;
		if (cornername & 0x4) {
			gfx_drawPixel(x0 + x, y0 + y, color);
			gfx_drawPixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2) {
			gfx_drawPixel(x0 + x, y0 - y, color);
			gfx_drawPixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8) {
			gfx_drawPixel(x0 - y, y0 + x, color);
			gfx_drawPixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1) {
			gfx_drawPixel(x0 - y, y0 - x, color);
			gfx_drawPixel(x0 - x, y0 - y, color);
		}
	}
}

void gfx_fillCircle(int16_t x0, int16_t y0, int16_t r,
		    color_t color)
{
	gfx_drawFastVLine(x0, y0 - r, 2*r+1, color);
	gfx_fillCircleHelper(x0, y0, r, 3, 0, color);
}

/* Used to do circles and roundrects */
void gfx_fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
			  uint8_t cornername, int16_t delta, color_t color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;

		if (cornername & 0x1) {
			gfx_drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
			gfx_drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
		}
		if (cornername & 0x2) {
			gfx_drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
			gfx_drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
		}
	}
}

/* Bresenham's algorithm - thx wikpedia */
void gfx_drawLine(int16_t x0, int16_t y0,
			    int16_t x1, int16_t y1,
			    color_t color)
{
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	for (; x0 <= x1; x0++) {
		if (steep) {
			gfx_drawPixel(y0, x0, color);
		} else {
			gfx_drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}

/* Draw a rectangle */
void gfx_drawRect(int16_t x, int16_t y,
		  int16_t w, int16_t h,
		  color_t color)
{
	gfx_drawFastHLine(x, y, w, color);
	gfx_drawFastHLine(x, y + h - 1, w, color);
	gfx_drawFastVLine(x, y, h, color);
	gfx_drawFastVLine(x + w - 1, y, h, color);
}

void gfx_drawFastVLine(int16_t x, int16_t y,
		       int16_t h, color_t color)
{
	/* Update in subclasses if desired! */
	gfx_drawLine(x, y, x, y + h - 1, color);
}

void gfx_drawFastHLine(int16_t x, int16_t y,
		       int16_t w, color_t color)
{
	/* Update in subclasses if desired! */
	gfx_drawLine(x, y, x + w - 1, y, color);
}

void gfx_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
		  color_t color)
{
	/* Update in subclasses if desired! */
	int16_t i;
	for (i = x; i < x + w; i++) {
		gfx_drawFastVLine(i, y, h, color);
	}
}

void gfx_fillScreen(color_t color)
{
	gfx_fillRect(0, 0, __gfx_state._width, __gfx_state._height, color);
}

/* Draw a rounded rectangle */
void gfx_drawRoundRect(int16_t x, int16_t y, int16_t w,
		       int16_t h, int16_t r, color_t color)
{
	/* smarter version */
	gfx_drawFastHLine(x + r    , y        , w - 2 * r, color); /* Top */
	gfx_drawFastHLine(x + r    , y + h - 1, w - 2 * r, color); /* Bottom */
	gfx_drawFastVLine(x        , y + r    , h - 2 * r, color); /* Left */
	gfx_drawFastVLine(x + w - 1, y + r    , h - 2 * r, color); /* Right */
	/* draw four corners */
	gfx_drawCircleHelper(x + r        , y + r        , r, 1, color);
	gfx_drawCircleHelper(x + w - r - 1, y + r        , r, 2, color);
	gfx_drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	gfx_drawCircleHelper(x + r        , y + h - r - 1, r, 8, color);
}

/* Fill a rounded rectangle */
void gfx_fillRoundRect(int16_t x, int16_t y, int16_t w,
		       int16_t h, int16_t r, color_t color) {
	/* smarter version */
	gfx_fillRect(x + r, y, w - 2 * r, h, color);

	/* draw four corners */
	gfx_fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	gfx_fillCircleHelper(x + r        , y + r, r, 2, h - 2 * r - 1, color);
}

/* Draw a triangle */
void gfx_drawTriangle(int16_t x0, int16_t y0,
		      int16_t x1, int16_t y1,
		      int16_t x2, int16_t y2, color_t color)
{
	gfx_drawLine(x0, y0, x1, y1, color);
	gfx_drawLine(x1, y1, x2, y2, color);
	gfx_drawLine(x2, y2, x0, y0, color);
}

/* Fill a triangle */
void gfx_fillTriangle(int16_t x0, int16_t y0,
		      int16_t x1, int16_t y1,
		      int16_t x2, int16_t y2, color_t color)
{
	int16_t a, b, y, last;

	/* Sort coordinates by Y order (y2 >= y1 >= y0) */
	if (y0 > y1) {
		swap(y0, y1); swap(x0, x1);
	}
	if (y1 > y2) {
		swap(y2, y1); swap(x2, x1);
	}
	if (y0 > y1) {
		swap(y0, y1); swap(x0, x1);
	}

	/* Handle awkward all-on-same-line case as its own thing */
	if (y0 == y2) {
		a = b = x0;
		if (x1 < a) {
			a = x1;
		} else if (x1 > b) {
			b = x1;
		}
		if (x2 < a) {
			a = x2;
		} else if (x2 > b) {
			b = x2;
		}
		gfx_drawFastHLine(a, y0, b - a + 1, color);
		return;
	}

	int16_t
	dx01 = x1 - x0,
	dy01 = y1 - y0,
	dx02 = x2 - x0,
	dy02 = y2 - y0,
	dx12 = x2 - x1,
	dy12 = y2 - y1,
	sa   = 0,
	sb   = 0;

	/* For upper part of triangle, find scanline crossings for segments
	 * 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	 * is included here (and second loop will be skipped, avoiding a /0
	 * error there), otherwise scanline y1 is skipped here and handled
	 * in the second loop...which also avoids a /0 error here if y0=y1
	 * (flat-topped triangle).
	 */
	if (y1 == y2) {
		last = y1;   /* Include y1 scanline */
	} else {
		last = y1 - 1; /* Skip it */
	}

	for (y = y0; y <= last; y++) {
		a   = x0 + sa / dy01;
		b   = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		/* longhand:
		   a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
		   b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		   */
		if (a > b) {
			swap(a, b);
		}
		gfx_drawFastHLine(a, y, b - a + 1, color);
	}

	/* For lower part of triangle, find scanline crossings for segments
	 * 0-2 and 1-2.  This loop is skipped if y1=y2.
	 */
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++) {
		a   = x1 + sa / dy12;
		b   = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		/* longhand:
		   a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
		   b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		   */
		if (a > b) {
			swap(a, b);
		}
		gfx_drawFastHLine(a, y, b - a + 1, color);
	}
}

void gfx_drawBitmap(int16_t x, int16_t y,
		    const uint8_t *bitmap, int16_t w, int16_t h,
		    color_t color)
{
	int16_t i, j, byteWidth = (w + 7) / 8;

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			if (pgm_read_byte(bitmap + j * byteWidth + i / 8) &
					 (128 >> (i & 7))) {
				gfx_drawPixel(x + i, y + j, color);
			}
		}
	}
}

void gfx_write(uint8_t c)
{
	if (c == '\n') {
		__gfx_state.cursor_y += __gfx_state.textsize * 12;
		__gfx_state.cursor_x  = 0;
	} else if (c == '\r') {
		/* skip em */
	} else {
		gfx_drawChar(__gfx_state.cursor_x, __gfx_state.cursor_y,
			     c, __gfx_state.textcolor, __gfx_state.textbgcolor,
			     __gfx_state.textsize);
		__gfx_state.cursor_x += __gfx_state.textsize * 8;
		if (__gfx_state.wrap &&
		    (__gfx_state.cursor_x > (__gfx_state._width -
									 __gfx_state.textsize*8))) {
			__gfx_state.cursor_y += __gfx_state.textsize * 12;
			__gfx_state.cursor_x  = 0;
		}
	}
}

void gfx_puts(char *s)
{
	while (*s) {
		gfx_write(*s);
		s++;
	}
}

/* Draw a character */
void gfx_drawChar(int16_t x, int16_t y, unsigned char c,
		  color_t color, color_t bg, uint8_t size)
{
	int8_t i, j, line;
	unsigned const char *glyph;

#ifndef FONT_SMALL
	int8_t descender, special_descender;
	unsigned const char *glyph, *special_glyph;

	// I wanted to see polish chars really badly so here's a hack
	uint8_t ci = FONT_SPECIAL_LAST;
#endif

	// utf-8 FIXME make it beautiful
	// 1 byte  0b0xxxxxxx
	// 2 bytes 0b110xxxxx 0b10xxxxxx
	// 3 bytes 0b1110xxxx 0b10xxxxxx 0b10xxxxxx
	// 4 bytes 0b11110xxx 0b10xxxxxx 0b10xxxxxx 0b10xxxxxx
	// char              | data                                                | stripped char
	// ą 0xb1 0b10110001 | 0xc4 0x85 0b11000100 0b10000101  0x205 0b0100000101 | a 0x61 0b01100001
	// ł 0xb3 0b10110011 | 0xc5 0x82 0b11000101 0b10000010  0x142 0b0101000010 | l 0x6c 0b01101010

	// 2 ways of handling fonts:
	// - add special features on base letters
	// - use font's hardcoded glyphs for each character

	if (console_special_character == 0xc4 && c == 0x85) {
		// c = 'a'; ci = FONT_SPECIAL_OGONEK;
		c = 177;
	} else if (console_special_character == 0xc4 && c == 0x87) {
		// c = 'c'; ci = FONT_SPECIAL_ACUTE;
		c = 230;
	} else if (console_special_character == 0xc4 && c == 0x99) {
		// c = 'e'; ci = FONT_SPECIAL_OGONEK;
		c = 234;
	} else if (console_special_character == 0xc5 && c == 0x82) {
		// c = 'l'; ci = FONT_SPECIAL_STROKE;
		c = 179;
	} else if (console_special_character == 0xc5 && c == 0x84) {
		// c = 'n'; ci = FONT_SPECIAL_ACUTE;
		c = 241;
	} else if (console_special_character == 0xc3 && c == 0xb3) {
		// c = 'o'; ci = FONT_SPECIAL_ACUTE;
		c = 243;
	} else if (console_special_character == 0xc5 && c == 0x9b) {
		// c = 's'; ci = FONT_SPECIAL_ACUTE;
		c = 182;
	} else if (console_special_character == 0xc5 && c == 0xba) {
		// c = 'z'; ci = FONT_SPECIAL_ACUTE;
		c = 188;
	} else if (console_special_character == 0xc5 && c == 0xbc) {
		// c = 'z'; ci = FONT_SPECIAL_DOT_ABOVE;
		c = 191;
	} else if (console_special_character == 0xc3 && c == 0xb6) {
		// c = 'o'; ci = FONT_SPECIAL_DIAERESIS;
		c = 246;
	}

	if (console_special_character == 0xc4 && c == 0x84) {
		// c = 'A'; ci = FONT_SPECIAL_OGONEK;
		c = 161;
	} else if (console_special_character == 0xc4 && c == 0x86) {
		// c = 'C'; ci = FONT_SPECIAL_ACUTE;
		c = 198;
	} else if (console_special_character == 0xc4 && c == 0x98) {
		// c = 'E'; ci = FONT_SPECIAL_OGONEK;
		c = 202;
	} else if (console_special_character == 0xc5 && c == 0x81) {
		// c = 'L'; ci = FONT_SPECIAL_STROKE;
		c = 163;
	} else if (console_special_character == 0xc5 && c == 0x83) {
		// c = 'N'; ci = FONT_SPECIAL_ACUTE;
		c = 209;
	} else if (console_special_character == 0xc3 && c == 0x93) {
		// c = 'O'; ci = FONT_SPECIAL_ACUTE;
		c = 211;
	} else if (console_special_character == 0xc5 && c == 0x9a) {
		// c = 'S'; ci = FONT_SPECIAL_ACUTE;
		c = 166;
	} else if (console_special_character == 0xc5 && c == 0xb9) {
		// c = 'Z'; ci = FONT_SPECIAL_ACUTE;
		c = 172;
	} else if (console_special_character == 0xc5 && c == 0xbb) {
		// c = 'Z'; ci = FONT_SPECIAL_DOT_ABOVE;
		c = 175;
	} else if (console_special_character == 0xc3 && c == 0x96) {
		// c = 'O'; ci = FONT_SPECIAL_DIAERESIS;
		c = 214;
	}

#ifdef FONT_SMALL
	// for \e(0
	if (console.character_set == CHARACTER_SPECIAL_SET && c >= '`' && c <= '~')
		c -= '`' - 1;

	glyph = &font4x6[c * 3];
#else
	// fix for 'j'
	if (c == 'j')
		ci = FONT_SPECIAL_DOT_ABOVE;

	glyph = &mcm_font[(c & 0x7f) * 9];
	special_glyph = &mcm_font[' ' * 9];

	if (ci < FONT_SPECIAL_LAST)
		special_glyph = &mcm_font_special[ci * 9];

	descender = (*glyph & 0x80) != 0;
	special_descender = (*special_glyph & 0x80) != 0;
#endif

#ifdef FONT_SMALL
#define FONT_UNDERLINE_Y 6 - 1
#define FONT_CROSSEDOUT_Y 6 - 4
#else
#define FONT_UNDERLINE_Y 12 - 2
#define FONT_CROSSEDOUT_Y 12 - 7
#endif

	for (i = 0; i < FONT_CHAR_HEIGHT; i++) {
		// underline / crossed out
		if ((console.text_mode & CONSOLE_TEXT_EFFECT_UNDERLINE && i == FONT_UNDERLINE_Y)
			|| (console.text_mode & CONSOLE_TEXT_EFFECT_CROSSED_OUT && i == FONT_CROSSEDOUT_Y)) {
			line = 0xff;
		} else {
			line = 0x00; // should be here?
#ifndef FONT_SMALL
			if (descender) {
				if (i > 2) {
					line = *(glyph + (i - 3));
				}
			} else {
				if (i < 9) {
					line = *(glyph + i);
				}
			}
#endif

#ifndef FONT_SMALL
			// special
			if (ci < FONT_SPECIAL_LAST) {
				if (special_descender) {
					if (i > 2) {
						line |= *(special_glyph + (i - 3));
					}
				} else {
					if (i < 9) {
						line |= *(special_glyph + i);
					}
				}
			}

			line &= 0x7f;
#else
			line = *(glyph + i / 2);
			line = i & 1? line << 4: line & 0xf0;
#endif
		}

		for (j = 0; j < FONT_CHAR_WIDTH; j++) {
			if (line & 0x80) {
				if (size == 1) { /* default size */
#ifdef FONT_SMALL
#define PIXEL_X x+j
#define PIXEL_Y y+i
#else
#define PIXEL_X x+j
#define PIXEL_Y y+i
#endif
					gfx_drawPixel(PIXEL_X, PIXEL_Y, color);
					// bold
#ifdef FONT_SMALL
					if (console.text_mode & CONSOLE_TEXT_EFFECT_BOLD && j < FONT_CHAR_WIDTH - 1)
						gfx_drawPixel(PIXEL_X+1, PIXEL_Y, color);
#else
					if (console.text_mode & CONSOLE_TEXT_EFFECT_BOLD && j > 0)
						gfx_drawPixel(PIXEL_X-1, PIXEL_Y, color);
#endif
				} else {  /* big size */
					gfx_fillRect(x+(j*size), y+(i*size),
						     size, size, color);
				}
			} else if (bg != color) {
				if (size == 1) { /* default size */
					gfx_drawPixel(PIXEL_X, PIXEL_Y, bg);
				} else {  /* big size */
					gfx_fillRect(x+j*size, y+i*size,
						     size, size, bg);
				}
			}

			line <<= 1;
		}
	}
}

void gfx_setCursor(int16_t x, int16_t y)
{
	__gfx_state.cursor_x = x;
	__gfx_state.cursor_y = y;
}

void gfx_setTextSize(uint8_t s)
{
	__gfx_state.textsize = (s > 0) ? s : 1;
}

void gfx_setTextColor(color_t c, color_t b)
{
	__gfx_state.textcolor   = c;
	__gfx_state.textbgcolor = b;
}

void gfx_setTextWrap(uint8_t w)
{
	__gfx_state.wrap = w;
}

uint8_t gfx_getRotation(void)
{
	return __gfx_state.rotation;
}

void gfx_setRotation(uint8_t x)
{
	__gfx_state.rotation = (x & 3);
	switch (__gfx_state.rotation) {
	case 0:
	case 2:
		__gfx_state._width  = GFX_WIDTH;
		__gfx_state._height = GFX_HEIGHT;
		break;
	case 1:
	case 3:
		__gfx_state._width  = GFX_HEIGHT;
		__gfx_state._height = GFX_WIDTH;
		break;
	}
}

/* Return the size of the display (per current rotation) */
uint16_t gfx_width(void)
{
	return __gfx_state._width;
}

uint16_t gfx_height(void)
{
	return __gfx_state._height;
}

