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
/**
 * $Id: glConsole.c,v 1.2 2005/12/28 18:51:18 psperl Exp $
 *
 * Encapsulation of an OpenGL-based console
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef MACOS
#include <unistd.h>
#endif /** MACOS */
#include <time.h>

#include "projectM.h"

#ifdef MACOS
#include <gl.h>
#else
#include <GL/gl.h>
#endif /** MACOS */
#include "glConsole.h"
#include "common.h"
#include "fatal.h"

#include "wipemalloc.h"

#ifndef _WIN32PC
#include <FTGL/FTGL.h>
#include <FTGL/FTGLPixmapFont.h>
#else
#include <FTGL.h>
#include <FTGLPixmapFont.h>
#endif

#define HEIGHT_SPACE 8 /* pixel space between lines */
#define DEFAULT_CONSOLE_FOREGROUND_COLOR CONSOLE_WHITE
#define DEFAULT_CONSOLE_BACKGROUND_COLOR CONSOLE_BLACK
#define DEFAULT_CURSOR_COLOR CONSOLE_GREEN
#define CURSOR_REFRESH_RATE 10

static int gl_console_set_color(color_t color);
int clear_console_screen_buffer(gl_console_t * gl_console);
int clear_console_scroll_buffer(gl_console_t * gl_console);
//float screen_width = 800;
//float screen_height = 600;

extern  FTGLPixmapFont *other_font;
extern projectM_t *PM;
int refresh_count = 0;


/* Creates a new console of (width X height) with left corner at
   (x, y) */
gl_console_t * glConsoleCreate(int screen_width, int screen_height, int scroll_width, int scroll_height,
								float start_x, float start_y, int Font_Descriptor) {

	console_char_t * buffer;
	gl_console_t * gl_console;

	if ((screen_width > MAX_CONSOLE_WIDTH) || (screen_width < 1))
		return NULL;

	if ((screen_height > MAX_CONSOLE_HEIGHT) || (screen_height < 1))
		return NULL;

	if ((gl_console = (gl_console_t*)wipemalloc(sizeof(gl_console_t))) == NULL)
		return NULL;

	if ((scroll_height < screen_height))
		return NULL;

	if ((scroll_width < screen_width))
		return NULL;

	if ((buffer = (console_char_t*)wipemalloc(scroll_width*scroll_height*sizeof(console_char_t))) == NULL)
	  return NULL;
	/* Need to know height and width of screen so we can see if the
	   if x and y are valid, will do later */

	/* Set struct parameters */
	gl_console->screen_buffer = buffer;
	gl_console->font_descriptor = Font_Descriptor;
	gl_console->screen_width = screen_width;
	gl_console->screen_height = screen_height;
	gl_console->scroll_width = scroll_width;
	gl_console->scroll_height = scroll_height;
	gl_console->start_x = start_x; /* x coordinate of console's upper left corner */
	gl_console->start_y = start_y; /* y coordinate of console's upper left corner */
	gl_console->screen_start_x = gl_console->screen_start_y = 0;
	gl_console->screen_x = gl_console->screen_y = 0; /* set cursor positions to zero */
	gl_console->cursor_ptr = buffer; /* start cursor pointer at beginning of buffer */
	gl_console->current_fg = DEFAULT_CONSOLE_FOREGROUND_COLOR;
	gl_console->current_bg = DEFAULT_CONSOLE_BACKGROUND_COLOR;
	gl_console->scroll_buffer = buffer;
	gl_console->cursor_color = DEFAULT_CURSOR_COLOR;
	gl_console->cursor_style = BAR_STYLE;
	gl_console->flags = 0;
	/* Clears the console buffer */
	clear_console_scroll_buffer(gl_console);

	if (GL_CONSOLE_DEBUG) printf("glConsoleCreate: finished initializing\n");

	/* Finished, return new console */
	return gl_console;
}

