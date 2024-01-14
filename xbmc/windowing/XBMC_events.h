/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Resolution.h"
#include "input/keyboard/XBMC_keyboard.h"

/* Event enumerations */
typedef enum
{
  XBMC_NOEVENT = 0, /* Unused (do not remove) */
  XBMC_KEYDOWN, /* Keys pressed */
  XBMC_KEYUP, /* Keys released */
  XBMC_KEYCOMPOSING_COMPOSING, /* A composed key (sequence) is under processing */
  XBMC_KEYCOMPOSING_FINISHED, /* A composed key is finished */
  XBMC_KEYCOMPOSING_CANCELLED, /* A composed key is cancelled */
  XBMC_MOUSEMOTION, /* Mouse moved */
  XBMC_MOUSEBUTTONDOWN, /* Mouse button pressed */
  XBMC_MOUSEBUTTONUP, /* Mouse button released */
  XBMC_QUIT, /* User-requested quit */
  XBMC_VIDEORESIZE, /* User resized video mode */
  XBMC_SCREENCHANGE, /* Window moved to a different screen */
  XBMC_VIDEOMOVE, /* User moved the window */
  XBMC_MODECHANGE, /* Video mode must be changed */
  XBMC_TOUCH,
  XBMC_BUTTON, /* Button (remote) pressed */
  XBMC_SETFOCUS,
  XBMC_USEREVENT,

  XBMC_MAXEVENT = 256 /* XBMC_EventType is represented as uchar */
} XBMC_EventType;

/* Keyboard event structure */
typedef struct XBMC_KeyboardEvent {
	XBMC_keysym keysym;
} XBMC_KeyboardEvent;

/* Mouse motion event structure */
typedef struct XBMC_MouseMotionEvent {
	uint16_t x, y;	/* The X/Y coordinates of the mouse */
} XBMC_MouseMotionEvent;

/* Mouse button event structure */
typedef struct XBMC_MouseButtonEvent {
	unsigned char button;	/* The mouse button index */
	uint16_t x, y;	/* The X/Y coordinates of the mouse at press time */
} XBMC_MouseButtonEvent;

/* The "window resized" event
   When you get this event, you are responsible for setting a new video
   mode with the new width and height.
 */
typedef struct XBMC_ResizeEvent {
	int w;		/* New width */
	int h;		/* New height */
} XBMC_ResizeEvent;

typedef struct XBMC_MoveEvent {
	int x;		/* New x position */
	int y;		/* New y position */
} XBMC_MoveEvent;

typedef struct XBMC_ScreenChangeEvent
{
  unsigned int screenIdx; /* The screen index */
} XBMC_ScreenChangeEvent;

struct XBMC_ModeChangeEvent
{
  RESOLUTION res;
};

/* The "quit requested" event */
typedef struct XBMC_QuitEvent {
} XBMC_QuitEvent;

/* A user-defined event type */
typedef struct XBMC_UserEvent {
	int code;	/* User defined event code */
	void *data1;	/* User defined data pointer */
	void *data2;	/* User defined data pointer */
} XBMC_UserEvent;

/* Multimedia keys on keyboards / remotes are mapped to APPCOMMAND events */
typedef struct XBMC_AppCommandEvent {
  unsigned int action; /* One of ACTION_... */
} XBMC_AppCommandEvent;

/* Mouse motion event structure */
typedef struct XBMC_TouchEvent {
  int action;           /* action ID */
  float x, y;           /* The X/Y coordinates of the mouse */
  float x2, y2;         /* Additional X/Y coordinates */
  float x3, y3;         /* Additional X/Y coordinates */
  int pointers;         /* number of touch pointers */
} XBMC_TouchEvent;

typedef struct XBMC_SetFocusEvent {
	int x;		/* x position */
	int y;		/* y position */
} XBMC_SetFocusEvent;

/* Button event structure */
typedef struct XBMC_ButtonEvent
{
  uint32_t button;
  uint32_t holdtime;
} XBMC_ButtonEvent;

/* General event structure */
typedef struct XBMC_Event {
  uint8_t type;
  union
  {
    XBMC_KeyboardEvent key;
    XBMC_MouseMotionEvent motion;
    XBMC_MouseButtonEvent button;
    XBMC_ResizeEvent resize;
    XBMC_MoveEvent move;
    XBMC_ModeChangeEvent mode;
    XBMC_QuitEvent quit;
    XBMC_UserEvent user;
    XBMC_AppCommandEvent appcommand;
    XBMC_TouchEvent touch;
    XBMC_ButtonEvent keybutton;
    XBMC_SetFocusEvent focus;
    XBMC_ScreenChangeEvent screen;
  };
} XBMC_Event;

