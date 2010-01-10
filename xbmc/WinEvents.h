/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#ifndef WINDOW_EVENTS_H
#define WINDOW_EVENTS_H

#pragma once

#include "StdString.h"
#include "XBMC_events.h"

typedef bool (* PHANDLE_EVENT_FUNC)(XBMC_Event& newEvent);

class CWinEventsBase
{
public:
  static PHANDLE_EVENT_FUNC m_pEventFunc;
};

#ifdef _WIN32
#include "WinEventsWin32.h"
#define CWinEvents CWinEventsWin32
#endif

#ifdef _LINUX
#include "WinEventsSDL.h"
#define CWinEvents CWinEventsSDL
#endif


#endif // WINDOW_EVENTS_H