/* Destroy the passed console */
int glConsoleDestroy(gl_console_t * console) {

	if (console == NULL)
		return PROJECTM_FAILURE;

	free(console->scroll_buffer);
	free(console);

        console = NULL;

	return PROJECTM_SUCCESS;

}


int glConsoleClearScreen(gl_console_t * console) {

	if (console == NULL)
		return PROJECTM_FAILURE;
	
	clear_console_screen_buffer(console);
	
	return PROJECTM_SUCCESS;
}


int glConsoleClearBuffer(gl_console_t * console) {

	if (console == NULL)
		return PROJECTM_FAILURE;

	clear_console_scroll_buffer(console);
	glConsoleAlignCursorLeft(console);
	glConsoleAlignCursorUp(console);
	
	return PROJECTM_SUCCESS;
}

/* Aligns cursor to the next character 'c' */
int glConsoleCursorToNextChar(char c, gl_console_t * gl_console) {

  console_char_t * endofbuf;
  if (gl_console == NULL)
    return PROJECTM_FAILURE;

  endofbuf = gl_console->scroll_buffer + (gl_console->scroll_width*gl_console->scroll_height) - 1;

  while ((gl_console->cursor_ptr != endofbuf) && (gl_console->cursor_ptr->symbol != c))
    glConsoleMoveCursorForward(gl_console);

  return PROJECTM_SUCCESS;

}

int glConsoleMoveCursorUp(gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;

	/* The screen buffer is at the top and so is the screen cursor */
	if ((gl_console->screen_start_y == 0) && (gl_console->screen_y == 0)) {

		  /* Wrap up flag specified, move cursor to bottom of screen */
		  if (gl_console->flags & CONSOLE_FLAG_SCROLL_WRAP_UP) {
		    glConsoleAlignCursorDown(gl_console);
		  }

		  
		  /* Do nothing instead */

	}	
	/* The screen cursor is at the top but not the screen buffer */
	else if (gl_console->screen_y == 0){

		/* screen_y stays the same, since it scrolls up one */	
		
		/* Decrement the cursor_ptr by one line */
		gl_console->cursor_ptr -= gl_console->scroll_width;
		
		/* Decrement starting 'y' position of the screen buffer */
		gl_console->screen_start_y--;
		
		/* Decrement the screen buffer by one line */
		gl_console->screen_buffer -= gl_console->scroll_width;
		
		
	}	
	/* General case, not at top or bottoom */
	else {
		gl_console->screen_y--;
		gl_console->cursor_ptr -= gl_console->scroll_width;
	}
	
	return PROJECTM_SUCCESS;
}


int glConsoleMoveCursorDown(gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;

	/* The screen buffer is at the bottom and so is the screen cursor, this may not ever
	   happen */
	if (gl_console->screen_start_y == (gl_console->scroll_height - gl_console->screen_height)) {
		if (gl_console->screen_y == (gl_console->screen_height - 1)) {
		  //		  printf("BOTTOM AND END\n");
		  /* Wrap down to up enabled */
		  if (gl_console->flags & CONSOLE_FLAG_SCROLL_WRAP_DOWN) {
		    glConsoleAlignCursorUp(gl_console);
		  }

		  /* Otherwise, do nothing */
		}
	}	
	
	/* The screen cursor is at the bottom but not the end of the scroll buffer */
	else if (gl_console->screen_y == (gl_console->screen_height - 1)) {
		
	  //	  printf("BOTTOM BUT NOT END\n");
		/* screen_y stays the same, since it scrolls down one */	

		/* Increment the cursor_ptr by one line */
		gl_console->cursor_ptr += gl_console->scroll_width;
		
		/* Increment starting 'y' position of the screen buffer */
		gl_console->screen_start_y++;
		
		/* Increment the screen buffer by one line */
		gl_console->screen_buffer += gl_console->scroll_width;
		
		
	}
	
	/* General case, not at top or bottoom */
	else {
	  //	  printf("GENERAL CASE\n");
	  gl_console->screen_y++;
	  gl_console->cursor_ptr += gl_console->scroll_width;
	}

	return PROJECTM_SUCCESS;
}

