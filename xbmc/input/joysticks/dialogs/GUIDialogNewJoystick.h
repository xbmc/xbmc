/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

#include <string>

namespace KODI
{
namespace JOYSTICK
{
  class CGUIDialogNewJoystick : protected CThread
  {
  public:
    CGUIDialogNewJoystick();
    virtual ~CGUIDialogNewJoystick() = default;

    void ShowAsync(const std::string& deviceName);

  protected:
    // implementation of CThread
    virtual void Process() override;

  private:
    std::string m_strDeviceName;
  };
}
}
