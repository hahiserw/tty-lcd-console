/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
// #include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/cm3/nvic.h>

#define debugln(message) debug(message "\r\n")
// #define LAME_RENDER

#include "config.h"
#include "itm.h"
#include "gfx.h"
#include "console.h"

#include <math.h>

static void lcd_setup(void)
{
	gfx_init(lcd_draw_pixel, GFX_WIDTH, GFX_HEIGHT);
}

static void clock_setup(void)
{
	// rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
	// rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_120MHZ]);
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
	// rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_48MHZ]);
}

static void gpio_setup(void)
{
	/* Enable GPIOD clock. */
	/* Manually: */
	// RCC_AHB1ENR |= RCC_AHB1ENR_IOPDEN;
	/* Using API functions: */
	rcc_periph_clock_enable(RCC_GPIOD);

	/* Set GPIO12 (in GPIO port D) to 'output push-pull'. */
	/* Manually: */
	// GPIOD_CRH = (GPIO_CNF_OUTPUT_PUSHPULL << (((8 - 8) * 4) + 2));
	// GPIOD_CRH |= (GPIO_MODE_OUTPUT_2_MHZ << ((8 - 8) * 4));
	/* Using API functions: */
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4 | GPIO5 | GPIO6 | GPIO7);
}

static void timer_setup(void)
{
	rcc_periph_clock_enable(DISPLAY_RCC);
	nvic_enable_irq(DISPLAY_TIMER_NVIC);
	// nvic_set_pririty(DISPLAY_TIMER_NVIC, 1);

	timer_reset(DISPLAY_TIMER_CONST);
	// timer_set_prescaler(DISPLAY_TIMER_CONST, 110);
	timer_set_period(DISPLAY_TIMER_CONST, 9000-1); // 9000 ~ 84MHZ
	timer_enable_update_event(DISPLAY_TIMER_CONST);
	timer_enable_counter(DISPLAY_TIMER_CONST);
	timer_enable_irq(DISPLAY_TIMER_CONST, TIM_DIER_UIE);


	rcc_periph_clock_enable(FRAME_RCC);
	nvic_enable_irq(FRAME_TIMER_NVIC);
	// nvic_set_pririty(FRAME_TIMER_NVIC, 2);

	// // TIM_PSC(FRAME_TIMER_CONST) = 64000 - 1;
	// // TIM_ARR(FRAME_TIMER_CONST) = 1000 - 1; // end timer value
	// // TIM_CR1(FRAME_TIMER_CONST) |= TIM_CR1_CEN;
	// timer_reset(FRAME_TIMER_CONST);
	// TIM_CNT(FRAME_TIMER_CONST) = 0x380;
	// TIM_ARR(FRAME_TIMER_CONST) = 640; // end timer value
	// TIM_CNT(FRAME_TIMER_CONST) = 640;
	// TIM_ARR(FRAME_TIMER_CONST) = 0xfff;
	// 100ms for 86MHz
	timer_set_prescaler(FRAME_TIMER_CONST, 42000 - 1); // 42000
	timer_set_period(FRAME_TIMER_CONST, 200); // 2
	// TIM_SR(FRAME_TIMER_CONST) = 0;
	// timer_enable_update_event(FRAME_TIMER_CONST);
	// timer_enable_counter(FRAME_TIMER_CONST);
	// timer_disable_counter(FRAME_TIMER_CONST);
	TIM_DIER(FRAME_TIMER_CONST) |= TIM_DIER_UIE;
	timer_enable_irq(FRAME_TIMER_CONST, TIM_DIER_UIE);
}

void DISPLAY_FN(void)
{
	if (!(TIM_SR(DISPLAY_TIMER_CONST) & TIM_SR_UIF))
		return;

	TIM_SR(DISPLAY_TIMER_CONST) &= ~TIM_SR_UIF;

	gpio_toggle(GPIOD, GPIO13);
	lcd_line();
}

void FRAME_FN(void) {
	if (!(TIM_SR(FRAME_TIMER_CONST) & TIM_SR_UIF))
		return;

	TIM_SR(FRAME_TIMER_CONST) &= ~TIM_SR_UIF; // clear interrupt flag

	// console.reverse = ~console.reverse;
	console.reverse = 0; // for now?
	// console_puts((uint8_t *)"</bell>");
	timer_disable_counter(FRAME_TIMER_CONST);
}

static void usart_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART2);
	nvic_enable_irq(NVIC_USART2_IRQ);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO3);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO3);

	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX); // rx only? tx for debug
	// usart_set_mode(USART2, USART_MODE_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	usart_enable_rx_interrupt(USART2);
	usart_enable(USART2);
}

// fifo
#define TEXT_BUFFER_SIZE 8000
uint8_t text_buffer[TEXT_BUFFER_SIZE] = {0};
uint8_t *text_write = text_buffer;
uint8_t *text_read = text_buffer;

char *usart_message = NULL;

// todo put into debug.c
#define DEBUG_BUFFER_SIZE 2000
char debug_buffer[DEBUG_BUFFER_SIZE] = {0};
void debug2(char *message, uint16_t v) {
	char *sc = message;
	char *dc = debug_buffer;

	// go to the \0 in case previous message wasn't written yet
	while (*dc)
		dc++;
	
	while ((*dc++ = *sc++))
		;

	dc--;
	*dc++ = ':';
	*dc++ = ' ';
	*dc++ = '0' +  (v / 10000);
	*dc++ = '0' + ((v /  1000) % 10);
	*dc++ = '0' + ((v /   100) % 10);
	*dc++ = '0' + ((v /    10) % 10);
	*dc++ = '0' +           (v % 10);
	*dc++ = ' ';
	*dc = 0;

	debug(debug_buffer);
	// debug_buffer[0] = 0;
}