/* Move the console forward one character, wrap around is the current behavior */
int glConsoleMoveCursorForward(gl_console_t * gl_console) {
	
	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	

	/* The case where the end of the screen is reached and the end of the scroll buffer */
	if ((gl_console->screen_start_x + gl_console->screen_width) == (gl_console->scroll_width)) {
		if ((gl_console->screen_x) == (gl_console->screen_width - 1)) {

		  /* Wrap around to next line if this flag is enabled */
		  if (gl_console->flags & CONSOLE_FLAG_SCROLL_WRAP_RIGHT) {
		    /* Down one and to the left */
		    glConsoleMoveCursorDown(gl_console);			
		    glConsoleAlignCursorLeft(gl_console);
		  }

		  /* Otherwise do nothing */
		}
	}
	
	/* The case where the end of the screen is reach, but the not end of the scroll buffer */
	else if (gl_console->screen_x == (gl_console->screen_width - 1)) {
	
		/* screen_x doesn't change because the whole console is shifted left */
		/* screen_y doesn't change because we haven't reached the end of the scroll buffer */
		
		gl_console->cursor_ptr++; /* increment cursor pointer */
		gl_console->screen_buffer++; /* move the screen buffer right one */
		gl_console->screen_start_x++; /* incrementing the start of the screen 'x' variable */
	}

	/* The most common case, no scrolling required. In other words, the cursor is at some 
	   arbitrary non end point position */	
	else {
		gl_console->screen_x++; /* increment x variable */
		gl_console->cursor_ptr++; /* increment cursor pointer */
	}
	
	return PROJECTM_SUCCESS;
}	


/* Moves the cursor backward one space */
int glConsoleMoveCursorBackward(gl_console_t * gl_console) {
	
	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	
	/* The case where the beginning of the screen is reached and the beginning of the scroll buffer */
	if ((gl_console->screen_start_x == 0) && (gl_console->screen_x == 0)) {
		  if (gl_console->flags & CONSOLE_FLAG_SCROLL_WRAP_LEFT) {
			glConsoleMoveCursorUp(gl_console);			
			glConsoleAlignCursorRight(gl_console);
		  }			
		
	}

	/* The case where the beginning of the screen is reach, but the not beginning of the scroll buffer */
	else if (gl_console->screen_x == 0) {
		gl_console->cursor_ptr--;     /* decrement cursor pointer */
		gl_console->screen_buffer--;  /* move the screen buffer left one */		
		gl_console->screen_start_x--; /* decrement the start of the screen 'x' variable */
	}		

	/* The most common case, no scrolling required. In other words, the cursor is at some
	   arbitrary non end point position */
	else {
		gl_console->screen_x--; /* increment x variable */
		gl_console->cursor_ptr--; /* increment cursor pointer */
	}

	/* Finised, return success */
	return PROJECTM_SUCCESS;
}

