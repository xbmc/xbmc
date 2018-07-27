/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/XBMC_events.h"

class CApplication;

class CAppInboundProtocol
{
public:
  CAppInboundProtocol(CApplication &app);
  bool OnEvent(XBMC_Event &event);
  void SetRenderGUI(bool renderGUI);

protected:
  CApplication &m_pApp;
};
