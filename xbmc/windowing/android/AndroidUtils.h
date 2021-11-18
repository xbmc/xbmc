/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "utils/HDRCapabilities.h"
#include "windowing/Resolution.h"

#include <string>
#include <vector>

#include <androidjni/Display.h>

class CAndroidUtils : public ISettingCallback
{
public:
  CAndroidUtils();
  ~CAndroidUtils() override = default;
  bool GetNativeResolution(RESOLUTION_INFO* res) const;
  bool SetNativeResolution(const RESOLUTION_INFO& res);
  bool ProbeResolutions(std::vector<RESOLUTION_INFO>& resolutions);
  bool UpdateDisplayModes();
  bool IsHDRDisplay();

  // Implementation of ISettingCallback
  static const std::string SETTING_LIMITGUI;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  static bool SupportsMediaCodecMimeType(const std::string& mimeType);

  // Android specific HDR type mapping
  // https://developer.android.com/reference/android/view/Display.HdrCapabilities#constants_1
  enum HDRTypes
  {
    DOLBY_VISION = 1,
    HDR10 = 2,
    HLG = 3,
    HDR10_PLUS = 4
  };

  static std::vector<int> GetDisplaySupportedHdrTypes();
  static CHDRCapabilities GetDisplayHDRCapabilities();

protected:
  mutable int m_width;
  mutable int m_height;

private:
  static void LogDisplaySupportedHdrTypes();
};
