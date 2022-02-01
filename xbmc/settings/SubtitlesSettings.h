/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "utils/Observer.h"

#include <memory>

class CSetting;
class CSettings;
struct StringSettingOption;

namespace KODI
{
namespace SUBTITLES
{
// This is a placeholder to keep the fontname setting valid
// even if the default app font could be changed
constexpr const char* FONT_DEFAULT_FAMILYNAME = "DEFAULT";

class CSubtitlesSettings : public ISettingCallback, public Observable
{
public:
  static CSubtitlesSettings& GetInstance();

  // Inherited from ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  static void SettingOptionsSubtitleFontsFiller(const std::shared_ptr<const CSetting>& setting,
                                                std::vector<StringSettingOption>& list,
                                                std::string& current,
                                                void* data);

protected:
  CSubtitlesSettings();
  ~CSubtitlesSettings() override;

private:
  // Construction parameters
  std::shared_ptr<CSettings> m_settings;
};

} // namespace SUBTITLES
} // namespace KODI
