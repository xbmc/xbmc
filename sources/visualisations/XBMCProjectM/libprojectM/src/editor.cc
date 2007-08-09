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
/* Editor written on top of glConsole */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "projectM.h"

#ifdef MACOS
#include <gl.h>
#else
#include <GL/gl.h>
#endif /** MACOS */
#include "common.h"
#include "fatal.h"
#include "event.h"

#include "preset_types.h"
#include "preset.h"

#include "glConsole.h"

#include "editor.h"
#include "interface_types.h"

#define MAX_BUFFER_SIZE 50000
#define KEY_REFRESH_RATE 2
#define KEY_DELAY_TIME 15

extern preset_t *active_preset;
extern interface_t current_interface;

typedef enum {
  OVERWRITE,
  INSERT
} edit_mode_t;

edit_mode_t edit_mode = OVERWRITE;

void refresh_from_cursor(char * s);
void save_cursor_pos();
void restore_cursor_pos();
void writeChar(char c);
void replace_char(char c);
void insert_char(char c);
void shift_buffer_right();
void insert_newline();
void shift_buffer_left();
void delete_newline();
void complete_refresh();

void moveCursorLeft();
void moveCursorRight();

int moveCursorUp();
int moveCursorDown();

int get_prev_newline_dist();
int get_next_newline_dist();
int key_delay_cnt = 0;
int move_to_next_newline();
int move_to_prev_newline();
void handle_home();
void handle_end();
void handle_pageup();
void handle_pagedown();

gl_console_t * editor_console = NULL;
int current_font = 0;
char editor_buffer[MAX_BUFFER_SIZE];
int key_refresh_cnt = 0;
int cursor_pos = 0;
int backup_pos = -1;
int cursorx = 0;
int cursory = 0;
int key_held_down = 0;
projectMKeycode last_sdl_key = (projectMKeycode) 0;
projectMEvent last_sdl_event;

void (*callfunc)(void*, void*) = NULL;

int loadEditor(char * string, void (*func)(), int screen_width, int screen_height, 
	       int scroll_width, int scroll_height, float start_x, float start_y) {

  if (string == NULL)
    return PROJECTM_FAILURE;

 

  if ((editor_console = 
       glConsoleCreate(screen_width, screen_height, scroll_width, scroll_height, start_x, start_y, current_font)) < 0)
    return PROJECTM_FAILURE;

  /* Set colors */
  glConsoleSetFGColor(CONSOLE_GREEN, editor_console);
  glConsoleSetBGColor(CONSOLE_BLACK, editor_console);
  glConsoleSetCursorColor(CONSOLE_RED, editor_console);

  /* Set flags */
  glConsoleSetFlags(CONSOLE_FLAG_CURSOR_BLINKING|CONSOLE_FLAG_ALWAYS_DRAW_BACKGROUND, editor_console);

  /* Clear the console buffer */
  //  glConsoleClearBuffer(editor_console);

  /* Print the characters of the passed string, realign cursor to top left */
  glConsolePrintString(string, editor_console);
  glConsoleAlignCursorLeft(editor_console);
  glConsoleAlignCursorUp(editor_console);

  /* Copy string into editor buffer */
  strncpy(editor_buffer, string, MAX_BUFFER_SIZE-1);
  cursor_pos = 0;

  callfunc = (void (*)(void*, void*))func;
  backup_pos = -1;
  edit_mode = OVERWRITE;
  glConsoleSetCursorStyle(BAR_STYLE, editor_console);
  return PROJECTM_SUCCESS;
}

int closeEditor() {

  
  current_font = 0;
  key_held_down = 0;

  glConsoleDestroy(editor_console);
  editor_console = NULL;
  callfunc = NULL;
  return PROJECTM_SUCCESS;
}


