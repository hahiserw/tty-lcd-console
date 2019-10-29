#ifndef _CONFIG_H
#define _CONFIG_H

#include <libopencm3/stm32/gpio.h>

#define DEBUG 1 // XXX not used yet, but should include debug.h in some places

// console will print that many spaces (appropriately padding) upon receiving
// a tab character
#define TAB_SIZE 4

// 0 - block
// 1 - a bit smaller block
// 2 - outline
#define CURSOR_STYLE 0

#define FONT_SMALL


// :syn keyword cType pos_t upos_t column_t ucolumn_t line_t uline_t color_t
typedef uint8_t color_t;

typedef uint16_t upos_t;
typedef int16_t pos_t;

typedef uint8_t ucolumn_t;
typedef int8_t   column_t; // lol not "u", FIXME

typedef uint8_t uline_t;
typedef int8_t   line_t;

#define GPIO_Pin_0 GPIO0
#define GPIO_Pin_1 GPIO1
#define GPIO_Pin_2 GPIO2
#define GPIO_Pin_3 GPIO3
#define GPIO_Pin_4 GPIO4
#define GPIO_Pin_5 GPIO5
#define GPIO_Pin_6 GPIO6
#define GPIO_Pin_7 GPIO7
#define GPIO_Pin_8 GPIO8
#define GPIO_Pin_9 GPIO9
#define GPIO_Pin_10 GPIO10
#define GPIO_Pin_11 GPIO11
#define GPIO_Pin_12 GPIO12
#define GPIO_Pin_13 GPIO13
#define GPIO_Pin_14 GPIO14
#define GPIO_Pin_15 GPIO15

// #define RAW

// lcd
#define DISPLAY_TIMER 3
// gif
#define FRAME_TIMER 2

#ifdef RAW
#define GPIOA 0
#define GPIOB 1
#define GPIOC 2
#define GPIOD 3
#define GPIOE 4
#define GPIOF 5
#endif

#define DATA_PORT    GPIOA
#define CONTROL_PORT GPIOD

// redefine these functions for faster memory access
// GPIO_SetBits(GPIOx, GPIO_Pin)
// GPIO_ResetBits(GPIOx, GPIO_Pin)
// GPIO_ToggleBits(GPIOx, GPIO_Pin)

#ifdef RAW
#define _SET(GPIOx, GPIO_Pin) *(0x40020000U + 0x400 * GPIOx + 0x18) = GPIO_Pin
#define _CLR(GPIOx, GPIO_Pin) *(0x40020000U + 0x400 * GPIOx + 0x18) = ((GPIO_Pin) << 16)
#define _TGL(GPIOx, GPIO_Pin) GPIOx->ODR ^= GPIO_Pin
#else
// #define _SET(GPIOx, GPIO_Pin) GPIOx->BSRRL = GPIO_Pin
// #define _CLR(GPIOx, GPIO_Pin) GPIOx->BSRRH = GPIO_Pin
// #define _TGL(GPIOx, GPIO_Pin) GPIOx->ODR ^= GPIO_Pin
#define _SET(GPIOx, GPIO_Pin) (GPIO_BSRR(GPIOx) = GPIO_Pin)
#define _CLR(GPIOx, GPIO_Pin) (GPIO_BSRR(GPIOx) = GPIO_Pin << 16)
#define _TGL(GPIOx, GPIO_Pin) {uint32_t port = GPIO_ODR(GPIOx); GPIO_BSRR(GPIOx) = ((port & GPIO_Pin) << 16) | (~port & GPIO_Pin);}
#endif

#define SET(a) _SET(a)
#define CLR(a) _CLR(a)
#define TGL(a) _TGL(a)

// #define CONTROL_OFFSET 8
// #define CL1 CONTROL_PORT, (GPIO_Pin_0<<CONTROL_OFFSET)
// #define CL2 CONTROL_PORT, (GPIO_Pin_1<<CONTROL_OFFSET)
// #define  DI CONTROL_PORT, (GPIO_Pin_2<<CONTROL_OFFSET)
// #define   M CONTROL_PORT, (GPIO_Pin_3<<CONTROL_OFFSET)

