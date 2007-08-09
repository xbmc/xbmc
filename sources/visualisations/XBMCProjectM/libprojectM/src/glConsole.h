/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */

#ifndef _GLCONSOLE_H
#define _GLCONSOLE_H

#include "glf.h"

#define GL_CONSOLE_DEBUG 0
#define MAX_CONSOLE_HEIGHT 500
#define MAX_CONSOLE_WIDTH 500

#define CONSOLE_FLAG_NONE 0
#define CONSOLE_FLAG_CURSOR_HIDDEN 1
#define CONSOLE_FLAG_CURSOR_BLINKING (1 << 1)
#define CONSOLE_FLAG_SCROLL_WRAP_RIGHT (1 << 3)
#define CONSOLE_FLAG_SCROLL_WRAP_LEFT (1 << 4)
#define CONSOLE_FLAG_SCROLL_WRAP_UP (1 << 5)
#define CONSOLE_FLAG_SCROLL_WRAP_DOWN (1 << 6)
#define CONSOLE_FLAG_ALWAYS_DRAW_BACKGROUND (1 << 7)

#define BAR_STYLE 0
#define UNDERLINE_STYLE 1

typedef enum {
  CONSOLE_RED,
  CONSOLE_BLACK,
  CONSOLE_BLUE,
  CONSOLE_WHITE,
  CONSOLE_GREEN,
  CONSOLE_TRANS
} color_t;

typedef struct CONSOLE_CHAR_T {
	char symbol;
	color_t fg_color;
	color_t bg_color;
} console_char_t;
	

typedef struct GL_CONSOLE_T {
  float start_x;
  float start_y;
  int screen_width;
  int screen_height;
  int scroll_width;
  int scroll_height;
  console_char_t * screen_buffer; /* start of current screen buffer */
  console_char_t * scroll_buffer; /* pointer to very top of buffer */
  int font_descriptor;
  int screen_x;
  int screen_y;
  int screen_start_x;
  int screen_start_y;
  int cursor_style;
  color_t current_fg;
  color_t current_bg;
  color_t cursor_color;
  console_char_t * cursor_ptr; /* pointer to position in console buffer */
  short int flags;
} gl_console_t;

int glConsoleSetFGColor(color_t color, gl_console_t * gl_console);
int glConsoleSetBGColor(color_t color, gl_console_t * gl_console);
int glConsoleSetCursorColor(color_t color, gl_console_t * gl_console);
int glConsoleSetCursorPos(int x, int y, gl_console_t * gl_console);
int glConsoleGetCursorPos(int * x, int  * y, gl_console_t * gl_console);
int glConsoleShowCursor(gl_console_t * console);
int glConsoleHideCursor(gl_console_t * console);
int glConsoleDraw( gl_console_t * console);
int glConsoleClearScreen(gl_console_t * console);
int glConsoleClearBuffer(gl_console_t * console);
int glConsolePrintString(char * s, gl_console_t * console);
int glConsoleSetFlags(int flags, gl_console_t * console);
int glConsoleSetCursorStyle(int style_num, gl_console_t * console);
int glConsoleAlignCursorRight(gl_console_t * gl_console);
int glConsoleAlignCursorLeft(gl_console_t * gl_console);
int glConsoleAlignCursorUp(gl_console_t * gl_console);
int glConsoleAlignCursorDown(gl_console_t * gl_console);

int glConsoleMoveCursorForward(gl_console_t * gl_console);
int glConsoleMoveCursorBackward(gl_console_t * gl_console);
int glConsoleMoveCursorUp(gl_console_t * gl_console);
int glConsoleMoveCursorDown(gl_console_t * gl_console);
int glConsoleStartShell(gl_console_t * gl_console);
int glConsoleCopyBuffer(char * src, int len, gl_console_t * gl_console);
int glConsoleCursorToNextChar(char c, gl_console_t * gl_console);
gl_console_t * glConsoleCreate(int screen_width, int screen_height, int scroll_width, int scroll_height, 
								float start_x, float start_y, int font_descriptor);
int glConsoleDestroy(gl_console_t * console);

#endif /** !_GLCONSOLE_H */