/* Sets the cursor position to (x,y) of the passed console */
int glConsoleSetCursorPos(int x, int y, gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;

	/* Bound checks */
	if ((x < 0) || (x > (gl_console->scroll_width-1)))
		return PROJECTM_FAILURE;

	if ((y < 0) || (y > (gl_console->scroll_height-1)))
		return PROJECTM_FAILURE;

       
	/* Set cursor ptr to new screen location. This variable does not depend on if the 
           screen needs to be scrolled */
	gl_console->cursor_ptr = gl_console->scroll_buffer + (y * gl_console->scroll_width) + x;

	/* Goal 1: Determine what needs to be changed on the X axis */


	/* Case (i): x coordinate is less than the starting screen buffer x coordinate. Must
	   shift the screen buffer left by difference of the two coordinates */
        if (x < gl_console->screen_start_x) {
	  //	  printf("X: case (i)\n");

	  /* Align x offset to new x corodinate */
	  gl_console->screen_start_x = x;

	  /* Set screen_x to zero because we are left edge aligned in the screen buffer */
	  gl_console->screen_x = 0;

	}

	/* Case (ii): x coordinate is greater than farthest screen buffer x coordinate. Must
	   shift the screen buffer right by difference of the two coordinates */
	else if (x > (gl_console->screen_start_x+gl_console->screen_width-1)) {
	  //	  printf("X: case (ii)\n");
	  
	  /* Align end of screen buffer X coordinate space to new x coordinate */
	  gl_console->screen_start_x = x - (gl_console->screen_width - 1);
	  
	  /* Set screen_x to right edge of screen buffer because we are right aligned to the new x coordinate */
	  gl_console->screen_x = gl_console->screen_width - 1;
	}

	/* CASE (iii): x coordinate must lie within the current screen buffer rectangle. No need to 
	   shift the screen buffer on the X axis */
        else {
	  //	  printf("X: case (iii)\n");

	  /* Set the new screen coordinate to the passed in x coordinate minus the starting screen buffer distance */
	  gl_console->screen_x = x - gl_console->screen_start_x;

	}


	/* Goal 2: Determine what needs to be changed on the Y axis */

	/* Case (i): y coordinate is less than the starting screen buffer y coordinate. Must
	   shift the screen buffer up by difference of the two coordinates */
        if (y < gl_console->screen_start_y) {
	  //	  printf("Y: case (i) y = %d, start_y = %d\n", y, gl_console->screen_start_y);

	  /* Align y offset to new y corodinate */
	  gl_console->screen_start_y = y;

	  /* Set screen_y to zero because we are top aligned in the screen buffer */
	  gl_console->screen_y = 0;

	}

	/* Case (ii): y coordinate is greater than loweest screen buffer y coordinate. Must
	   shift the screen buffer down by difference of the two coordinates */
	else if (y > (gl_console->screen_start_y+gl_console->screen_height-1)) {
	  //  	  printf("Y: case (ii)\n");

	  /* Align end of screen buffer Y coordinate space to new y coordinate */
	  gl_console->screen_start_y = y - (gl_console->screen_height-1);
	  
	  /* Set screen_y to bottom edge of screen buffer because we are bottom aligned to the new y coordinate */
	  gl_console->screen_y = gl_console->screen_height - 1;
	}

	/* CASE (iii): y coordinate must lie within the current screen buffer rectangle. No need to 
	   shift the screen buffer on the Y axis */

        else {
	  //	  printf("Y: case (iii)\n");	  
	  /* Set the new screen coordinate to the passed in y coordinate minus the starting screen buffer distance */
	  gl_console->screen_y = y - gl_console->screen_start_y;

	}


	/* Re-adjust screen buffer ptr based on computed screen starting coordinates */
	gl_console->screen_buffer = gl_console->scroll_buffer + (gl_console->screen_start_y*gl_console->screen_width)
	  + gl_console->screen_start_x;


	return PROJECTM_SUCCESS;
}

/* Sets 'x' and 'y' to the console coordinates of the current cursor position */
int glConsoleGetCursorPos(int * x, int  * y, gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;

	/* Null arguments passed, bail */
	if ((x == NULL) || (y == NULL))
		return PROJECTM_FAILURE;

	/* Set coordinates with appropiate offset */
	*x = gl_console->screen_x + gl_console->screen_start_x;
	*y = gl_console->screen_y + gl_console->screen_start_y;

	return PROJECTM_SUCCESS;
}

/* Helper function that changes the current color */
static int gl_console_set_color(color_t color) {

	switch (color) {
	
	case CONSOLE_BLACK:
	  glColor4f(0, 0, 0, 1);
	  break;
	case CONSOLE_RED:
	  glColor4f(1, 0, 0, 1);
	  break;
	case CONSOLE_GREEN:
	  glColor4f(0, 1, 0, 1);
	  break;
	case CONSOLE_WHITE:
	  glColor4f(1, 1, 1, 1);
	  break;
	case CONSOLE_BLUE:
	  glColor4f(0, 0, 1, 1);
	  break;
	case CONSOLE_TRANS:
	  glColor4f(0, 0, 0, 0);
	  break;
	default: /* hmm, use white I guess */
	  glColor4f(1, 1, 1, 1);
	}		
	
	return PROJECTM_SUCCESS;
}

