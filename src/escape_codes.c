#include <math.h>
#include <string.h>
#include "console.h"
#include "escape_codes.h"
#include "gfx.h"


struct escape_codes_struct escape_codes;


void escape_codes_init(void) {
	// escape_codes.argc     = 0;
	escape_code_reset();
}


/*
 * it is assumed that calling this function should return at least one
 * argument, if no digits were found, then one arg with default_value would be
 * returned
 */
// urxvt takes first 32 args
inline void parse_agruments(uint8_t *start, uint16_t default_value) {
	escape_codes.argc = 1;

	if (!IS_ARG_CHAR(*start)) {
		escape_codes.argv[0] = default_value;
		return;
	}
	
	uint8_t *end = start;

	// find the end of arguments string
	while (IS_ARG_CHAR(*end)) {
		if (*end == ';') {
			// checkme
			if (escape_codes.argc == ESCAPE_ARGS_LIMIT)
				break;

			escape_codes.argc++;
		}

		end++;
	}

	uint8_t *digit_boundary = end;
	uint8_t *current        = end;

	uint8_t arg = escape_codes.argc;

#define DIGIT_POSITION (digit_boundary - current - 1)

	// convert
	while (current >= start) {
		if (IS_DIGIT(*current)) {
			// check if the most significant digit is 6, discard value and say
			// that maximum argument value is 59999 xD
			// or somehow check if it exceeds 65535
			escape_codes.argv[arg] +=
				POW10(DIGIT_POSITION) * (*current - '0');

			if (DIGIT_POSITION == 4 && *current - '0' == 6)
				escape_codes.argv[arg] = 59999;
		} else {
			digit_boundary = current;

			// decrement arg here and skip subtraction below?
			if (arg > 0) {
				escape_codes.argv[arg - 1] = 0;

				// no number on another position so set the default_value
				// kinda not good?
				if (!IS_DIGIT(*(current - 1)))
					escape_codes.argv[arg - 1] = default_value;
			}

			arg--;
		}

		current--;
	}
}

// reset on rxvt-unicode-256color
// bash "\ec\e[?1000l\e[?25h\r"
// zsh  "\e[c\e[r\e[m\e[?7;25h\e[?1;3;4;5;6;9;66;1000;1001;1049l\e[4l\r"

// \e=
// \eD
// \e[4A
// \e[?1047h
// \ec

void escape_code_reset(void) {
	escape_codes.current = escape_codes.buffer;
	console.current_mode = CONSOLE_MODE_TEXT;
}

// xxx return \e1m on wrong escape sequence?

