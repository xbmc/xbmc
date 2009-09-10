/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "GUIDialogVideoSettings.h"
#include "GUIWindowManager.h"
#include "GUIPassword.h"
#include "Util.h"
#include "MathUtils.h"
#include "GUISettings.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "VideoDatabase.h"
#include "GUIDialogYesNo.h"
#include "Settings.h"
#include "SkinInfo.h"

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/VDPAU.h"
#endif

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
#define VIDEO_SETTINGS_SOFTEN             13
#define VIDEO_SETTINGS_SCALINGMETHOD      18

#define VIDEO_SETTING_VDPAU_NOISE         19
#define VIDEO_SETTING_VDPAU_SHARPNESS     20

void CGUIDialogVideoSettings::CreateSettings()
{
  m_usePopupSliders = g_SkinInfo.HasSkinFile("DialogSlider.xml");
  // clear out any old settings
  m_settings.clear();
  // create our settings
  {
#ifdef HAVE_LIBVDPAU
    const int entries[] = { 16018, 16019, 20131, 20130, 20129, 16022, 16021, 16020, 16310, 16314};
    AddSpin(VIDEO_SETTINGS_INTERLACEMETHOD, 16023, (int*)&g_stSettings.m_currentVideoSettings.m_InterlaceMethod, 10, entries);                           
#else
    const int entries[] = { 16018, 16019, 20131, 20130, 20129, 16022, 16021, 16020, 16314};
    AddSpin(VIDEO_SETTINGS_INTERLACEMETHOD, 16023, (int*)&g_stSettings.m_currentVideoSettings.m_InterlaceMethod, 9, entries);
#endif
  }
  {
    const int entries[] = { 16301, 16302, 16303, 16304, 16305, 16306, 16307, 16308, 16309 };
    AddSpin(VIDEO_SETTINGS_SCALINGMETHOD, 16300, (int*)&g_stSettings.m_currentVideoSettings.m_ScalingMethod, 3, entries);
  }
  AddBool(VIDEO_SETTINGS_CROP, 644, &g_stSettings.m_currentVideoSettings.m_Crop);
  {
    const int entries[] = {630, 631, 632, 633, 634, 635, 636 };
    AddSpin(VIDEO_SETTINGS_VIEW_MODE, 629, &g_stSettings.m_currentVideoSettings.m_ViewMode, 7, entries);
  }
  AddSlider(VIDEO_SETTINGS_ZOOM, 216, &g_stSettings.m_currentVideoSettings.m_CustomZoomAmount, 0.5f, 0.01f, 2.0f, FormatFloat);
  AddSlider(VIDEO_SETTINGS_PIXEL_RATIO, 217, &g_stSettings.m_currentVideoSettings.m_CustomPixelRatio, 0.5f, 0.01f, 2.0f, FormatFloat);

#ifdef HAS_VIDEO_PLAYBACK
  if (g_renderManager.SupportsBrightness() || (g_VDPAU && !(g_guiSettings.GetBool("videoplayer.vdpaustudiolevel"))))
    AddSlider(VIDEO_SETTINGS_BRIGHTNESS, 464, &g_stSettings.m_currentVideoSettings.m_Brightness, 0, 1, 100, FormatInteger);
  if (g_renderManager.SupportsContrast() || (g_VDPAU && !(g_guiSettings.GetBool("videoplayer.vdpaustudiolevel"))))
    AddSlider(VIDEO_SETTINGS_CONTRAST, 465, &g_stSettings.m_currentVideoSettings.m_Contrast, 0, 1, 100, FormatInteger);
  if (g_renderManager.SupportsGamma())
    AddSlider(VIDEO_SETTINGS_GAMMA, 466, &g_stSettings.m_currentVideoSettings.m_Gamma, 0, 1, 100, FormatInteger);
#ifdef HAVE_LIBVDPAU
  CSharedLock lock(g_renderManager.GetSection());
  if (g_VDPAU) {
    AddSlider(VIDEO_SETTING_VDPAU_NOISE, 16312, &g_stSettings.m_currentVideoSettings.m_NoiseReduction, 0.0f, 0.01f, 1.0f, FormatFloat);
    AddSlider(VIDEO_SETTING_VDPAU_SHARPNESS, 16313, &g_stSettings.m_currentVideoSettings.m_Sharpness, -1.0f, 0.02f, 1.0f, FormatFloat);
  }
#endif
#endif
  AddSeparator(8);
  AddButton(VIDEO_SETTINGS_MAKE_DEFAULT, 12376);
  AddButton(VIDEO_SETTINGS_CALIBRATION, 214);
}

void CGUIDialogVideoSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
#ifdef HAS_VIDEO_PLAYBACK
  if (setting.id == VIDEO_SETTINGS_CROP)
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
  else 
#endif
  if (setting.id == VIDEO_SETTINGS_BRIGHTNESS || setting.id == VIDEO_SETTINGS_CONTRAST || setting.id == VIDEO_SETTINGS_GAMMA)
    CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
  else if (setting.id == VIDEO_SETTINGS_CALIBRATION)
  {
    // launch calibration window
    if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].settingsLocked() && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;
    m_gWindowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
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

CStdString CGUIDialogVideoSettings::FormatInteger(float value, float minimum)
{
  CStdString text;
  text.Format("%i", MathUtils::round_int(value));
  return text;
}

CStdString CGUIDialogVideoSettings::FormatFloat(float value, float minimum)
{
  CStdString text;
  text.Format("%2.2f", value);
  return text;
}

