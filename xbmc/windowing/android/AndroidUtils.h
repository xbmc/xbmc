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

  static std::vector<int> GetDisplaySupportedHdrTypes();
  static CHDRCapabilities GetDisplayHDRCapabilities();
  static std::pair<bool, bool> GetDolbyVisionCapabilities();

  /*!
   * \brief Whether the user enabled forcing of Dolby Vision display support.
   */
  static bool IsDolbyVisionForcedBySetting();

  /*!
   * \brief Whether forced Dolby Vision must not be used for content at a given frame rate.
   * \param fps the content frame rate, <= 0 when unknown
   * \return true when Dolby Vision display support is only present because the user forced
   *         it and the content cannot be shown that way, either because it is above the 30fps
   *         that HDMI 2.0 carries as Dolby Vision at 4K, or because its frame rate is unknown
   *         and so no display mode will be requested for it.
   */
  static bool IsForcedDolbyVisionBlockedForFps(double fps);

protected:
  mutable int m_width;
  mutable int m_height;

private:
  static void LogDisplaySupportedHdrTypes();
};
