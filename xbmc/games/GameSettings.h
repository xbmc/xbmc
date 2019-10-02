/*
 *  Copyright (C) 2012-2018 Team Kodi
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
namespace GAME
{

class CGameSettings : public ISettingCallback,
                      public Observable
{
public:
  CGameSettings();
  ~CGameSettings() override;

  // General settings
  bool GamesEnabled();
  bool ShowOSDHelp();
  void SetShowOSDHelp(bool bShow);
  void ToggleGames();
  bool AutosaveEnabled();
  bool RewindEnabled();
  unsigned int MaxRewindTimeSec();

  // Inherited from ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

private:
  // Construction parameters
  std::shared_ptr<CSettings> m_settings;
};

} // namespace GAME
}
