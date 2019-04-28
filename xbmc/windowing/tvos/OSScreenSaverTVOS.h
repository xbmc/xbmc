/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/OSScreenSaver.h"

class COSScreenSaverTVOS : public KODI::WINDOWING::IOSScreenSaver
{
public:
  COSScreenSaverTVOS() = default;
  void Inhibit() override;
  void Uninhibit() override;
};