/* Sets the passed console to a new foreground */
int glConsoleSetFGColor(color_t color, gl_console_t * gl_console) {
	
		if (gl_console == NULL)
			return PROJECTM_FAILURE;
		
		gl_console->current_fg = color;
		
		return PROJECTM_SUCCESS;
}

/* Sets the passed console to a new background */
int glConsoleSetBGColor(color_t color, gl_console_t * gl_console) {
	
		if (gl_console == NULL)
			return PROJECTM_FAILURE;
		
		gl_console->current_bg = color;
		
		return PROJECTM_SUCCESS;
}

/* Sets the cursor color */
int glConsoleSetCursorColor(color_t color, gl_console_t * gl_console) {

		if (gl_console == NULL)
			return PROJECTM_FAILURE;
		
		gl_console->cursor_color = color;
		
		return PROJECTM_SUCCESS;
}

/* Prints a string starting from the current cursor position */
int glConsolePrintString(char * s, gl_console_t * gl_console) {

	int len;
	int i;
	console_char_t console_char;
	char symbol;
	
	/* Null argument checks */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;

	if (s == NULL)
		return PROJECTM_FAILURE;
	
	/* Set the console struct to correct fg and bg values. 
	   The character value will change in the for loop */
	console_char.fg_color = gl_console->current_fg;
	console_char.bg_color = gl_console->current_bg;
	console_char.symbol = 0;
	
	/* Calculate length of the string */
	len = strlen(s);
	
	
	for (i = 0; i < len; i++) {

		/* Case on the type of character */
		
		switch (symbol = *(s+i)) {
			
		case '\r':
		case '\n':	  
		  console_char.symbol = symbol;
		  *gl_console->cursor_ptr = console_char;		  		  
		  glConsoleMoveCursorDown(gl_console);
		  glConsoleAlignCursorLeft(gl_console);
			break;
		case '\b':
			glConsoleMoveCursorBackward(gl_console);			
			break;
		default:
		  /* Change the screen_buffer value */
		  console_char.symbol = symbol;
		  *gl_console->cursor_ptr = console_char;
		  glConsoleMoveCursorForward(gl_console);
	      
		}		
	}	
	
	return PROJECTM_SUCCESS;	
}


/* Clears the console screen_buffer, using current fg and bg values */
int clear_console_screen_buffer(gl_console_t * gl_console) {
	
	int i, console_size;
	console_char_t console_char;
	
	/* Set console struct to current fg and bg values */
	console_char.fg_color = gl_console->current_fg;
	console_char.bg_color = gl_console->current_bg;
	console_char.symbol = 0; /* empty symbol */
	
	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	
	console_size = gl_console->scroll_width * gl_console->scroll_height;
	
	for (i = 0; i < console_size; i++)
		*(gl_console->screen_buffer + i) = console_char;
		
	return PROJECTM_SUCCESS;			
}

/* Clears the entire buffer */
int clear_console_scroll_buffer(gl_console_t * gl_console) {

	int i, console_size;
	console_char_t console_char;
	
	/* Set console struct to current fg and bg values */
	console_char.fg_color = gl_console->current_fg;
	console_char.bg_color = gl_console->current_bg;
	console_char.symbol = 0; /* empty symbol */

	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	
	console_size = gl_console->scroll_width * gl_console->scroll_height;
	
	for (i = 0; i < console_size; i++) 
		*(gl_console->scroll_buffer + i) = console_char;
		
	return PROJECTM_SUCCESS;			
}

