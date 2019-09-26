/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../OSScreenSaver.h"

#import <IOKit/pwr_mgt/IOPMLib.h>

class COSScreenSaverOSX : public KODI::WINDOWING::IOSScreenSaver
{
public:
  COSScreenSaverOSX() = default;
  void Inhibit() override;
  void Uninhibit() override;

private:
  IOPMAssertionID m_assertionID = 0;
};