void key_helper( projectMKeycode key, projectMEvent event, projectMModifier modifier) {
  char c;
  
  switch(key) {
  case PROJECTM_K_INSERT:
      	if (edit_mode == OVERWRITE) {
	  edit_mode = INSERT;
	  glConsoleSetCursorStyle(UNDERLINE_STYLE, editor_console);
	}
	else {
	  edit_mode = OVERWRITE;
	  glConsoleSetCursorStyle(BAR_STYLE, editor_console);
	}
	break;
  case PROJECTM_K_RETURN:
    if (modifier== PROJECTM_KMOD_LCTRL) {
      callfunc(editor_buffer, active_preset);
    }
    else {
      writeChar('\n');
    }
    break;
  case PROJECTM_K_LCTRL:
    break;
  case PROJECTM_K_RIGHT:
    moveCursorRight();
    //	glConsoleMoveCursorForward(editor_console);
    break;
  case PROJECTM_K_LEFT:
    moveCursorLeft();
    //	glConsoleMoveCursorBackward(editor_console);
    break;
  case PROJECTM_K_UP:
    moveCursorUp();
    break;	
  case PROJECTM_K_DOWN:
    moveCursorDown();
    //	glConsoleMoveCursorDown(editor_console);
    break;
  case PROJECTM_K_PAGEUP:
    handle_pageup();
    //    glConsoleAlignCursorUp(editor_console);
    break;
  case PROJECTM_K_PAGEDOWN:
    handle_pagedown();
    //    glConsoleAlignCursorDown(editor_console);
    break;
  case PROJECTM_K_HOME:
    handle_home();
    //    glConsoleAlignCursorLeft(editor_console);
    break;
  case PROJECTM_K_END:
    handle_end();
    //    glConsoleAlignCursorRight(editor_console);
    break;
  case PROJECTM_K_LSHIFT:
    break;
  case PROJECTM_K_RSHIFT:
    break;
  case PROJECTM_K_CAPSLOCK:
    break;
  case PROJECTM_K_BACKSPACE:
    writeChar('\b');
    break;
    
  case PROJECTM_K_ESCAPE:
    closeEditor();
    current_interface = DEFAULT_INTERFACE;
    break;
    
  default: 
    /* All regular characters */
    c = (char)key;
  
    writeChar(c);
    break;
  }
}


void editor_key_handler( projectMEvent event, projectMKeycode keycode ) {
 
    switch( event ) {
      
    case PROJECTM_KEYUP:
      //      printf("KEY UP\n");
      key_held_down = 0;
      return;
      
    case PROJECTM_KEYDOWN:
      //      printf("KEY DOWN FIRST\n");
      key_held_down = 1;
      last_sdl_key = keycode;
      last_sdl_event = event;
      key_helper(last_sdl_key, event, (projectMModifier)0);
      key_delay_cnt = KEY_DELAY_TIME;
      return;
      
    default:
     
      break;
    }


  

  
  
}

void refreshEditor() {

  /* Refresh the editor console */
  glConsoleDraw(editor_console);
  
  /* Update keyboard related events */
  if (key_delay_cnt > 0)
    key_delay_cnt--;
  else if ((key_held_down) && ((key_refresh_cnt % KEY_REFRESH_RATE) == 0)) {
    key_helper(last_sdl_key, last_sdl_event, (projectMModifier)0);
  }
  
  key_refresh_cnt++;

}

void moveCursorRight() {

  /* Out of bounds check */
  if (cursor_pos >= (MAX_BUFFER_SIZE-1))
    return;

  if (editor_buffer[cursor_pos+1] == 0)
    return;
    
  /* If we are at a new line character jump down to next line */
  if (editor_buffer[cursor_pos] == '\n') {
    glConsoleAlignCursorLeft(editor_console);
    glConsoleMoveCursorDown(editor_console);

  }

  /* Otherwise just advance cursor forward */
  else
    glConsoleMoveCursorForward(editor_console);

  //printf("editor: %c\n", editor_buffer[cursor_pos]);
  /* Move cursor forward in editor buffer */
  cursor_pos++;

}

void moveCursorLeft() {

  /* Out of bounds check */
  if (cursor_pos <= 0)
    return;

  /* If the previous (left) character is a new line jump up a line and all the way to the end of character string */
  if (editor_buffer[cursor_pos-1] == '\n') {
    glConsoleMoveCursorUp(editor_console);
    glConsoleCursorToNextChar('\n', editor_console);
  }
  
  /* Otherwise just move cursor back a space */
  else
    glConsoleMoveCursorBackward(editor_console);

  /* Move cursor forward in editor buffer */
  cursor_pos--;

}


int moveCursorUp() {


  int dist1, dist2;
  int return_flag = PROJECTM_SUCCESS;
  
  /* We need this distance to:
     1) move the cursor back to the previous new line
     2) position the cursor at the same column on the previous line
  */

  /* Move cursor to previous newline character */
  if (cursor_pos == 0)
    return PROJECTM_FAILURE;
  
  dist1 = move_to_prev_newline();
  
  if (cursor_pos == 0)
    return_flag = PROJECTM_FAILURE;
  else {
    moveCursorLeft();
    /* Again move to previous newline */
    dist2 = move_to_prev_newline();
  }

  /* Move cursor forward appropiately, stopping prematurely if new line is reached */
  while ((dist1 > 1) && (editor_buffer[cursor_pos] !='\n') && (cursor_pos <= (MAX_BUFFER_SIZE-1))) {
    cursor_pos++;
    glConsoleMoveCursorForward(editor_console);
    dist1--;
  }
  
  return return_flag;
}


