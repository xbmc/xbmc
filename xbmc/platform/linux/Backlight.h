/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/IBacklight.h"

#include "platform/linux/SysfsPath.h"

#include <memory>
#include <string>

class CBacklight : public IBacklight
{
public:
  CBacklight() = default;
  ~CBacklight() = default;

  static void Register(std::shared_ptr<CBacklight> backlight);

  bool Init(const std::string& drmDevicePath);

  // CBacklight Overrides
  bool SetBrightness(int brightness) override;
  int GetBrightness() const override;
  int GetMaxBrightness() const override;

private:
  CSysfsPath m_actualBrightnessPath;
  CSysfsPath m_maxBrightnessPath;
  CSysfsPath m_brightnessPath;
};