void escape_code_handle(uint8_t next_char) {
	// reset buffer for another escape code
	if (next_char == 0x1b) {
		escape_codes.current = escape_codes.buffer;
		return;
	}


	// put chars into buffer

#define ESCAPE_BUFFER_CHARS (escape_codes.current - escape_codes.buffer)

	if (ESCAPE_BUFFER_CHARS < ESCAPE_BUFFER_SIZE - 1) {
		*escape_codes.current++ = next_char;
		*escape_codes.current = 0; // -1 above because of this
	} else {
		// too long escape code
		escape_code_reset();
		return;
	}

#define IS_SUB_MODIFIER(v) (v == '?' || v == '>' || v == '!')

	// C1 [sub_modifier [number [;number]...] command
	switch (escape_codes.buffer[0]) {
	case '[': // CSI - control sequence introducer
		if (ESCAPE_BUFFER_CHARS == 1)
			return;

		if (ESCAPE_BUFFER_CHARS == 2 && IS_SUB_MODIFIER(next_char))
			return;

		// continue until non numeric character is aquired
		if (IS_ARG_CHAR(next_char))
			return;

#define START_INDEX 1 + IS_SUB_MODIFIER(escape_codes.buffer[1])
		// default value for these characters will be 0
#define DEFAULT_ARG_VALUE (next_char != 'm' \
						   && next_char != 'J' \
						   && next_char != 'K' \
						   )

		// should not be called every time?
		parse_agruments(&escape_codes.buffer[START_INDEX], DEFAULT_ARG_VALUE);

		handle_escape_code_open_csi(next_char);
		break;

	case ']': // OSC - operating system commands
		// eat everything for now
		if (next_char != '\a')
			return;

		// xxx check me
		// if (next_char != '\\'
		// 	&& escape_codes.buffer[ESCAPE_BUFFER_CHARS - 2] == 0x1b)
		// 	return;

		// won't work, it will keep reading after the semicolon
		// parse_agruments(&escape_codes.buffer[1], DEFAULT_ARG_VALUE);
		// TODO XXX should split this function into parse_csi_args and
		// retrieve_number which will return pointer to character after the
		// last digit

		// console_escape_code = yep;
		// // set window title "\e]0;...\a" or \e]0;...\e\\"
		// if (ESCAPE_BUFFER_CHARS == 2 && next_char != '0') {
		// 	console.current_mode = CONSOLE_MODE_TEXT;
		// 	break;
		// }

		// if (ESCAPE_BUFFER_CHARS == 3 && next_char != ';') {
		// 	console.current_mode = CONSOLE_MODE_TEXT;
		// 	break;
		// }

		// if (ESCAPE_BUFFER_CHARS > 3) {
		// 	if (next_char == '\a')
		// 		*(escape_codes.current - 1) = 0; // delete \a from buffer
		// 	else
		// 		return;
		// 	console_title_update(&escape_codes.buffer[3]#<{(|, ALIGN_CENTER|)}>#);
		// 	else if (!IS_VISIBLE(next_char)) // allows for images too!
		// 		console.current_mode = CONSOLE_MODE_TEXT;
		// }

	// "]11;?\a" - background color
		break;

	case '(': // designate G0 character set
	case ')': // designate G1 character set
	case '*': // designate G2 character set
	case '+': // designate G3 character set
	case '-': // designate G1 character set
	case '.': // designate G2 character set
	case '/': // designate G3 character set (next_char == A -> supplementary latin-1)
		if (ESCAPE_BUFFER_CHARS == 1)
			return;

		if (ESCAPE_BUFFER_CHARS == 2) {
			switch (escape_codes.buffer[1]) {
			case '0': // DEC special character and line drawing set
				console.character_set = CHARACTER_SPECIAL_SET;
				break;

			// case 'A': // UK
			// case 'B': // USASCII
			default:
				console.character_set = CHARACTER_DEFAULT_SET;
				break;
			}
		}
		break;

	case '=': // DECKPAM - application keypad
		// urxvt; pressing num key generates (but what for? for some programs?):
		// num * - \eOj
		// num + - \eOk
		// num - - \eOm
		// num . - \eOn
		// num / - \eOo
		// num 0 - \eOp
		// num 1 - \eOq
		// num 2 - \eOs
		// ...
		// num 9 - \eOy
		// num enter - \eOM
		break;

	case '>': // DECKPNM - normal keypad
		break;

	case '7': // DECSC - save cursor
		current_view->saved_cursor_x = current_view->screen_x;
		current_view->saved_cursor_y = current_view->screen_y;
		break;

	case '8': // DECRC - restore cursor
		current_view->screen_x = current_view->saved_cursor_x;
		current_view->screen_y = current_view->saved_cursor_y;
		break;

	// case 'F': // cursor to lower left corner of screen

	case 'D': // move window up 1 line
		console_region_scroll_lines(console.scroll_region_top,
									console.scroll_region_bottom,
									1);
		break;

	case 'M': // move window down 1 line
		// XXX should also work without scroll_region
		// test with `less`
		console_region_scroll_lines(console.scroll_region_top,
									console.scroll_region_bottom,
									-1);
		break;
	}

	escape_code_reset();
}


// XXX better not directly operate on console variables from here!
// write functions for that
// TODO sort by częstość występowania occurence frequency?

