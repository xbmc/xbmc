/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "HDRStatus.h"

class CWinRtUtil
{
private:
  CWinRtUtil() = default;
  ~CWinRtUtil() = default;

public:
  static HDR_STATUS GetWindowsHDRStatus();
  static HDR_STATUS GetWindowsHDRStatusUWP();
};