int moveCursorDown() {

  int dist1, dist2;

  dist2 = get_prev_newline_dist();
  
  //printf("prev new line distance: %d\n", dist2);
  /* Move cursor to next line, store the distance
     away from the newline. If this distance is 
     less than (error value) or equal to zero do nothing */
  if ((dist1 = move_to_next_newline()) <= 0) {
    return PROJECTM_FAILURE;
  }
  //  printf("distance away from next newline: %d\n", dist1);
  while ((cursor_pos != (MAX_BUFFER_SIZE-1)) && (editor_buffer[cursor_pos] != '\n') && (dist2 > 0)) {
    cursor_pos++;
    glConsoleMoveCursorForward(editor_console);
    dist2--;
  }

  return PROJECTM_SUCCESS;
;
}

int get_prev_newline_dist() {

  int cnt = 0;

  if (cursor_pos == 0)
    return 0;

  /* If we are already at the newline, then skip the first character
     and increment cnt */
  if (editor_buffer[cursor_pos] == '\n') {
    /* Top of buffer, return 0 */
    if (cursor_pos == 0)
      return 0;
    /* Otherwise set count to one */
    cnt++;
  }
  while (editor_buffer[cursor_pos-cnt] != '\n') {
    
    /* In this case we are the top of the editor buffer, so stop */
    if ((cursor_pos-cnt) <= 0)
      return cnt;
    
    cnt++;
    
  } 

  //printf("Returning %d\n", cnt-1);
  return cnt-1;
}


int get_next_newline_dist() {

  int cnt = 0;


  while (editor_buffer[cursor_pos+cnt] != '\n') {
    
    /* In this case we have exceeded the cursor buffer, so abort action */		
    if ((cursor_pos+cnt) >= (MAX_BUFFER_SIZE-1))
      return 0;
    
		cnt++;
		
  }
    
  return cnt;
}

/* Moves cursor to next line, returns length away from next one.
   If this is the last line, returns 0.
   On error returns -1 (PROJECTM_FAILURE)
*/

int move_to_next_newline() {

	int cnt = 0;


	while (editor_buffer[cursor_pos+cnt] != '\n') {
	
		/* In this case we have exceeded the cursor buffer, so abort action */		
		if ((cursor_pos+cnt) >= (MAX_BUFFER_SIZE-1))
			return 0;
			
		cnt++;

	}

	
	/* New line is the last character in buffer, so quit */
	if ((cursor_pos+cnt+1) > (MAX_BUFFER_SIZE-1))
		return 0;

	if (editor_buffer[cursor_pos+cnt+1] == 0)
	  return 0;

	/* One more time to skip past new line character */
	cnt++;

	/* Now we can move the cursor position accordingly */
	cursor_pos += cnt;
	
	/* Now move the console cursor to beginning of next line
       These functions are smart enough to not exceed the console
	   without bounds checking */
	glConsoleMoveCursorDown(editor_console);
	glConsoleAlignCursorLeft(editor_console);

	/* Finally, return distance cursor was away from the new line */
	return cnt;
}

/* Moves cursor to previous line, returns length away from previous one.
   If this is the first line, returns 0.
   On error returns -1 (PROJECTM_FAILURE)
   More specifically, the cursor will be exactly at the new line character
   of the previous line.
   Now its the beginning of the line, not the newline 
*/

int move_to_prev_newline() {

	int cnt = 0;

	if (editor_buffer[cursor_pos] == '\n') {
	  if (cursor_pos == 0)
	    return 0;
	  else {
	    cnt++;
	    glConsoleMoveCursorBackward(editor_console);
	  }
	}
	while (((cursor_pos - cnt) > -1) && (editor_buffer[cursor_pos-cnt] != '\n')) {


	  glConsoleMoveCursorBackward(editor_console);

	  cnt++;

	} 

	//for (i=0;i < cnt; i++)

	
	/* New line is the last character in buffer, so quit */
	//	if ((cursor_pos-cnt-1) <= 0)
	//	return 0;
	

	/* Now we can move the cursor position accordingly */
	cursor_pos -= cnt-1;

	/* Now move the console cursor to end of previous line
	   These functions are smart enough to not exceed the console 
	   without bounds checking */  ;
	//	glConsoleMoveCursorUp(editor_console);
	//	glConsoleCursorToNextChar('\n', editor_console);

	/* Finally, return distance cursor was away from the new line */
	return cnt;

}

