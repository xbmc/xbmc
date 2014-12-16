/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#pragma once

#ifndef WINDOW_EVENTS_SDL_H
#define WINDOW_EVENTS_SDL_H

#include "system.h"

#ifdef HAS_SDL
#if SDL_VERSION == 1
#include <SDL/SDL_events.h>
#elif SDL_VERSION == 2
#include <SDL/SDL_events.h>
#endif

#include "WinEvents.h"

class CWinEventsSDL : public IWinEvents
{
public:
  virtual bool MessagePump();
  virtual size_t GetQueueSize();

private:
#ifdef TARGET_DARWIN
  static bool ProcessOSXShortcuts(SDL_Event& event);
#elif defined(TARGET_POSIX)
  static bool ProcessLinuxShortcuts(SDL_Event& event);
#endif
};

#endif
#endif // WINDOW_EVENTS_SDL_H