#define CL2 CONTROL_PORT, GPIO_Pin_8
#define  DI CONTROL_PORT, GPIO_Pin_9
#define   M CONTROL_PORT, GPIO_Pin_10
#define CL1 CONTROL_PORT, GPIO_Pin_11

#define DATA_PINS(GPIO_Pin) DATA_PORT, GPIO_Pin<<4



#define _STR(x) #x
#define STR(x) _STR(x)

#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__
// #define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)

#define PPCAT(a, b, c) a ## b ## c
#define PCAT(a, b, c) PPCAT(a, b, c)
#define TIMER(t) PRIMITIVE_CAT(TIM, t)


#define FRAME_TIMER_CONST TIMER(FRAME_TIMER)
#define FRAME_TIMER_NVIC  PCAT(NVIC_TIM, FRAME_TIMER, _IRQ)
#define FRAME_FN          PCAT(tim, FRAME_TIMER, _isr)
#define FRAME_RCC         PCAT(RCC_, TIM, FRAME_TIMER)

#define DISPLAY_TIMER_CONST TIMER(DISPLAY_TIMER)
#define DISPLAY_TIMER_NVIC  PCAT(NVIC_TIM, DISPLAY_TIMER, _IRQ)
#define DISPLAY_FN          PCAT(tim, DISPLAY_TIMER, _isr)
#define DISPLAY_RCC         PCAT(RCC_, TIM, DISPLAY_TIMER)




#define cursor_position_update() \
		current_view->last_cursor_x = current_view->screen_x; \
		current_view->last_cursor_y = current_view->screen_y;


#define CURSOR_X (current_view->last_cursor_x >> 3)
#define CURSOR_Y (current_view->last_cursor_y + y)

#ifdef FONT_SMALL
#define CURSOR_VALUE_0 (0xf0 >> (current_view->last_cursor_x & 4))
#define CURSOR_VALUE_1 (0x70 >> (current_view->last_cursor_x & 4))
#define CURSOR_VALUE_2 (0x50 >> (current_view->last_cursor_x & 4))
#else
#define CURSOR_VALUE_0 0xff
#define CURSOR_VALUE_1 0x7f
#define CURSOR_VALUE_2 0x41
#endif

#if CURSOR_STYLE == 0 // block
#define CURSOR \
		for (uint8_t y = 0; y < FONT_CHAR_HEIGHT; y++) { \
			*(current_view->screen_buffer + CURSOR_Y * LCD_WIDTH + CURSOR_X \
			  ) ^= CURSOR_VALUE_0; \
		}
#elif CURSOR_STYLE == 1 // smaller block
#define CURSOR \
		for (uint8_t y = 0; y < FONT_CHAR_HEIGHT - 1; y++) { \
			*(current_view->screen_buffer + CURSOR_Y * LCD_WIDTH + CURSOR_X \
			  ) ^= CURSOR_VALUE_1; \
		}
#elif CURSOR_STYLE == 2 // outline
#define CURSOR \
		*(current_view->screen_buffer + (current_view->last_cursor_y) \
		  * LCD_WIDTH + CURSOR_X \
		  ) ^= CURSOR_VALUE_1; \
		*(current_view->screen_buffer \
		  + (current_view->last_cursor_y + FONT_CHAR_HEIGHT - 2) \
		  * LCD_WIDTH + CURSOR_X \
		  ) ^= CURSOR_VALUE_1; \
		for (uint8_t y = 1; y < FONT_CHAR_HEIGHT - 2; y++) { \
			*(current_view->screen_buffer + CURSOR_Y * LCD_WIDTH + CURSOR_X \
			  ) ^= CURSOR_VALUE_2; \
		}
#endif

#define cursor_reverse() \
	if (console.show_cursor) { \
		CURSOR \
	}



void debug(char *);
void debug2(char *, uint16_t);

#endif
