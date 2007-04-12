/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

///////////////////////////////////////////////////////////////
// Timer

#ifndef __XBTIMER_H__
#define __XBTIMER_H__
#pragma once

#include <xtl.h>
#include "xbdefines.h"

typedef VOID (CALLBACK TimerProcCallback)(
  HWND hwnd,     // handle of window for timer messages
  UINT uMsg,     // WM_TIMER message
  UINT idEvent,  // timer identifier
  DWORD dwTime   // current system time
);

typedef TimerProcCallback* TIMERPROC;

UINT SetTimer(
  HWND hWnd,              // handle of window for timer messages
  UINT nIDEvent,          // timer identifier
  UINT uElapse,           // time-out value
  TIMERPROC lpTimerFunc   // address of timer procedure
);

BOOL KillTimer(
  HWND hWnd,      // handle of window that installed timer
  UINT uIDEvent   // timer identifier
);
 

#endif // __XBTIMER_H__