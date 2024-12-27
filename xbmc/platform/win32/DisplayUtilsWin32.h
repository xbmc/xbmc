/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "HDRStatus.h"

#include <optional>
#include <string>
#include <vector>

#include <wingdi.h>

class CDisplayUtilsWin32
{
public:
  CDisplayUtilsWin32() = delete;
  ~CDisplayUtilsWin32() = delete;

  struct DisplayConfigId
  {
    LUID adapterId;
    UINT32 id;
  };

  static std::wstring GetCurrentDisplayName();
  static std::vector<DISPLAYCONFIG_PATH_INFO> GetDisplayConfigPaths();
  static std::optional<DisplayConfigId> GetCurrentDisplayTargetId();
  static std::optional<DisplayConfigId> GetDisplayTargetId(const std::wstring& gdiDeviceName);
  static HDR_STATUS GetDisplayHDRStatus(const DisplayConfigId& identifier);
  static HDR_STATUS SetDisplayHDRStatus(const DisplayConfigId& identifier, bool enable);
};