/* Align the cursor all the way to the right of the passed console */
int glConsoleAlignCursorRight(gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	
	/* Set the cursor pointer to the rightmost end of the scroll buffer */
	gl_console->cursor_ptr += gl_console->scroll_width - (gl_console->screen_start_x + gl_console->screen_x) - 1;

	/* Move the screen buffer to the farthest right as possible */
	gl_console->screen_buffer += gl_console->scroll_width - gl_console->screen_start_x - gl_console->screen_width;

	/* Set the screen start 'x' value to length of the scroll buffer minus the length
	   of the screen buffer with a -1 offset to access the array correctly */
	gl_console->screen_start_x = gl_console->scroll_width - gl_console->screen_width;

	/* Set the screen_x cursor value to the screen width length minus an array adjustment offset */
	gl_console->screen_x = gl_console->screen_width - 1;
	

	return PROJECTM_SUCCESS;
} 

/* Align the cursor all the way to the right of the passed console */
int glConsoleAlignCursorLeft(gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	
	/* Set the cursor pointer to the left most end of the scroll buffer */
	gl_console->cursor_ptr -= (gl_console->screen_start_x + gl_console->screen_x);
	
	/* Set the screen buffer all the way to left */
	gl_console->screen_buffer -= gl_console->screen_start_x;

	/* Set the screen start x to the leftmost end of the scroll buffer */
	gl_console->screen_start_x = 0;
	
	/* Set the screen_x cursor value to the leftmost end of the screne buffer */
	gl_console->screen_x = 0;

	
	/* Finished, return success */
	return PROJECTM_SUCCESS;
}

/* Align the cursor to the top of the screen of the passed console */
int glConsoleAlignCursorUp(gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	
	/* Set the cursor pointer to the top of the buffer plus the x offset */
	gl_console->cursor_ptr = gl_console->scroll_buffer + gl_console->screen_start_x + gl_console->screen_x;

	/* Set the screen start y to the top of the scroll buffer */
	gl_console->screen_start_y = 0;

	/* Set the screen_y cursor value to the top of the screen buffer */
	gl_console->screen_y = 0;

	/* Set screen buffer to the top shifted lefted by the screen start value */
	gl_console->screen_buffer = gl_console->scroll_buffer + gl_console->screen_start_x;
	
	/* Finished, return success */
	return PROJECTM_SUCCESS;
}


/* Align the cursor to the top of the screen of the passed console */
int glConsoleAlignCursorDown(gl_console_t * gl_console) {

	/* Null argument check */
	if (gl_console == NULL)
		return PROJECTM_FAILURE;
	
	/* Set the cursor pointer to the bottom of the buffer plus the x offset */
	gl_console->cursor_ptr = gl_console->scroll_buffer + 
	          ((gl_console->scroll_height-1)*gl_console->scroll_width) 
                	  + gl_console->screen_start_x + gl_console->screen_x;
	
	/* Set the screen start y to the bottom of the scroll buffer mi screen_height */
	gl_console->screen_start_y = gl_console->scroll_height - gl_console->screen_height;

	/* Set the screen_y cursor value to the bottom of the screen buffer */
	gl_console->screen_y = gl_console->screen_height - 1;

	/* Set screen buffer to the bottom minus the height of the screen 
	   shifted left by the screen start value */
	
	gl_console->screen_buffer = gl_console->scroll_buffer + 
	  ((gl_console->scroll_height - gl_console->screen_height)
	   *gl_console->scroll_width) + gl_console->screen_start_x;	
	
	/* Finished, return success */
	return PROJECTM_SUCCESS;
}



