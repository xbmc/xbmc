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

class CSetting;
class CSettings;

namespace KODI
{
namespace SUBTITLES
{

class CSubtitlesSettings : public ISettingCallback, public Observable
{
public:
  static CSubtitlesSettings& GetInstance();

  // Inherited from ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

protected:
  CSubtitlesSettings();
  ~CSubtitlesSettings() override;

private:
  // Construction parameters
  std::shared_ptr<CSettings> m_settings;
};

} // namespace SUBTITLES
} // namespace KODI
