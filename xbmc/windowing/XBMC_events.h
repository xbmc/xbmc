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

/* Include file for SDL event handling */

#ifndef _XBMC_events_h
#define _XBMC_events_h

#include "input/XBMC_keyboard.h"
#include "input/XBMC_mouse.h"


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
       XBMC_JOYAXISMOTION,      /* Joystick axis motion */
       XBMC_JOYBALLMOTION,      /* Joystick trackball motion */
       XBMC_JOYHATMOTION,       /* Joystick hat position change */
       XBMC_JOYBUTTONDOWN,      /* Joystick button pressed */
       XBMC_JOYBUTTONUP,        /* Joystick button released */
       XBMC_QUIT,               /* User-requested quit */
       XBMC_SYSWMEVENT,         /* System specific event */
       XBMC_VIDEORESIZE,        /* User resized video mode */
       XBMC_VIDEOMOVE,          /* User moved the window */
       XBMC_VIDEOEXPOSE,        /* Screen needs to be redrawn */
       XBMC_APPCOMMAND,         /* Media commands, such as WM_APPCOMMAND on Windows for media keys. */
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

/* Joystick axis motion event structure */
typedef struct XBMC_JoyAxisEvent {
	unsigned char type;	/* XBMC_JOYAXISMOTION */
	unsigned char which;	/* The joystick device index */
	unsigned char axis;	/* The joystick axis index */
	int16_t value;	/* The axis value (range: -32768 to 32767) */
} XBMC_JoyAxisEvent;

/* Joystick trackball motion event structure */
typedef struct XBMC_JoyBallEvent {
	unsigned char type;	/* XBMC_JOYBALLMOTION */
	unsigned char which;	/* The joystick device index */
	unsigned char ball;	/* The joystick trackball index */
	int16_t xrel;	/* The relative motion in the X direction */
	int16_t yrel;	/* The relative motion in the Y direction */
} XBMC_JoyBallEvent;

/* Joystick hat position change event structure */
typedef struct XBMC_JoyHatEvent {
	unsigned char type;	/* XBMC_JOYHATMOTION */
	unsigned char which;	/* The joystick device index */
	unsigned char hat;	/* The joystick hat index */
	unsigned char value;	/* The hat position value:
			    XBMC_HAT_LEFTUP   XBMC_HAT_UP       XBMC_HAT_RIGHTUP
			    XBMC_HAT_LEFT     XBMC_HAT_CENTERED XBMC_HAT_RIGHT
			    XBMC_HAT_LEFTDOWN XBMC_HAT_DOWN     XBMC_HAT_RIGHTDOWN
			   Note that zero means the POV is centered.
			*/
} XBMC_JoyHatEvent;

/* Joystick button event structure */
typedef struct XBMC_JoyButtonEvent {
	unsigned char type;	/* XBMC_JOYBUTTONDOWN or XBMC_JOYBUTTONUP */
	unsigned char which;	/* The joystick device index */
	unsigned char button;	/* The joystick button index */
	unsigned char state;	/* XBMC_PRESSED or XBMC_RELEASED */
} XBMC_JoyButtonEvent;

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

/* General event structure */
typedef union XBMC_Event {
  unsigned char type;
  XBMC_ActiveEvent active;
  XBMC_KeyboardEvent key;
  XBMC_MouseMotionEvent motion;
  XBMC_MouseButtonEvent button;
  XBMC_JoyAxisEvent jaxis;
  XBMC_JoyBallEvent jball;
  XBMC_JoyHatEvent jhat;
  XBMC_JoyButtonEvent jbutton;
  XBMC_ResizeEvent resize;
  XBMC_MoveEvent move;
  XBMC_ExposeEvent expose;
  XBMC_QuitEvent quit;
  XBMC_UserEvent user;
  XBMC_SysWMEvent syswm;
  XBMC_AppCommandEvent appcommand;
} XBMC_Event;

#endif /* _XBMC_events_h */
