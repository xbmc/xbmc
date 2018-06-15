/*
 *  Copyright (C) 2018 Chris Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../OSScreenSaver.h"

class COSScreenSaverAndroid : public KODI::WINDOWING::IOSScreenSaver
{
public:
  explicit COSScreenSaverAndroid() {}
  void Inhibit() override;
  void Uninhibit() override;
};