void handle_escape_code_open_csi(uint8_t next_char) {
	// if (cursor_move)
	cursor_reverse(); // XXX should be here?

	switch (next_char) {
	case '@': // ICH - insert argv[0] character
		console_move_chars_in_line(console_get_line(), console_get_column(),
								   escape_codes.argv[0]);
		break;

	case 'A': // cursor up
		console_set_line(console_get_line() - escape_codes.argv[0]);
		break;

	case 'B': // cursor down
		console_set_line(console_get_line() + escape_codes.argv[0]);
		break;

	case 'C': // cursor forward
		console_set_column(console_get_column() + escape_codes.argv[0]);
		break;

	case 'D': // cursor back
		console_set_column(console_get_column() - escape_codes.argv[0]);
		break;

	case 'H': // cursor position
	case 'f': { // cursor position
		line_t line     = escape_codes.argv[0];
		column_t column = 0;

		if (escape_codes.argc > 1)
			column = escape_codes.argv[1];

		console_set_column(column - 1);
		console_set_line(line - 1);
	}
		break;


	case 'J': { // erase in display
		uline_t current_line = console_get_line();

		switch (escape_codes.argv[0]) {
		case 0: // from cursor to the end of the screen
			// XXX test me
			for (uint8_t i = 0; i < SCREEN_LINES - current_line; i++)
				current_view->clear_marks[(current_line + i) % BUFFER_LINES] = 0;
			break;

		case 1: // from cursor to the beggining of the screen
			// XXX test me
			for (uint8_t i = 0; i < current_line; i++)
				current_view->clear_marks[(current_line - i) % BUFFER_LINES] = 0;
			break;

			// XXX breaks on 16th line
		case 2: // entire screen and move the cursor to 1,1
			// memset(screen_start, 0, LCD_WIDTH * FONT_CHAR_HEIGHT * SCREEN_LINES);
			// console_set_column(0);
			// console_set_line(0);
			// break;

			console_set_column(0);
			console_set_line(0);

			for (uint8_t i = 0; i < SCREEN_LINES; i++)
				current_view->clear_marks[(current_line + i) % BUFFER_LINES] = 0;
			// memset(current_view->screen_buffer, 0, LCD_WIDTH * FONT_CHAR_HEIGHT * SCREEN_LINES);
			// for (uint8_t i = 0; i < SCREEN_LINES; i++)
			// 	current_view->clear_marks[(i + CURRENT_LINE) % BUFFER_LINES] = 0;

			// current_view->screen_y = screen_start - screen_buffer;
			// current_view->screen_y = (screen_start - screen_buffer) / 50;
			// current_view->screen_x = 0;
			break;

		case 3: // clear the entire buffer and move cursor to 1,1
			// XXX test me
			memset(current_view->screen_buffer, 0, SCREEN_BUFFER_SIZE);

			current_view->screen_y = current_view->screen_start - current_view->screen_buffer;
			current_view->screen_x = 0;
			break;
		}

		// console_dirty = 1;
		console_work();
	}
		break;

	case 'K': // erasing in line
		switch (escape_codes.argv[0]) {
		case 0: // from the current cursor position
			current_view->clear_marks[CURRENT_LINE] = console_get_column();
			break;

		case 1: // from the beggining of the line to the current cursor position
			// XXX vim show occurences: [I
			current_view->clear_marks[CURRENT_LINE] = -console_get_column() - 1;
			break;

		case 2: // entire line
			current_view->clear_marks[CURRENT_LINE] = 0;
			break;
		}

		// console_dirty = 1;
		console_work();
		break;

	// 1049h 1h 12;25h 12l 25h 25l
	// 12l 25h 25l 12l 25h 25l 12l 25h 25l 1l 12l 25h 1049l
	// what if values == 1049 or 2004 or 12 or 1?
	// what if values == 1049 or 2004 or 12 or 1 or 12 + 25 << 8?

	case 'P':  // DCH - delete argv[0] characters
		console_move_chars_in_line(console_get_line(), console_get_column(),
								   -escape_codes.argv[0]);
		break;

	case 'X': { // ECH - erase argv[0] characters
		ucolumn_t spaces = escape_codes.argv[0];

		if (!spaces)
			break;

		ucolumn_t current_column = console_get_column();

		if (spaces + current_column > MAX_COLUMN)
			spaces = MAX_COLUMN - current_column;

		gfx_fillRect(current_view->screen_x, current_view->screen_y,
					 spaces * FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT,
					 console.background_color);
	}
		break;

	case 'c': // reset console
		// xxx what should it do?
		console_reset();
		// or if modifier == '>' (and argv[0] == 0) respond with terminal
		// identification code, which here would probably be:
		// CSI > 0 ; version ; 0 c (vt100)
		break;

	case 'G': // CHA - cursor character absolute
		// console_set_line(0);
		console_set_column(escape_codes.argv[0] - 1);
		break;

	case 'd': // VPA - line position absolute
		console_set_line(escape_codes.argv[0] - 1);
		console_set_column(0);
		break;

	// case 'e': // VPR - line position relative

	case 'h': // SM - set mode
		if (escape_codes.buffer[1] != '?')
			// 4 - IRM - insert mode
			break;

		// DECSET
		switch (escape_codes.argv[0]) {
		case 5: // DECSCNM - reverse video
			console.reverse = 0xff; // xxx will break bell
			break;

		case 7: // DECAWM - wrap around mode
			console.wrap_around = 1;
			break;

		case 12: // att610 - start blinking cursor
			// global, not bufferwise
			break;

		case 25: // DECTCEM - show cursor
			// case (25 << 16) | 12: // (first_arg == 12 && second_arg == 25)
			console.show_cursor = 1;
			break;

		case 47: case 1047: // use alternate screen buffer
			current_view = &console.view[1];
			break;

		// case 1000: // send mouse x and y on button press and release (xterm)
		// case 1015: // enable urxvt mouse mode

		case 1048: // save cursor as in DESCSC
			break;

		case 1049: // 1048 + 1047 and clear the screen
			current_view = &console.view[1];
			break;

		case 2004: // bracket paste mode
			// For example, let's say I copied the string "echo 'hello'\n" from
			// a website. When I paste into my terminal it will send
			// "\e[200~echo 'hello'\n\e[201~" to whatever program is
			// running."]]"
			// I admit this is hard to get excited about, but it turns out that
			// it enables something very cool: programs can tell the difference
			// between stuff you type manually and stuff you pasted.
			break;
		}
		break;

	case 'l': // RM - reset mode
		if (escape_codes.buffer[1] != '?')
			// 4 - IRM - replace mode
			break;

		// DECRST
		switch (escape_codes.argv[0]) {
		case 5: // DECSCNM - reverse video
			console.reverse = 0;
			break;

		case 7: // DECAWM - no wrap around mode
			console.wrap_around = 0;
			break;

		case 12: // att610 - stop blinking cursor
			break;

		case 25: // hide the cursor
			console.show_cursor = 0;
			break;

		case 47: case 1047: // use alternate screen buffer
			current_view = &console.view[0];
			break;

		case 1049: // 1048 + 1047 and clear the screen
			current_view = &console.view[0];
			break;
		}
		break;

	case 'm': // set graphics rendition
		if (escape_codes.argc == 0)
			console_color_reset();

		for (uint8_t i = 0; i < escape_codes.argc; i++) {
			switch (escape_codes.argv[i]) {
			case 0:
				console_color_reset();
				break;

			case 1: // bold
				console.text_mode |= CONSOLE_TEXT_EFFECT_BOLD;
				break;

			case 4: // underline
				console.text_mode |= CONSOLE_TEXT_EFFECT_UNDERLINE;
				break;

			case 3: // italic (fallback to inverse - for now..?)
				// 	console.text_mode |= CONSOLE_TEXT_EFFECT_ITALIC;
				// 	break;
			case 7:
				console.foreground_color = LCD_WHITE;
				console.background_color = LCD_BLACK;
				break;

			case 9: // crossed out
				console.text_mode |= CONSOLE_TEXT_EFFECT_CROSSED_OUT;
				break;

			case 21:
				console.text_mode &= ~CONSOLE_TEXT_EFFECT_BOLD;
				break;

			case 24:
				console.text_mode &= ~CONSOLE_TEXT_EFFECT_UNDERLINE;
				break;

			case 23:
				// 	console.text_mode &= ~CONSOLE_TEXT_EFFECT_ITALIC;
				// 	break;
			case 27:
				console.foreground_color = LCD_BLACK;
				console.background_color = LCD_WHITE;
				break;

			case 29: // crossed out
				console.text_mode &= ~CONSOLE_TEXT_EFFECT_CROSSED_OUT;
				break;

			// should it be here?
			// case 30:
			// 	console_foreground_color = LCD_BLACK;
			// 	break;

			// case 37:
			// 	console_foreground_color = LCD_WHITE;
			// 	break;

			// case 40:
			// 	console_background_color = LCD_BLACK;
			// 	break;

			// case 47:
			// 	console_background_color = LCD_WHITE;
			// 	break;
			}
		}
		break;

	// case 'n': // value == 6 - write current cursor position (\e[<row>;<column>R)
	// case 'n': // DSR - device status report
	// 	switch (escape_codes.argv[0]) {
	// 	case 5: // status report
	// 		console_send("\e[0n"); // OK
	// 		break;

	// 	case 6: // CPR - cursor position report
	// 		console_send("\e[" row ";" column "n");
	// 		break;
	// 	}
	// 	break;

	// xxx gets called when scroll_region is set
	case 'S': // SU - scroll up argv[0] lines
		if (escape_codes.buffer[1] == '?' || escape_codes.argc != 1)
			return;

		console_region_scroll_lines(console.scroll_region_top,
									console.scroll_region_bottom,
									escape_codes.argv[0]);
		break;

	case 'T': // SD - scroll down argv[0] lines
		if (escape_codes.buffer[1] == '>' || escape_codes.argc != 1)
			return;

		console_region_scroll_lines(console.scroll_region_top,
									console.scroll_region_bottom,
									-escape_codes.argv[0]);
		break;

	case 'L': // IL - insert argv[0] lines
		if (escape_codes.argc != 1)
			return;

		console_region_scroll_lines(console.scroll_region_top,
									console.scroll_region_bottom,
									-escape_codes.argv[0]);
		break;

	case 'M': // DL - delete argv[0] lines
		// from cursor? top?
		break;

	case 'r':
		if (escape_codes.buffer[1] != '?') {
			// set top and bottom margins
			// [12;24r   Set scrolling region to lines 12 thru 24.  If a linefeed or an
			//             INDex is received while on line 24, the former line 12 is deleted
			//             and rows 13-24 move up.  If a RI (reverse Index) is received while
			//             on line 12, a blank line is inserted there as rows 12-13 move down.
			//             All VT100 compatible terminals (except GIGI) have this feature.]

			// CSI Ps ; Ps r
			//           Set Scrolling Region [top;bottom] (default = full size of win-
			//           dow) (DECSTBM).
			if (escape_codes.argc <= 2) {
				uline_t top    = escape_codes.argv[0];
				uline_t bottom = escape_codes.argv[1];

				if (top >= bottom)
					break;

				console.scroll_region_top = escape_codes.argc == 0?
					0:
					top - 1; // XXX could break if user said 0

				console.scroll_region_bottom = escape_codes.argc <= 1?
					MAX_LINE - 1:
					bottom - 1; // XXX could break if user said > MAX_LINE

				// XXX also move cursor to scroll_region.top or bottom?

				// scroll irssi by one line
				// \e[2;24r
				// \e[2;1H // not necessary?
				// \e[24;1H // not necessary?
				// \e[1S
				// \e[1;26r // same as \e[r in that case
			}
			// CSI Pt; Pl; Pb; Pr; Ps$ r
			//           Change Attributes in Rectangular Area (DECCARA), VT400 and up.
			//             Pt; Pl; Pb; Pr denotes the rectangle.
			//             Ps denotes the SGR attributes to change: 0, 1, 4, 5, 7.
		} else {
			// CSI ? Pm r
			//           Restore DEC Private Mode Values.  The value of Ps previously
			//           saved is restored.  Ps values are the same as for DECSET.
			if (escape_codes.argv[0] == 1001) {
				// wut
			}
		}
		break;

	case 's':
		if (escape_codes.buffer[1] != '?') {
			// CSI s     Save cursor (ANSI.SYS), available only when DECLRMM is dis-
			//           abled.
			// CSI Pl; Pr s
			//           Set left and right margins (DECSLRM), available only when
			//           DECLRMM is enabled (VT420 and up).
		} else {
			// CSI ? Pm s
			//           Save DEC Private Mode Values.  Ps values are the same as for
			//           DECSET.
		}
		break;

	// case 't': // xterm? window manipulation
	// 	switch (escape_codes.argv[0]) {
	// 	case 11:
	// 		//   Ps = 1 1  -> Report xterm window state.  If the xterm window
	// 		// is open (non-iconified), it returns CSI 1 t .  If the xterm
	// 		// window is iconified, it returns CSI 2 t .
	// 		console_send("\e[1t");
	// 		break;

	// 	case 13:
	// 		//   Ps = 1 3  -> Report xterm window position.
	// 		// Result is CSI 3 ; x ; y t
	// 		console_send("\e[3;0;0t");
	// 		break;

	// 	case 14:
	// 		//   Ps = 1 4  -> Report xterm window in pixels.
	// 		// Result is CSI  4  ;  height ;  width t
	// 		console_send("\e[" (escape_codes.argv[0] + '0' - 10) ";"
	// 					 LCD_WIDTH*8 ";" (LCD_HEIGHT - TITLE_BAR_HEIGHT) "t");
	// 		break;

	// 	case 18:
	// 	case 19:
	// 		//   Ps = 1 8  -> Report the size of the text area in characters.
	// 		// Result is CSI  8  ;  height ;  width t
	// 		//   Ps = 1 9  -> Report the size of the screen in characters.
	// 		// Result is CSI  9  ;  height ;  width t
	// 		console_send("\e[8;" (SCREEN_LINES - (TITLE_BAR? 1: 0)) ";" SCREEN_COLUMNS "t");
	// 		break;

	// 		//   Ps = 2 0  -> Report xterm window's icon label.
	// 		// Result is OSC  L  label ST

	// 		//   Ps = 2 1  -> Report xterm window's title.
	// 		// Result is OSC  l  label ST

	// 		//   Ps = 2 2  ;  0  -> Save xterm icon and window title on
	// 		// stack.

	// 		//   Ps = 2 2  ;  1  -> Save xterm icon title on stack.

	// 		//   Ps = 2 2  ;  2  -> Save xterm window title on stack.

	// 		//   Ps = 2 3  ;  0  -> Restore xterm icon and window title from

	// 		// stack.
	// 		//   Ps = 2 3  ;  1  -> Restore xterm icon title from stack.

	// 		//   Ps = 2 3  ;  2  -> Restore xterm window title from stack.

	// 		//   Ps >= 2 4  -> Resize to Ps lines (DECSLPP).

	// 	}
	// 	break;

	case 't': {
		console.current_mode = CONSOLE_MODE_TEXT;

		uint8_t lol[7] = {0};

		lol[0] = 'c';
		lol[1] = ':';
		lol[2] = ' ';
		lol[3] = '0' + ((escape_codes.argc / 10) % 10);
		lol[4] = '0' + ((escape_codes.argc / 1) % 10);
		lol[5] = ' ';
		lol[6] = 0;
		console_puts(lol);

		for (uint8_t i = 0; i < escape_codes.argc; i++) {
			lol[0] = '0' + ((escape_codes.argv[i] / 10000) % 10);
			lol[1] = '0' + ((escape_codes.argv[i] / 1000) % 10);
			lol[2] = '0' + ((escape_codes.argv[i] / 100) % 10);
			lol[3] = '0' + ((escape_codes.argv[i] / 10) % 10);
			lol[4] = '0' + ((escape_codes.argv[i] / 1) % 10);
			lol[5] = ' ';
			lol[6] = 0;
			console_puts(lol);
		}

		console.current_mode = CONSOLE_MODE_ESCAPE_CODE;
	}
		break;

		// custom
	case '^': // display an image
		// cursor was reveresed before drawin - undo this so the image
		// doesn't an inverted part
		// cursor_reverse();

		console.image_width   = escape_codes.argv[0];
		console.image_height  = escape_codes.argv[1];
		// TODO
		// console.image_x       = escape_codes.argv[2];
		// console.image_y       = escape_codes.argv[3];
		console.image_receive = 0;

		// like that because only image_width is checked to see if image
		// should be drawn
		if (console.image_width == 0 || console.image_height == 0) {
			console.image_width = 0;
			break;
		}

		// make room for the image if cursor is too low
		// uint8_t image_lines = image_height / FONT_CHAR_HEIGHT + 1;
		// if (SCREEN_LINES - CURRENT_LINE_VISIBLE < image_lines) {
		// 	// scroll
		// 	// wrap? % BUFFER_SIZE
		// 	screen_start += LCD_WIDTH * (image_height / FONT_CHAR_HEIGHT + 1) * FONT_CHAR_HEIGHT;

		// 	// and clear screen
		// 	// FIXME
		// 	for (uint8_t l = 0; l < image_lines; l++)
		// 		clear_marks[CURRENT_LINE + l] = 0;
		// 	console_dirty = 1;
		// }

		// if (image_lines <= SCREEN_LINES - CURRENT_LINE_VISIBLE - 1)
		// 	current_view->screen_y += image_lines * FONT_CHAR_HEIGHT;
		// else
		// 	current_view->screen_y += (SCREEN_LINES - CURRENT_LINE_VISIBLE) * FONT_CHAR_HEIGHT;

		uint8_t image_lines = ((console.image_height - 1) / FONT_CHAR_HEIGHT + 1);

		console_append_new_lines(image_lines);
		console_set_line(console_get_line() - image_lines);
		break;
	}

	// don't show cursor if image is being drawn
	// if (cursor_move) {
	cursor_position_update();
	cursor_reverse();
	// }

	escape_code_reset();
}
