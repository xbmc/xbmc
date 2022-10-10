/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/windows/GUIWindowPVRTimersBase.h"

#include <string>

namespace PVR
{
  class CGUIWindowPVRTVTimerRules : public CGUIWindowPVRTimersBase
  {
  public:
    CGUIWindowPVRTVTimerRules();
    ~CGUIWindowPVRTVTimerRules() override = default;

  protected:
    std::string GetRootPath() const override;
    std::string GetDirectoryPath() override;
  };

  class CGUIWindowPVRRadioTimerRules : public CGUIWindowPVRTimersBase
  {
  public:
    CGUIWindowPVRRadioTimerRules();
    ~CGUIWindowPVRRadioTimerRules() override = default;

  protected:
    std::string GetRootPath() const override;
    std::string GetDirectoryPath() override;
  };
}