void handle_return() {


}


void handle_backspace() {


}

void handle_home() {

  while ((cursor_pos > 0) && (editor_buffer[cursor_pos-1] != '\n')) {
    cursor_pos--;
  } 

  glConsoleAlignCursorLeft(editor_console);

}

void handle_end() {
  
  while ((cursor_pos < (MAX_BUFFER_SIZE-1)) && (editor_buffer[cursor_pos+1] != 0) && (editor_buffer[cursor_pos] != '\n')) {
    cursor_pos++;
    glConsoleMoveCursorForward(editor_console);
  } 

  //  glConsoleCursorToNextChar('\n', editor_console);

}

void handle_pageup() {

  int dist1;

  dist1 = move_to_prev_newline();

  while (cursor_pos != 0)
    moveCursorLeft();
  
  while ((dist1 > 1) && (cursor_pos < (MAX_BUFFER_SIZE-1)) && (editor_buffer[cursor_pos+1] != 0) && (editor_buffer[cursor_pos] != '\n')) {
    cursor_pos++;
    glConsoleMoveCursorForward(editor_console);
    dist1--;
  } 

}


void handle_pagedown() {

  int dist1;

  dist1 = get_prev_newline_dist();

  while (cursor_pos < (MAX_BUFFER_SIZE-2) && (editor_buffer[cursor_pos+1] != 0))
    moveCursorRight();
  
  move_to_prev_newline();
  moveCursorRight();

 while ((dist1 > 1) && (cursor_pos < (MAX_BUFFER_SIZE-1)) && (editor_buffer[cursor_pos+1] != 0) && (editor_buffer[cursor_pos] != '\n')) {
   cursor_pos++;
   glConsoleMoveCursorForward(editor_console);
   dist1--;
  } 

}


/* Writes a character to console and editor according to editor mode */
void writeChar(char c) {

  switch (edit_mode) {
	/* Overwrite mode, replaces cursor character with passed character.
	   Cursor remains standing */
	case OVERWRITE:

	/* Case on c to catch special characters */
	switch (c) {

		case '\b': /* Backspace */
		  //			printf("backspace case, overwrite mode:\n");
			/* At beginning of buffer, do nothing */
			if (cursor_pos == 0)
				return;

			/* At first character of current line.
			   Default behavior is to delete the new line,
			   and squeeze the rest of the editor buffer back one character */
			if (editor_buffer[cursor_pos-1] == '\n') {
			  delete_newline();
			  return;
			}

         		/* Normal overwrite back space case.
  			   Here the previous character is replaced with a space,
			   and the cursor moves back one */
			
			editor_buffer[--cursor_pos]= ' ';
			(editor_console->cursor_ptr-1)->symbol = ' ';
			glConsoleMoveCursorBackward(editor_console);
			return;
		case '\n': /* New line character */
		  insert_newline();
		  return;
		default: /* not a special character, do typical behavior */
		  //		printf("DEFAULT CASE: char = %c\n", c);

			/* If cursor is sitting on the new line, then we
			   squeeze the pressed character between the last character
			   and the new line */
			if (editor_buffer[cursor_pos] == '\n') {
				insert_char(c);
				return;
			}

			/* Otherwise, replace character in editor buffer */
			replace_char(c);
			return;
	}
	
	return;

  case INSERT: /* Insert Mode */
    switch (c) {
    case '\b': /* Backspace case */

     
      if (editor_buffer[cursor_pos-1] == '\n') {
	delete_newline();
	return;
      }
    
      shift_buffer_left();
      cursor_pos--;
      glConsoleMoveCursorBackward(editor_console);
      refresh_from_cursor(editor_buffer+cursor_pos);
      return;
      
    case '\n':
      insert_newline();
      return;

    default:
      //      printf("insert_char: char = %c\n", c);
      insert_char(c);
      return;
    }
  default: /* Shouldn't happen */
    return;
    
    
    
    
  }
  



}

void delete_newline() {


  if (cursor_pos == 0)
    return;

  /* Move console cursor to previous line's end of character */
  glConsoleMoveCursorUp(editor_console);
  glConsoleCursorToNextChar('\n', editor_console);
  
  shift_buffer_left();
  cursor_pos--;

  /* Lazy unoptimal refresh here  */
  complete_refresh();
  return;



}

