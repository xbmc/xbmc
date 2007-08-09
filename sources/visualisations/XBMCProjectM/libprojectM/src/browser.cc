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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "fatal.h"

#include "preset_types.h"
#include "preset.h"

#include "glConsole.h"

#include "interface_types.h"

#include "event.h"

extern interface_t current_interface;

gl_console_t * browser_console = NULL;
int active_font2 = 0;
char input_buffer[MAX_PATH_SIZE];
int buf_pos = 0;

int loadBrowser() { 

 

  if ((browser_console = glConsoleCreate(40, 10, 80, 20, 1, 1, active_font2)) < 0)
    return PROJECTM_FAILURE;

  glConsoleSetFGColor(CONSOLE_RED, browser_console);
  glConsoleSetBGColor(CONSOLE_BLACK, browser_console);
  glConsoleSetFlags(CONSOLE_FLAG_CURSOR_BLINKING | CONSOLE_FLAG_ALWAYS_DRAW_BACKGROUND, browser_console);
  glConsoleClearBuffer(browser_console);

  buf_pos = 0;
  memset(input_buffer, 0, MAX_PATH_SIZE);

  return PROJECTM_SUCCESS;
}

int closeBrowser() { 

  
  active_font2 = 0;

  glConsoleDestroy(browser_console);
  browser_console = NULL;

  return PROJECTM_SUCCESS;
}

void browser_key_handler( projectMEvent event, projectMKeycode keycode, projectMModifier modifier ) {

  char s[2];
  
  s[0] = 0;
  s[1] = 0;
 
    
    switch( event ) {
    case PROJECTM_KEYDOWN:	 		    
      switch(keycode) {
          case PROJECTM_K_UP:
	    glConsoleMoveCursorUp(browser_console);
	    break;
          case PROJECTM_K_RETURN:
	    loadPresetByFile(input_buffer);
	    closeBrowser();
	    current_interface = DEFAULT_INTERFACE;
	    break;
          case PROJECTM_K_RIGHT:
	    glConsoleMoveCursorForward(browser_console);
	    break;
          case PROJECTM_K_LEFT:
	    printf("CURSOR BACKWARD\n");
	    glConsoleMoveCursorBackward(browser_console);
	    break;
          case PROJECTM_K_DOWN:
	    glConsoleMoveCursorDown(browser_console);
	    break;
          case PROJECTM_K_PAGEUP:
	    glConsoleAlignCursorUp(browser_console);
	    break;
          case PROJECTM_K_PAGEDOWN:
	    glConsoleAlignCursorDown(browser_console);
	    break;
          case PROJECTM_K_INSERT:
	    glConsoleAlignCursorLeft(browser_console);
	    break;
          case PROJECTM_K_DELETE:
	    glConsoleAlignCursorRight(browser_console);
	    break;
          case PROJECTM_K_LSHIFT:
	    break;
          case PROJECTM_K_RSHIFT:
	    break;
          case PROJECTM_K_CAPSLOCK:
	    break;
          case PROJECTM_K_ESCAPE:
	    closeBrowser();
	    current_interface = DEFAULT_INTERFACE;
	    break;	     
   	    
          default: /* All regular characters */
	    if (buf_pos == MAX_PATH_SIZE) {
	      buf_pos = 0; 
	    }
	    input_buffer[buf_pos] = (char)keycode;
	    if ((modifier == PROJECTM_KMOD_LSHIFT) || (modifier == PROJECTM_KMOD_RSHIFT) || (modifier == PROJECTM_KMOD_CAPS))
	      input_buffer[buf_pos] -= 32;

	    *s = input_buffer[buf_pos];
	    glConsolePrintString(s, browser_console);
	    buf_pos++;
	    break;
      }
    }
  
  
}

void refreshBrowser() {

  // glConsoleClearBuffer(browser_console);
  //  glConsoleSetCursorPos(1, 1, browser_console);
  //glConsolePrintString("Enter a file to load:\n", browser_console);
  //glConsolePrintString(input_buffer, browser_console);
  glConsoleDraw(browser_console);
}

