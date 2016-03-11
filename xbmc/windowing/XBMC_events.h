#pragma once

/*
 *      SDL - Simple DirectMedia Layer
 *      Copyright (C) 1997-2009 Sam Lantinga
 *      Sam Lantinga
 *      slouken@libsdl.org
 *  
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/* Include file for SDL event handling */

#include "input/XBMC_keyboard.h"


/* General keyboard/mouse state definitions */
#define XBMC_RELEASED	0
#define XBMC_PRESSED	1

/* Event enumerations */
typedef enum {
       XBMC_NOEVENT = 0,        /* Unused (do not remove) */
       XBMC_ACTIVEEVENT,        /* Application loses/gains visibility */
       XBMC_KEYDOWN,            /* Keys pressed */
       XBMC_KEYUP,              /* Keys released */
       XBMC_MOUSEMOTION,        /* Mouse moved */
       XBMC_MOUSEBUTTONDOWN,    /* Mouse button pressed */
       XBMC_MOUSEBUTTONUP,      /* Mouse button released */
       XBMC_QUIT,               /* User-requested quit */
       XBMC_SYSWMEVENT,         /* System specific event */
       XBMC_VIDEORESIZE,        /* User resized video mode */
       XBMC_VIDEOMOVE,          /* User moved the window */
       XBMC_VIDEOEXPOSE,        /* Screen needs to be redrawn */
       XBMC_APPCOMMAND,         /* Media commands, such as WM_APPCOMMAND on Windows for media keys. */
       XBMC_TOUCH,
       XBMC_SETFOCUS,
       XBMC_USEREVENT,

       XBMC_MAXEVENT = 256      /* XBMC_EventType is represented as uchar */
} XBMC_EventType;

/* Application visibility event structure */
typedef struct XBMC_ActiveEvent {
	unsigned char type;	/* XBMC_ACTIVEEVENT */
	unsigned char gain;	/* Whether given states were gained or lost (1/0) */
	unsigned char state;	/* A mask of the focus states */
} XBMC_ActiveEvent;

/* Keyboard event structure */
typedef struct XBMC_KeyboardEvent {
	unsigned char type;	/* XBMC_KEYDOWN or XBMC_KEYUP */
	unsigned char which;	/* The keyboard device index */
	unsigned char state;	/* XBMC_PRESSED or XBMC_RELEASED */
	XBMC_keysym keysym;
} XBMC_KeyboardEvent;

/* Mouse motion event structure */
typedef struct XBMC_MouseMotionEvent {
	unsigned char type;	/* XBMC_MOUSEMOTION */
	unsigned char which;	/* The mouse device index */
	unsigned char state;	/* The current button state */
	uint16_t x, y;	/* The X/Y coordinates of the mouse */
	int16_t xrel;	/* The relative motion in the X direction */
	int16_t yrel;	/* The relative motion in the Y direction */
} XBMC_MouseMotionEvent;

/* Mouse button event structure */
typedef struct XBMC_MouseButtonEvent {
	unsigned char type;	/* XBMC_MOUSEBUTTONDOWN or XBMC_MOUSEBUTTONUP */
	unsigned char which;	/* The mouse device index */
	unsigned char button;	/* The mouse button index */
	unsigned char state;	/* XBMC_PRESSED or XBMC_RELEASED */
	uint16_t x, y;	/* The X/Y coordinates of the mouse at press time */
} XBMC_MouseButtonEvent;

/* The "window resized" event
   When you get this event, you are responsible for setting a new video
   mode with the new width and height.
 */
typedef struct XBMC_ResizeEvent {
	unsigned char type;	/* XBMC_VIDEORESIZE */
	int w;		/* New width */
	int h;		/* New height */
} XBMC_ResizeEvent;

typedef struct XBMC_MoveEvent {
	unsigned char type;	/* XBMC_VIDEOMOVE */
	int x;		/* New x position */
	int y;		/* New y position */
} XBMC_MoveEvent;

/* The "screen redraw" event */
typedef struct XBMC_ExposeEvent {
	unsigned char type;	/* XBMC_VIDEOEXPOSE */
} XBMC_ExposeEvent;

/* The "quit requested" event */
typedef struct XBMC_QuitEvent {
	unsigned char type;	/* XBMC_QUIT */
} XBMC_QuitEvent;

/* A user-defined event type */
typedef struct XBMC_UserEvent {
	unsigned char type;	/* XBMC_USEREVENT */
	int code;	/* User defined event code */
	void *data1;	/* User defined data pointer */
	void *data2;	/* User defined data pointer */
} XBMC_UserEvent;

/* If you want to use this event, you should include XBMC_syswm.h */
struct XBMC_SysWMmsg;
typedef struct XBMC_SysWMmsg XBMC_SysWMmsg;
typedef struct XBMC_SysWMEvent {
	unsigned char type;
	XBMC_SysWMmsg *msg;
} XBMC_SysWMEvent;

/* Multimedia keys on keyboards / remotes are mapped to APPCOMMAND events */
typedef struct XBMC_AppCommandEvent {
  unsigned char type; /* XBMC_APPCOMMAND */
  unsigned int action; /* One of ACTION_... */  
} XBMC_AppCommandEvent;

/* Mouse motion event structure */
typedef struct XBMC_TouchEvent {
  unsigned char type;   /* XBMC_TOUCH */
  int action;           /* action ID */
  float x, y;           /* The X/Y coordinates of the mouse */
  float x2, y2;         /* Additional X/Y coordinates */
  int pointers;         /* number of touch pointers */
} XBMC_TouchEvent;

typedef struct XBMC_SetFocusEvent {
	unsigned char type;	/* XBMC_SETFOCUS */
	int x;		/* x position */
	int y;		/* y position */
} XBMC_SetFocusEvent;

/* General event structure */
typedef union XBMC_Event {
  unsigned char type;
  XBMC_ActiveEvent active;
  XBMC_KeyboardEvent key;
  XBMC_MouseMotionEvent motion;
  XBMC_MouseButtonEvent button;
  XBMC_ResizeEvent resize;
  XBMC_MoveEvent move;
  XBMC_ExposeEvent expose;
  XBMC_QuitEvent quit;
  XBMC_UserEvent user;
  XBMC_SysWMEvent syswm;
  XBMC_AppCommandEvent appcommand;
  XBMC_TouchEvent touch;
  XBMC_SetFocusEvent focus;
} XBMC_Event;

