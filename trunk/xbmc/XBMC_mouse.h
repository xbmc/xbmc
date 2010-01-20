/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* Include file for SDL mouse event handling */

#ifndef _XBMC_mouse_h
#define _XBMC_mouse_h

#include "XBMC_stdinc.h"

/* Useful data types */
typedef struct XBMC_Rect {
  int16_t   x, y;
  uint16_t  w, h;
} XBMC_Rect;


typedef struct WMcursor WMcursor;	/* Implementation dependent */
typedef struct XBMC_Cursor {
	XBMC_Rect area;			/* The area of the mouse cursor */
	int16_t hot_x, hot_y;		/* The "tip" of the cursor */
	unsigned char *data;			/* B/W cursor data */
	unsigned char *mask;			/* B/W cursor mask */
	unsigned char *save[2];			/* Place to save cursor area */
	WMcursor *wm_cursor;		/* Window-manager cursor */
} XBMC_Cursor;

#endif /* _XBMC_mouse_h */
