/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "GUIDialogVideoMisc.h"
#include "Application.h"
#include "addons/Skin.h"
#include "settings/Settings.h"
#include "powermanagement/PowerManager.h"

using namespace std;

#define VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END      1
#define VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END_MODE 2

// Settings here are not meant to be persisted in a video database
CGUIDialogVideoMisc::CGUIDialogVideoMisc()
    : CGUIDialogSettings(WINDOW_DIALOG_VIDEO_MISC, "VideoSettingsMisc.xml")
{
}

CGUIDialogVideoMisc::~CGUIDialogVideoMisc(void)
{
}

void CGUIDialogVideoMisc::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo->HasSkinFile("DialogSlider.xml");
  // clear out any old settings
  m_settings.clear();
  
  AddBool(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END, 13017, &g_settings.m_currentVideoSettings.m_AutoPowerStateAfterPlayback);
  EnableSettings(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END, (g_application.m_pPlayer->HasAutoPowerStateSupport()));
  UpdateSetting(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END);

  {
    vector<pair<int, int> > entries;
    if (g_powerManager.CanPowerdown())
      entries.push_back(make_pair(POWERSTATE_SHUTDOWN, 13005));

    if (g_powerManager.CanHibernate())
      entries.push_back(make_pair(POWERSTATE_HIBERNATE, 13010));

    if (g_powerManager.CanSuspend())
      entries.push_back(make_pair(POWERSTATE_SUSPEND, 13011));

    AddSpin(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END_MODE, 13018, (int*)&g_settings.m_currentVideoSettings.m_AutoPowerStateMode, entries);
    EnableSettings(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END_MODE, g_settings.m_currentVideoSettings.m_AutoPowerStateAfterPlayback);
    UpdateSetting(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END_MODE);
  }
}

void CGUIDialogVideoMisc::OnSettingChanged(SettingInfo &setting)
{
  if (setting.id == VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END)
  {
    EnableSettings(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END_MODE, g_settings.m_currentVideoSettings.m_AutoPowerStateAfterPlayback);
    UpdateSetting(VIDEO_SETTINGS_POWER_STATE_ON_PLAY_END_MODE);
  }
}