/* Refreshes entire console but retains initial cursor position */
void complete_refresh() {

  int sx, sy;

  glConsoleGetCursorPos(&sx, &sy, editor_console);
  
  glConsoleSetCursorPos(0, 0, editor_console);

  glConsoleClearBuffer(editor_console);
  glConsolePrintString(editor_buffer, editor_console);
  glConsoleSetCursorPos(sx, sy, editor_console);


}
/* Helper function to insert a newline and adjust the graphical console
   accordingly. Behavior is same regardless of edit mode type */
void insert_newline() {

 
  /* End of buffer, deny request */
  if (cursor_pos == (MAX_BUFFER_SIZE-1))
      return;

  shift_buffer_right();
  editor_buffer[cursor_pos] = '\n';
  
  /* Lazy unoptimal refresh */
  complete_refresh();

  cursor_pos++;
  glConsoleAlignCursorLeft(editor_console);
  glConsoleMoveCursorDown(editor_console);
}

/* Helper function to insert a character. Updates the console and editor buffer
   by inserting character at cursor position with passed argument, and moving
   the cursor forward if possible */
void insert_char(char c) {

	/* End of editor buffer, just replace the character instead */
	if (cursor_pos == (MAX_BUFFER_SIZE-1)) {
		editor_buffer[cursor_pos] = c;
		editor_console->cursor_ptr->symbol = c;
		return;
	}

	//printf("EDITOR BUFFER WAS:\n%s\n-----------------------------------\n", editor_buffer+cursor_pos);

	/* Shift contents of editor buffer right */
	shift_buffer_right();


	/* Now place the passed character at the desired location */
	editor_buffer[cursor_pos] = c;

	//	printf("EDITOR BUFFER IS NOW:\n%s\n-----------------------------------\n", editor_buffer+cursor_pos);

	/* Refresh console from current cursor position */
	refresh_from_cursor(editor_buffer+cursor_pos);

	/* Move cursor forward */
	cursor_pos++;
	glConsoleMoveCursorForward(editor_console);

}

/* Helper function to replace a character. Updates the console and editor buffer
   by replacing character at cursor position with passed argument, and
   moving the cursor forward if possible */
void replace_char(char c) {

	/* End of buffer, replace character but dont go forward */
	if (cursor_pos == (MAX_BUFFER_SIZE-1)) {
		editor_buffer[cursor_pos] = c;
		editor_console->cursor_ptr->symbol = c;
		return;
	}

	/* Replace character, move cursor forward one */
	editor_buffer[cursor_pos++] = c;
	editor_console->cursor_ptr->symbol = c;
	glConsoleMoveCursorForward(editor_console);

}


void save_cursor_pos() {

	backup_pos = cursor_pos;
	glConsoleGetCursorPos(&cursorx, &cursory, editor_console);
	//	printf("cursor_x: %d, cursor_y: %d\n", cursorx, cursory);
}

void restore_cursor_pos() {

	if (backup_pos == -1)
		return;
	cursor_pos = backup_pos;
	glConsoleSetCursorPos(cursorx, cursory, editor_console);
	backup_pos = -1;
}

/* Primarily for optimization, this function refreshs the editor console from the cursor position onward
   rather than the entire screen buffer */
void refresh_from_cursor(char * s) {

	if (s == NULL)
		return;

	save_cursor_pos();
	glConsolePrintString(s, editor_console);
	restore_cursor_pos();
}


/* Shifts editor buffer right from current cursor position */
void shift_buffer_right() {

	int i;
	char backup, backup2;

	if (cursor_pos == (MAX_BUFFER_SIZE-1))
		return;

	backup = editor_buffer[cursor_pos];

	for (i = cursor_pos; i < (MAX_BUFFER_SIZE-1); i++) {
		backup2 = editor_buffer[i+1];
		editor_buffer[i+1] = backup;
		backup = backup2;
	}

}

/* Shifts editor buffer left from current position */
void shift_buffer_left() {


	int i;
	char backup, backup2;

	if (cursor_pos == 0)
		return;

	//printf("cursor at: %c\n", editor_buffer[cursor_pos]);
	backup = editor_buffer[MAX_BUFFER_SIZE-1];

	//printf("shift_buffer_left: [before]\n%s\n", editor_buffer);

	for (i = MAX_BUFFER_SIZE-1; i >= cursor_pos; i--) {
		backup2 = editor_buffer[i-1];
		editor_buffer[i-1] = backup;
		backup = backup2;
	}
	//	printf("shift_buffer_left: [after]\n%s\n", editor_buffer);

}