// make it so that if previous message is not send yet append current one
// ... and to do that there should be a write buffer
void debug(char *message) {
	if (usart_message) {
		// append
	}
	usart_message = message;
	usart_enable_tx_interrupt(USART2);
}

void usart2_isr(void)
{
	// RXNE - rx not empty
	// RXNEIE - rxne interrupt enable
	if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
		((USART_SR(USART2) & USART_SR_RXNE) != 0))
	{
		gpio_set(GPIOD, GPIO15);

		uint8_t c = usart_recv(USART2);

		// update receive fifo buffer with another character (and just ignore
		// null bytes)
		if (c && text_write - text_buffer < TEXT_BUFFER_SIZE - 1) {
			*text_write++ = c;
			*text_write = 0;
		}

	}
	else
	if (((USART_CR1(USART2) & USART_CR1_TXEIE) != 0) &&
		((USART_SR(USART2) & USART_SR_TXE) != 0))
	{
		usart_send(USART2, *usart_message++);

		if (*usart_message == 0) {
			usart_disable_tx_interrupt(USART2);
			usart_message = 0;
			debug_buffer[0] = 0;
		}
	}

	gpio_clear(GPIOD, GPIO15);
}

// void FRAME_FN(void)
// {
// 	gpio_toggle(GPIOD, GPIO14);
//
// 	draw = 1;
//
// 	TIM_SR(FRAME_TIMER_CONST) &= ~TIM_SR_UIF;
// }

// TODO
// - write console helper functions
// - update functions to address lines / disaplay data in wrapped/cycled manner (always: y % HEIGHT)
// - DMA for screen
// - DMA for USART
// - add some game?

void easter_egg(void);
void easter_egg(void) {
	// draw some stuff in alternate buffer
	current_view = &console.view[1];
	console_puts((uint8_t*)
				 "\e[60;98^//////////gAA////4ABgAA////+ABgAD//FX+ABgAH+4on/gB"
				 "gAP/oAV/wBgA/+gBN/wBgB/8AAC/4BgB/wAAF/8BgD/QAAC3+BgD8AAAB//"
				 "BgH/IAASv/BgH+AAABb/hgH8AAAB//hgH7AAAAP/xgPsAAADb/9gP9QAAA3"
				 "/9gf9AAAVb/9gf8AAABff/g/+AAEsr//g/4AAH+///g/9eAH3V//g///AIB"
				 "+//h//KA3Xr//h//k4X/yP/h/+34699//h/3+46bsf/j//9obBxn/j9/KZc"
				 "rMf/j//hoOoSX/n/f8ykEIf/v/6F4AAB3/v/8QwQAEf/v/SE4QACb///wAQ"
				 "cAAN///oAgKAA3/v/yBgAAAP///4A4/ACd///UDtxgAP///4B//8Bf///gC"
				 "+fvBP///0D//3hf///yX///hX///4P//+w////oe3//hff//6/+vf1f7//9"
				 "v8j97v///3/wFX1////7+7V/v/////f9SHn/////88JX7/////2Kjj/////"
				 "//3+//////////rf/////////////////////////7/////////////////"
				 "////////////////////////d////////3///3/v///////////////////"
				 "/////7+//////////3////////////v/////////v/////////j////////"
				 "/j/////////n/////////h/////////h///f/////n/////////n///////"
				 "//v//////9//////////////////7////////+/P///////rpf//////8AA"
				 "///////mAH///////QAD///////AAH/////////////gAAAAAAAABoAEBQA"
				 "ACABgAEBQAACABrjuZQcznMZqUEFQlKSSlqTEdQl6SSlqQklQlCSSlqXCdQ"
				 "c6RMZgAAAAEAAABgAAAAYAAABgAAAAAAAAB//////////"
				);
	current_view = &console.view[0];
}

int main(void)
{
	clock_setup();
	gpio_setup();
	timer_setup();
	lcd_setup();
	usart_setup();

	iwdg_reset();
	iwdg_set_period_ms(500); // can execute escape codes up to 500ms I guess
	iwdg_start();

	console_init();

	debugln("Console on ITM-400160K1-01 debug ready :^)");

#define CONSOLE_DIMENSIONS STR(SCREEN_COLUMNS) "x" STR(SCREEN_LINES)

	// gfx_setCursor(0, 0); // lol, use it in other places!
	gfx_setTextColor(LCD_BLACK, LCD_WHITE);
	console_puts((uint8_t*)
				 "\a" // FIXME temporary workaround for broken, too short, first bell
				 "\e[8;12^QiQY/4OBg4P/q/8=" // tv icon :)
				 " Console " CONSOLE_DIMENSIONS
				 " on ITM-400160K1-01 ready \e[1m:^)\e[m\n"
				 );

	easter_egg();

	// 50x13 = 650 characters on screen

	for (;;) {
		// there is another character in buffer
		if (*text_read) {
			gpio_toggle(GPIOD, GPIO12);

			// write another character
			console_update(*text_read);

			text_read++;

			if (*text_read == 0) {
				// reset pointers
				text_read = text_write = text_buffer;
				// "clear" buffer
				*text_buffer = 0;
			}
			// will be syncing :\ has to handle erasing properly somehow
			// console_work();
		}

		iwdg_reset();

		gpio_clear(GPIOD, GPIO12);
	}

	return 0;
}
