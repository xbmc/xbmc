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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/
#pragma once

#include "WinEvents.h"
#include <X11/Xlib.h>
#include "threads/SystemClock.h"
#include <map>

class CWinEventsX11 : public CWinEventsBase
{
public:
  CWinEventsX11();
  virtual ~CWinEventsX11();
  static bool Init(Display *dpy, Window win);
  static void Quit();
  static bool HasStructureChanged();
  static void PendingResize(int width, int height);
  static void SetXRRFailSafeTimer(int millis);
  static bool MessagePump();

protected:
  static int Utf8ToUnicode(const char *utf8, const int utf8Length, uint16_t *utf16, const int utf16MaxLength);
  static XBMCKey LookupXbmcKeySym(KeySym keysym);
  static bool ProcessKey(XBMC_Event &event, int repeatDelay);
  static bool ProcessKeyRepeat();
  static bool ProcessShortcuts(XBMC_Event& event);
  static CWinEventsX11 *WinEvents;
  Display *m_display;
  Window m_window;
  Atom m_wmDeleteMessage;
  char *m_keybuf;
  uint16_t *m_utf16buf;
  XIM m_xim;
  XIC m_xic;
  XBMC_Event m_lastKey;
  XbmcThreads::EndTime m_repeatKeyTimeout;
  std::map<uint32_t,uint32_t> m_symLookupTable;
  int m_keymodState;
  bool m_structureChanged;
  int m_RREventBase;
  XbmcThreads::EndTime m_xrrFailSafeTimer;
  bool m_xrrEventPending;
};
