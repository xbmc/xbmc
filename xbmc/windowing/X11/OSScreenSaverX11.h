/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../OSScreenSaver.h"
#include "threads/Timer.h"

#include <X11/Xlib.h>

class COSScreenSaverX11 : public KODI::WINDOWING::IOSScreenSaver
{
public:
  explicit COSScreenSaverX11(Display* dpy);
  void Inhibit() override;
  void Uninhibit() override;

private:
  void ResetScreenSaver();

  Display* m_dpy;
  CTimer m_screensaverResetTimer;
};
