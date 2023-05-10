/*
*  Copyright (C) 2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#pragma once

#include "xbmc/windowing/OSScreenSaver.h"

#include <webos-helpers/libhelpers.h>

namespace KODI::WINDOWING::WAYLAND
{

class COSScreenSaverWebOS : public IOSScreenSaver
{
public:
  ~COSScreenSaverWebOS() override;

  /**
   * Screensaver inhibition on webOS works by subscribing to the tvpower service. The service will
   * respond every few minutes with a timestamp that needs to be confirmed if the screensaver
   * should still be inhibited. If those responses are not answered the screensaver will be
   * uninhibited again
   */
  void Inhibit() override;
  void Uninhibit() override;

private:
  static bool OnScreenSaverAboutToStart(LSHandle* sh, LSMessage* reply, void* ctx);

  std::unique_ptr<HContext> m_requestContext;
};

} // namespace KODI::WINDOWING::WAYLAND
