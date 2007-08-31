/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogVideoSettings.h"
#include "GUIPassword.h"
#include "Util.h"
#include "Application.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "VideoDatabase.h"

CGUIDialogVideoSettings::CGUIDialogVideoSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "VideoOSDSettings.xml")
{
}

CGUIDialogVideoSettings::~CGUIDialogVideoSettings(void)
{
}

#define VIDEO_SETTINGS_CROP               1
#define VIDEO_SETTINGS_VIEW_MODE          2
#define VIDEO_SETTINGS_ZOOM               3
#define VIDEO_SETTINGS_PIXEL_RATIO        4
#define VIDEO_SETTINGS_BRIGHTNESS         5
#define VIDEO_SETTINGS_CONTRAST           6
#define VIDEO_SETTINGS_GAMMA              7
#define VIDEO_SETTINGS_INTERLACEMETHOD    8
// separator 9
#define VIDEO_SETTINGS_MAKE_DEFAULT       10

#define VIDEO_SETTINGS_CALIBRATION        11
#define VIDEO_SETTINGS_FLICKER            12
#define VIDEO_SETTINGS_SOFTEN             13
#define VIDEO_SETTINGS_FILM_GRAIN         14
#define VIDEO_SETTINGS_NON_INTERLEAVED    15
#define VIDEO_SETTINGS_NO_CACHE           16
#define VIDEO_SETTINGS_FORCE_INDEX        17
#define VIDEO_SETTINGS_SCALINGMETHOD      18

void CGUIDialogVideoSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  {
    const int entries[] = { 16018, 16019, 16020, 16021, 16022, 20129, 20130, 20131 };
    AddSpin(VIDEO_SETTINGS_INTERLACEMETHOD, 16023, (int*)&g_stSettings.m_currentVideoSettings.m_InterlaceMethod, 8, entries);
  }
  {
    const int entries[] = { 160301, 16302, 16303, 16304, 16305, 16306 };
    AddSpin(VIDEO_SETTINGS_INTERLACEMETHOD, 16300, (int*)&g_stSettings.m_currentVideoSettings.m_ScalingMethod, 6, entries);
  }
  AddBool(VIDEO_SETTINGS_CROP, 644, &g_stSettings.m_currentVideoSettings.m_Crop);
  {
    const int entries[] = {630, 631, 632, 633, 634, 635, 636 };
    AddSpin(VIDEO_SETTINGS_VIEW_MODE, 629, &g_stSettings.m_currentVideoSettings.m_ViewMode, 7, entries);
  }
  AddSlider(VIDEO_SETTINGS_ZOOM, 216, &g_stSettings.m_currentVideoSettings.m_CustomZoomAmount, 0.5f, 0.01f, 2.0f);
  AddSlider(VIDEO_SETTINGS_PIXEL_RATIO, 217, &g_stSettings.m_currentVideoSettings.m_CustomPixelRatio, 0.5f, 0.01f, 2.0f);
  AddSlider(VIDEO_SETTINGS_BRIGHTNESS, 464, &g_stSettings.m_currentVideoSettings.m_Brightness, 0, 100);
  AddSlider(VIDEO_SETTINGS_CONTRAST, 465, &g_stSettings.m_currentVideoSettings.m_Contrast, 0, 100);
  AddSlider(VIDEO_SETTINGS_GAMMA, 466, &g_stSettings.m_currentVideoSettings.m_Gamma, 0, 100);

  AddSeparator(8);
  AddButton(VIDEO_SETTINGS_MAKE_DEFAULT, 12376);
  m_flickerFilter = g_guiSettings.GetInt("videoplayer.flicker");
  AddSpin(VIDEO_SETTINGS_FLICKER, 13100, &m_flickerFilter, 0, 5, g_localizeStrings.Get(351).c_str());
  m_soften = g_guiSettings.GetBool("videoplayer.soften");
  AddBool(VIDEO_SETTINGS_SOFTEN, 215, &m_soften);
  AddSlider(VIDEO_SETTINGS_FILM_GRAIN, 14058, (int*)&g_stSettings.m_currentVideoSettings.m_FilmGrain, 0, 10);
  AddButton(VIDEO_SETTINGS_CALIBRATION, 214);
  AddBool(VIDEO_SETTINGS_NON_INTERLEAVED, 306, &g_stSettings.m_currentVideoSettings.m_NonInterleaved);
  AddBool(VIDEO_SETTINGS_NO_CACHE, 431, &g_stSettings.m_currentVideoSettings.m_NoCache);
  AddButton(VIDEO_SETTINGS_FORCE_INDEX, 12009);
}

void CGUIDialogVideoSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == VIDEO_SETTINGS_NON_INTERLEAVED ||  setting.id == VIDEO_SETTINGS_NO_CACHE)
    g_application.Restart(true);
  else if (setting.id == VIDEO_SETTINGS_FILM_GRAIN)
    g_application.DelayedPlayerRestart();
#ifdef HAS_VIDEO_PLAYBACK
  else if (setting.id == VIDEO_SETTINGS_CROP)
    g_renderManager.AutoCrop(g_stSettings.m_currentVideoSettings.m_Crop);
  else if (setting.id == VIDEO_SETTINGS_VIEW_MODE)
  {
    g_renderManager.SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
    g_stSettings.m_currentVideoSettings.m_CustomZoomAmount = g_stSettings.m_fZoomAmount;
    g_stSettings.m_currentVideoSettings.m_CustomPixelRatio = g_stSettings.m_fPixelRatio;
    UpdateSetting(VIDEO_SETTINGS_ZOOM);
    UpdateSetting(VIDEO_SETTINGS_PIXEL_RATIO);
  }
  else if (setting.id == VIDEO_SETTINGS_ZOOM || setting.id == VIDEO_SETTINGS_PIXEL_RATIO)
  {
    g_stSettings.m_currentVideoSettings.m_ViewMode = VIEW_MODE_CUSTOM;
    g_renderManager.SetViewMode(VIEW_MODE_CUSTOM);
    UpdateSetting(VIDEO_SETTINGS_VIEW_MODE);
  }
#endif
  else if (setting.id == VIDEO_SETTINGS_BRIGHTNESS || setting.id == VIDEO_SETTINGS_CONTRAST || setting.id == VIDEO_SETTINGS_GAMMA)
    CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
  else if (setting.id == VIDEO_SETTINGS_FLICKER || setting.id == VIDEO_SETTINGS_SOFTEN)
  {
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    g_guiSettings.SetInt("videoplayer.flicker", m_flickerFilter);
    g_guiSettings.SetBool("videoplayer.soften", m_soften);
    g_graphicsContext.SetVideoResolution(res);
  }
  else if (setting.id == VIDEO_SETTINGS_CALIBRATION)
  {
    // launch calibration window
    if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].settingsLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;
    m_gWindowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  else if (setting.id == VIDEO_SETTINGS_FORCE_INDEX)
  {
    g_stSettings.m_currentVideoSettings.m_bForceIndex = true;
    g_application.Restart(true);
  }
  else if (setting.id == VIDEO_SETTINGS_MAKE_DEFAULT)
  {
    if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].settingsLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    // prompt user if they are sure
    if (CGUIDialogYesNo::ShowAndGetInput(12376, 750, 0, 12377))
    { // reset the settings
      CVideoDatabase db;
      db.Open();
      db.EraseVideoSettings();
      db.Close();
      g_stSettings.m_defaultVideoSettings = g_stSettings.m_currentVideoSettings;
      g_settings.Save();
    }
  }
}