int glConsoleDraw( gl_console_t * console) {

	int x,y;
	//float minx, miny, maxx, maxy;
	char * symbol;
	console_char_t * console_char;
	float start_x, start_y;

	//REMOVE SOON
	//int width=800,height=600;
	

	//	console->start_x=0;
	//	console->start_y=1;
       

	int width=1;
	int height=1;
     


	start_y = -(console->start_y - 1.0);
	start_x = console->start_x;

	/* Null argument check */
	if (console == NULL)
		return PROJECTM_FAILURE;

	symbol = (char*)wipemalloc(sizeof(char)+1);
	*symbol = *(symbol+1) = 0;

	/* First, push the gl matrix to save data */	
	glPushMatrix();
        
     
	/* Start rendering at the console's start position */
	glTranslatef(start_x,start_y,-1);
	//glTranslatef(0,0.5,-1);
	/* Assign a pointer to the start of the console buffer */
	console_char = console->screen_buffer;
	
	/* Iterate through entire console buffer, drawing each 
	   character one at a time with appropiate foreground and
	   background values. May not be the most efficient. */

	//  glScalef(8.0,8.0,0);
	
     
       
	float llx;           // The bottom left near most ?? in the x axis
        float lly;           // The bottom left near most ?? in the y axis
        float llz;           // The bottom left near most ?? in the z axis
        float urx;           // The top right far most ?? in the x axis
        float ury;           // The top right far most ?? in the y axis
        float urz;           // The top right far most ?? in the z axis       
        float advance;

	
        //Figure out size of one console unit  
	other_font->FaceSize(16*(PM->vh/512.0));
	advance=other_font->Advance("W");
	other_font->BBox("qpg_XT[",llx,lly,llx,urx,ury,urz);


        float invfix=1.0/512;
	llx*=invfix;lly*=invfix;llz*=invfix;
	urx*=invfix;ury*=invfix;urz*=invfix;
	advance=advance/PM->vw;

	glTranslatef(advance*0.5,lly-ury,0);

	for (y = 0; y < console->screen_height; y++) {

	  glPushMatrix();
#ifdef _WIN32PC
    char buffer[MAX_CONSOLE_WIDTH+1];
#else
    char buffer[console->screen_width+1];
#endif
    memset( buffer, '\0',sizeof(char) * (console->screen_width+1));

	  
    	  
	  for (x = 0; x < console->screen_width; x++) {

	    console_char = console->screen_buffer + (y*console->scroll_width) + x;
	    *symbol = console_char->symbol;


	      /* Draw the background color */
      	      if ((console->flags & CONSOLE_FLAG_ALWAYS_DRAW_BACKGROUND) || (*symbol != 0)) {

		/* Draw the background by drawing a rectangle */
	       	gl_console_set_color(console_char->bg_color);
		glRectf(llx, lly ,llx+advance, ury);

	      }

	    /* We are at the cursor position. See if the cursor is hidden or not and act accordingly */
	    if ((console_char == console->cursor_ptr) && (!(console->flags & CONSOLE_FLAG_CURSOR_HIDDEN))) {

	      /* Cursor is not hidden and blinking */
	      if (console->flags & CONSOLE_FLAG_CURSOR_BLINKING) {
		if (refresh_count % CURSOR_REFRESH_RATE)
		  gl_console_set_color(console->cursor_color);
		else
		  gl_console_set_color(console_char->bg_color);
	      }

	      /* Cursor is not hidden, and not blinking */
	      else {
		gl_console_set_color(console->cursor_color);
	      }

	      /* Draw the cursor according to the set style */
	      if (console->cursor_style == BAR_STYLE)
	      	glRectf(llx, lly, llx+advance, ury);
	      else if (console->cursor_style == UNDERLINE_STYLE) {
	      	glRectf(llx, lly, llx+advance, ury);
	      }
	    }

	    /* The cursor is not here or hidden, draw regular background (OLD COMMENT) */

	    /* Instead of the above, do nothing because we always draw the background before
	       printing the cursor to the screen */
	    else {

//	      if ((console->flags & CONSOLE_FLAG_ALWAYS_DRAW_BACKGROUND) || (*symbol != 0)) {

		/* Draw the background by drawing a rectangle */
//		gl_console_set_color(console_char->bg_color);
//		glRectf(-0.5, 1.0 ,1.0, -1.0);

	      //}
	    }

	    /* If the symbol is nonzero draw it */
	    if (*symbol != 0 && *symbol != '\n') {
	      buffer[x]=*symbol;   
              //gl_console_set_color(console_char->fg_color);          
	    }
	      /* Now shift to the right by the size of the character space */
	      glTranslatef(advance,0,0);

	      //	      console_char++;
	  }


	// glColor4f(0.0,1.0,1.0,1.0);
	  //glTranslatef(((lly-ury)*console->screen_width),0,0);
	  //glRasterPos2f(50,-50);
      
	 glPopMatrix();

	console_char = console->screen_buffer + (y*console->scroll_width + 1);
	gl_console_set_color(console_char->fg_color);
	glRasterPos2f(0,0);

        other_font->Render(buffer);
 
	 
	  /* New line, shift down the size of the symbol plus some extra space */
	 
		  
	  glTranslatef(0,(lly-ury), 0);
	}
	//glColor4f(1,1,1,1);
       
	/* Finished, pop the gl matrix and return success */
	glPopMatrix();
	free(symbol);
	refresh_count++;
	return PROJECTM_SUCCESS;	
}



