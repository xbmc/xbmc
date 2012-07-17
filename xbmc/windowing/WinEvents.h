/*
*      Copyright (C) 2005-2012 Team XBMC
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
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#ifndef WINDOW_EVENTS_H
#define WINDOW_EVENTS_H

#pragma once

#include "utils/StdString.h"
#include "XBMC_events.h"

typedef bool (* PHANDLE_EVENT_FUNC)(XBMC_Event& newEvent);

class CWinEventsBase
{
public:
  static PHANDLE_EVENT_FUNC m_pEventFunc;
};

#if   defined(TARGET_WINDOWS)
#include "windows/WinEventsWin32.h"
#define CWinEvents CWinEventsWin32

#elif defined(TARGET_DARWIN_OSX)
#include "osx/WinEventsOSX.h"
#define CWinEvents CWinEventsOSX

#elif defined(TARGET_DARWIN_IOS)
#include "osx/WinEventsIOS.h"
#define CWinEvents CWinEventsIOS

#elif defined(TARGET_ANDROID)
#include "android/WinEventsAndroid.h"
#define CWinEvents CWinEventsAndroid

#elif defined(TARGET_FREEBSD) && defined(HAS_SDL_WIN_EVENTS)
#include "WinEventsSDL.h"
#define CWinEvents CWinEventsSDL

#elif defined(TARGET_LINUX) && defined(HAS_SDL_WIN_EVENTS)
#include "WinEventsSDL.h"
#define CWinEvents CWinEventsSDL

#elif defined(TARGET_LINUX) && defined(HAS_X11_WIN_EVENTS)
#include "WinEventsX11.h"
#define CWinEvents CWinEventsX11

#elif defined(TARGET_LINUX) && defined(HAS_LINUX_EVENTS)
#include "WinEventsLinux.h"
#define CWinEvents CWinEventsLinux

#endif

#endif // WINDOW_EVENTS_H
