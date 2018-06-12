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

#include "xbmc/windowing/WinEvents.h"
#include "xbmc/windowing/X11/WinSystemX11.h"
#include <X11/Xlib.h>
#include "threads/SystemClock.h"

#include <clocale>
#include <map>

class CWinEventsX11 : public IWinEvents
{
public:
  CWinEventsX11(CWinSystemX11& winSystem);
  virtual ~CWinEventsX11();
  bool MessagePump() override;
  bool Init(Display *dpy, Window win);
  void Quit();
  bool HasStructureChanged();
  void PendingResize(int width, int height);
  void SetXRRFailSafeTimer(int millis);

protected:
  XBMCKey LookupXbmcKeySym(KeySym keysym);
  bool ProcessKey(XBMC_Event &event);
  Display *m_display = nullptr;
  Window m_window = 0;
  Atom m_wmDeleteMessage;
  char *m_keybuf = nullptr;
  size_t m_keybuf_len = 0;
  XIM m_xim = nullptr;
  XIC m_xic = nullptr;
  std::map<uint32_t,uint32_t> m_symLookupTable;
  int m_keymodState;
  bool m_structureChanged;
  int m_RREventBase;
  XbmcThreads::EndTime m_xrrFailSafeTimer;
  bool m_xrrEventPending;
  CWinSystemX11& m_winSystem;
};
