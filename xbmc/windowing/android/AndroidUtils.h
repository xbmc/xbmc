/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/HDRCapabilities.h"
#include "windowing/Resolution.h"

#include <string>
#include <vector>

#include <androidjni/Display.h>

class CAndroidUtils
{
public:
  CAndroidUtils();
  ~CAndroidUtils() = default;
  bool GetNativeResolution(RESOLUTION_INFO* res) const;
  bool SetNativeResolution(const RESOLUTION_INFO& res);
  bool ProbeResolutions(std::vector<RESOLUTION_INFO>& resolutions);
  bool UpdateDisplayModes();
  bool IsHDRDisplay();

  static const std::string SETTING_LIMITGUI;

  static bool SupportsMediaCodecMimeType(const std::string& mimeType);

  static std::vector<int> GetDisplaySupportedHdrTypes();
  static CHDRCapabilities GetDisplayHDRCapabilities();
  static std::pair<bool, bool> GetDolbyVisionCapabilities();

protected:
  mutable int m_width;
  mutable int m_height;

private:
  static void LogDisplaySupportedHdrTypes();
};