int glConsoleSetFlags(int flags, gl_console_t * gl_console) {

  if (gl_console == NULL)
    return PROJECTM_FAILURE;

  gl_console->flags = gl_console->flags | flags;

  return PROJECTM_SUCCESS;
}

#ifdef NOGOOD
int glConsoleStartShell(gl_console_t * gl_console) {
  
  int pid1, pid2;
  char * s;
  char c;
  FILE * fs;

  if (gl_console == NULL)
    return PROJECTM_FAILURE;

  if ((s = wipemalloc(sizeof(char) + 512)) == NULL)
    return PROJECTM_FAILURE;

  memset(s, 0, 512);

#if !defined(MACOS) && !defined(WIN32)
  if ((pid1 = fork()) == 0) {
    printf("bash starting\n");
    execve("/bin/bash", NULL, NULL);
    printf("bash exiting..\n");
    exit(0);
  }

  if ((pid2 = fork()) == 0) {
    //fs = fopen("tempfile", "r");
    printf("about to loop on stdout\n");
    while (1) {
      //printf("^");//fflush(stdout);
      //ungetc(c, stdin);
      fread(s, 1, 1, stdout);
      *s = 'a';
      //printf("%c", *s);fflush(stdout);
      glConsolePrintString(s, gl_console);
    }
    //fclose(fs);
    printf("waiting for pid1 to exit\n");
    waitpid(pid1, NULL, 0);
    free(s);
    printf("pid1 exited\n");
    return PROJECTM_SUCCESS;
  }

  printf("bash should have started\n");
#endif /** !MACOS */
  return PROJECTM_SUCCESS;
}
#endif
/* Copies the console buffer into a character array */
int glConsoleCopyBuffer(char * src, int len, gl_console_t * gl_console) {

  int i;
  int j = 0;
  int size;
  char c;
  if (src == NULL)
    return PROJECTM_FAILURE;
  if (len < 0)
    return PROJECTM_FAILURE;
  if (gl_console == NULL)
    return PROJECTM_FAILURE;

  /* Clear the character space */
  memset(src, 0, len);
  
  size = gl_console->scroll_width*gl_console->scroll_height;

  for (i=0; ((i - j) < len) && (i < size); i++) {
    c = (gl_console->scroll_buffer + i)->symbol;

    /* We don't want to accidentally null terminate the string...*/
      if (c != 0)
	src[i - j] = c;
      else 
	j++;
      
      //    if (c != 0)
      //      src[i] = c;
      //    else
      //      src[i] = '\n';
  }

  /* Ensure the string actually ends */
  src[len - 1] = 0;

  return PROJECTM_SUCCESS;
}

/* Sets the cursor draw style */
int glConsoleSetCursorStyle(int style_num, gl_console_t * gl_console) {

	if (gl_console == NULL)
		return PROJECTM_FAILURE;

	gl_console->cursor_style = style_num;

	return PROJECTM_SUCCESS;
}
