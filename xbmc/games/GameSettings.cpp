/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameSettings.h"

#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

namespace
{
  const std::string SETTING_GAMES_ENABLE = "gamesgeneral.enable";
  const std::string SETTING_GAMES_SHOW_OSD_HELP = "gamesgeneral.showosdhelp";
  const std::string SETTING_GAMES_ENABLEAUTOSAVE = "gamesgeneral.enableautosave";
  const std::string SETTING_GAMES_ENABLEREWIND = "gamesgeneral.enablerewind";
  const std::string SETTING_GAMES_REWINDTIME = "gamesgeneral.rewindtime";
}

CGameSettings::CGameSettings()
{
  m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  m_settings->RegisterCallback(this, {
    SETTING_GAMES_ENABLEREWIND,
    SETTING_GAMES_REWINDTIME,
  });
}

CGameSettings::~CGameSettings()
{
  m_settings->UnregisterCallback(this);
}

bool CGameSettings::GamesEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLE);
}

bool CGameSettings::ShowOSDHelp()
{
  return m_settings->GetBool(SETTING_GAMES_SHOW_OSD_HELP);
}

void CGameSettings::SetShowOSDHelp(bool bShow)
{
  if (m_settings->GetBool(SETTING_GAMES_SHOW_OSD_HELP) != bShow)
  {
    m_settings->SetBool(SETTING_GAMES_SHOW_OSD_HELP, bShow);

    //! @todo Asynchronous save
    m_settings->Save();
  }
}

void CGameSettings::ToggleGames()
{
  m_settings->ToggleBool(SETTING_GAMES_ENABLE);
}

bool CGameSettings::AutosaveEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLEAUTOSAVE);
}

bool CGameSettings::RewindEnabled()
{
  return m_settings->GetBool(SETTING_GAMES_ENABLEREWIND);
}

unsigned int CGameSettings::MaxRewindTimeSec()
{
  int rewindTimeSec = m_settings->GetInt(SETTING_GAMES_REWINDTIME);

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
