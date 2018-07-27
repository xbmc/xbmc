/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
