/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameSettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

namespace
{
  const std::string SETTING_GAMES_ENABLE = "gamesgeneral.enable";
  const std::string SETTING_GAMES_ENABLEREWIND = "gamesgeneral.enablerewind";
  const std::string SETTING_GAMES_REWINDTIME = "gamesgeneral.rewindtime";
}

CGameSettings::CGameSettings(CSettings &settings) :
  m_settings(settings)
{
  m_settings.RegisterCallback(this, {
    SETTING_GAMES_ENABLEREWIND,
    SETTING_GAMES_REWINDTIME,
  });
}

CGameSettings::~CGameSettings()
{
  m_settings.UnregisterCallback(this);
}

bool CGameSettings::GamesEnabled()
{
  return m_settings.GetBool(SETTING_GAMES_ENABLE);
}

void CGameSettings::ToggleGames()
{
  m_settings.ToggleBool(SETTING_GAMES_ENABLE);
}

bool CGameSettings::RewindEnabled()
{
  return m_settings.GetBool(SETTING_GAMES_ENABLEREWIND);
}

unsigned int CGameSettings::MaxRewindTimeSec()
{
  int rewindTimeSec = m_settings.GetInt(SETTING_GAMES_REWINDTIME);

  return static_cast<unsigned int>(std::max(rewindTimeSec, 0));
}

void CGameSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_GAMES_ENABLEREWIND ||
      settingId == SETTING_GAMES_REWINDTIME)
  {
    SetChanged();
    NotifyObservers(ObservableMessageSettingsChanged);
  }
}
