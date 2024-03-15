/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "windowing/XBMC_events.h"

#include <deque>

class CApplication;

class CAppInboundProtocol
{
  friend class CApplication;

public:
  CAppInboundProtocol(CApplication& app);
  bool OnEvent(const XBMC_Event& newEvent);
  void SetRenderGUI(bool renderGUI);
  void Close();

protected:
  void HandleEvents();

  bool m_closed = false;
  CApplication &m_pApp;
  std::deque<XBMC_Event> m_portEvents;
  CCriticalSection m_portSection;
};
