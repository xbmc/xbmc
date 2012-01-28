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

#include "system.h"
#include "GUIUserMessages.h"
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "interfaces/Builtins.h"
#include "input/KeyboardLayoutConfiguration.h"
#include "filesystem/Directory.h"
#include "Util.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIImage.h"
#include "utils/Weather.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "ViewDatabase.h"
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#include "PlayListPlayer.h"
#include "addons/Skin.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/AudioContext.h"
#include "network/libscrobbler/lastfmscrobbler.h"
#include "network/libscrobbler/librefmscrobbler.h"
#include "GUIPassword.h"
#include "GUIInfoManager.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "addons/GUIDialogAddonSettings.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "addons/Visualisation.h"
#include "addons/AddonManager.h"
#include "addons/AddonInstaller.h"
#include "storage/MediaManager.h"
#include "network/Network.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIFontManager.h"
#ifdef _LINUX
#include "LinuxTimezone.h"
#include <dlfcn.h>
#include "cores/AudioRenderers/AudioRendererFactory.h"
#if defined(USE_ALSA)
#include "cores/AudioRenderers/ALSADirectSound.h"
#endif
#ifdef HAS_HAL
#include "HALManager.h"
#endif
#endif
#if defined(__APPLE__) 
#if defined(__arm__)
#include "IOSCoreAudio.h"
#else
#include "CoreAudio.h"
#include "XBMCHelper.h"
#endif
#endif
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/PVRManager.h"
#include "network/GUIDialogAccessPoints.h"
#include "filesystem/Directory.h"

#include "FileItem.h"
#include "guilib/GUIToggleButtonControl.h"
#include "filesystem/SpecialProtocol.h"

#include "network/Zeroconf.h"
#include "peripherals/Peripherals.h"
#include "peripherals/dialogs/GUIDialogPeripheralManager.h"

#ifdef _WIN32
#include "WIN32Util.h"
#include "cores/AudioRenderers/AudioRendererFactory.h"
#endif
#include <map>
#include "Settings.h"
#include "AdvancedSettings.h"
#include "input/MouseStat.h"
#include "guilib/LocalizeStrings.h"
#include "LangInfo.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "windowing/WindowingFactory.h"

#if defined(HAVE_LIBCRYSTALHD)
#include "cores/dvdplayer/DVDCodecs/Video/CrystalHD.h"
#endif

#if defined(HAS_AIRPLAY)
#include "network/AirPlayServer.h"
#endif

#if defined(HAS_WEB_SERVER)
#include "network/WebServer.h"
#endif

using namespace std;
using namespace XFILE;
using namespace ADDON;
using namespace PVR;
using namespace PERIPHERALS;

#define CONTROL_GROUP_BUTTONS           0
#define CONTROL_GROUP_SETTINGS          1
#define CONTROL_SETTINGS_LABEL          2
#define CATEGORY_GROUP_ID               3
#define SETTINGS_GROUP_ID               5
#define CONTROL_DEFAULT_BUTTON          7
#define CONTROL_DEFAULT_RADIOBUTTON     8
#define CONTROL_DEFAULT_SPIN            9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_DEFAULT_EDIT            12
#define CONTROL_START_BUTTONS           -100
#define CONTROL_START_CONTROL           -80

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
    : CGUIWindow(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml")
{
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalButton = NULL;
  m_pOriginalCategoryButton = NULL;
  m_pOriginalImage = NULL;
  m_pOriginalEdit = NULL;
  // set the correct ID range...
  m_idRange = 9;
  m_iScreen = 0;
  m_strOldTrackFormat = "";
  m_strOldTrackFormatRight = "";
  m_returningFromSkinLoad = false;
  m_delayedSetting = NULL;
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory(void)
{
  FreeControls();
  delete m_pOriginalEdit;
}

bool CGUIWindowSettingsCategory::OnBack(int actionID)
{
  g_settings.Save();
  m_lastControlID = 0; // don't save the control as we go to a different window each time
  return CGUIWindow::OnBack(actionID);
}

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      for (unsigned int i = 0; i < m_vecSettings.size(); i++)
      {
        if (m_vecSettings[i]->GetID() == (int)iControl)
          OnClick(m_vecSettings[i]);
      }
    }
    break;
  case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_BUTTONS && focusedControl < (int)(CONTROL_START_BUTTONS + m_vecSections.size()) &&
          focusedControl - CONTROL_START_BUTTONS != m_iSection && !m_returningFromSkinLoad)
      {
        // changing section, check for updates and cancel any delayed changes
        m_delayedSetting = NULL;
        CheckForUpdates();

        if (m_vecSections[focusedControl-CONTROL_START_BUTTONS]->m_strCategory == "masterlock")
        {
          if (!g_passwordManager.IsMasterLockUnlocked(true))
          { // unable to go to this category - focus the previous one
            SET_CONTROL_FOCUS(CONTROL_START_BUTTONS + m_iSection, 0);
            return false;
          }
        }
        m_iSection = focusedControl - CONTROL_START_BUTTONS;

        CreateSettings();
      }
      return true;
    }
  case GUI_MSG_LOAD_SKIN:
    {
      if (IsActive())
        m_returningFromSkinLoad = true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_delayedSetting = NULL;
      if (message.GetParam1() != WINDOW_INVALID && !m_returningFromSkinLoad)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iSection = 0;
        ResetControlStates();
      }
      m_iScreen = (int)message.GetParam2() - (int)CGUIWindow::GetID();
      CGUIWindow::OnMessage(message);
      m_returningFromSkinLoad = false;
      return true;
    }
    break;
  case GUI_MSG_UPDATE_ITEM:
    if (m_delayedSetting)
    {
      OnSettingChanged(m_delayedSetting);
      m_delayedSetting = NULL;
      return true;
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
      {
        // Cancel delayed setting - it's only used for res changing anyway
        m_delayedSetting = NULL;
        if (IsActive() && g_guiSettings.GetResolution() != g_graphicsContext.GetVideoResolution())
        {
          g_guiSettings.SetResolution(g_graphicsContext.GetVideoResolution());
          CreateSettings();
        }
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_delayedSetting = NULL;

      CheckForUpdates();
      CGUIWindow::OnMessage(message);
      FreeControls();
      return true;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsCategory::SetupControls()
{
  // cleanup first, if necessary
  FreeControls();
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalCategoryButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_CATEGORY_BUTTON);
  m_pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  m_pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalCategoryButton || !m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalButton)
    return ;
  m_pOriginalEdit = (CGUIEditControl *)GetControl(CONTROL_DEFAULT_EDIT);
  if (!m_pOriginalEdit || m_pOriginalEdit->GetControlType() != CGUIControl::GUICONTROL_EDIT)
  {
    delete m_pOriginalEdit;
    m_pOriginalEdit = new CGUIEditControl(*m_pOriginalButton);
  }
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalButton->SetVisible(false);
  m_pOriginalCategoryButton->SetVisible(false);
  m_pOriginalEdit->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);
  // setup our control groups...
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (!group)
    return;
  // get a list of different sections
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(m_iScreen);
  if (!pSettingsGroup) return ;
  // update the screen string
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, pSettingsGroup->GetLabelID());
  // get the categories we need
  pSettingsGroup->GetCategories(m_vecSections);
  // run through and create our buttons...
  int j=0;
  for (unsigned int i = 0; i < m_vecSections.size(); i++)
  {
    if (m_vecSections[i]->m_labelID == 12360 && !g_settings.IsMasterUser())
      continue;
    CGUIButtonControl *pButton = NULL;
    if (m_pOriginalCategoryButton->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
      pButton = new CGUIToggleButtonControl(*(CGUIToggleButtonControl *)m_pOriginalCategoryButton);
    else
      pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
    pButton->SetLabel(g_localizeStrings.Get(m_vecSections[i]->m_labelID));
    pButton->SetID(CONTROL_START_BUTTONS + j);
    pButton->SetVisible(true);
    pButton->AllocResources();
    group->AddControl(pButton);
    j++;
  }
  if (m_iSection < 0 || m_iSection >= (int)m_vecSections.size())
    m_iSection = 0;
  CreateSettings();
  // set focus correctly
  m_defaultControl = CONTROL_START_BUTTONS;
}

CGUIControl* CGUIWindowSettingsCategory::AddIntBasedSpinControl(CSetting *pSetting, float groupWidth, int &iControlID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)AddSetting(pSetting, groupWidth, iControlID);
  if (!pSettingInt->m_entries.empty())
  {
    for (map<int,int>::iterator it=pSettingInt->m_entries.begin(); it != pSettingInt->m_entries.end();++it)
      pControl->AddLabel(g_localizeStrings.Get(it->first), it->second);
    pControl->SetValue(pSettingInt->GetData());
  }
  return pControl;
}

void CGUIWindowSettingsCategory::CreateSettings()
{
  FreeSettingsControls();

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (!group)
    return;
  vecSettings settings;
  g_guiSettings.GetSettingsGroup(m_vecSections[m_iSection]->m_strCategory, settings);
  int iControlID = CONTROL_START_CONTROL;
  for (unsigned int i = 0; i < settings.size(); i++)
  {
    CSetting *pSetting = settings[i];
    CStdString strSetting = pSetting->GetSetting();
    if (pSetting->GetType() == SETTINGS_TYPE_INT)
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)AddIntBasedSpinControl(pSetting, group->GetWidth(), iControlID);
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      if (strSetting.Equals("videoplayer.pauseafterrefreshchange"))
      {
        pControl->AddLabel(g_localizeStrings.Get(13551), 0);

        for (int i = 1; i <= MAXREFRESHCHANGEDELAY; i++)
        {
          CStdString delayText;
          delayText.Format(g_localizeStrings.Get(13553).c_str(), (double)i / 10.0);
          pControl->AddLabel(delayText, i);
        }
        pControl->SetValue(pSettingInt->GetData());
      }
      else if (strSetting.Equals("subtitles.color"))
      {
        for (int i = SUBTITLE_COLOR_START; i <= SUBTITLE_COLOR_END; i++)
          pControl->AddLabel(g_localizeStrings.Get(760 + i), i);
        pControl->SetValue(pSettingInt->GetData());
      }
      else if (strSetting.Equals("lookandfeel.startupwindow"))
        FillInStartupWindow(pSetting);
      else if (strSetting.Equals("subtitles.height") || strSetting.Equals("karaoke.fontheight") )
        FillInSubtitleHeights(pSetting, pControl);
      else if (strSetting.Equals("videoscreen.screen"))
        FillInScreens(strSetting, g_guiSettings.GetResolution());
      else if (strSetting.Equals("videoscreen.resolution"))
        FillInResolutions(strSetting, g_guiSettings.GetInt("videoscreen.screen"), g_guiSettings.GetResolution(), false);
      else if (strSetting.Equals("epg.defaultguideview"))
        FillInEpgGuideView(pSetting);
      else if (strSetting.Equals("pvrplayback.startlast"))
        FillInPvrStartLastChannel(pSetting);
      continue;
    }
#ifdef HAS_WEB_SERVER
    else if (strSetting.Equals("services.webserverport"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
      continue;
    }
#endif
    else if (strSetting.Equals("services.esport"))
    {
#ifdef HAS_EVENT_SERVER
      AddSetting(pSetting, group->GetWidth(), iControlID);
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
      continue;
#endif
    }
    else if (strSetting.Equals("network.httpproxyport"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      CBaseSettingControl *control = GetSetting(pSetting->GetSetting());
      control->SetDelayed();
      continue;
    }
    else if (strSetting.Equals("subtitles.font") || strSetting.Equals("karaoke.font") )
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInSubtitleFonts(pSetting);
      continue;
    }
    else if (strSetting.Equals("subtitles.charset") || strSetting.Equals("locale.charset") || strSetting.Equals("karaoke.charset"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInCharSets(pSetting);
      continue;
    }
    else if (strSetting.Equals("lookandfeel.font"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInSkinFonts(pSetting);
      continue;
    }
    else if (strSetting.Equals("lookandfeel.soundskin"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInSoundSkins(pSetting);
      continue;
    }
    else if (strSetting.Equals("locale.language"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInLanguages(pSetting);
      continue;
    }
#ifdef _LINUX
    else if (strSetting.Equals("locale.timezonecountry"))
    {
      CStdString myTimezoneCountry = g_guiSettings.GetString("locale.timezonecountry");
      int myTimezeoneCountryIndex = 0;

      CGUISpinControlEx *pControl = (CGUISpinControlEx *)AddSetting(pSetting, group->GetWidth(), iControlID);
      vector<CStdString> countries = g_timezone.GetCounties();
      for (unsigned int i=0; i < countries.size(); i++)
      {
        if (countries[i] == myTimezoneCountry)
           myTimezeoneCountryIndex = i;
        pControl->AddLabel(countries[i], i);
      }
      pControl->SetValue(myTimezeoneCountryIndex);
      continue;
    }
    else if (strSetting.Equals("locale.timezone"))
    {
      CStdString myTimezoneCountry = g_guiSettings.GetString("locale.timezonecountry");
      CStdString myTimezone = g_guiSettings.GetString("locale.timezone");
      int myTimezoneIndex = 0;

      CGUISpinControlEx *pControl = (CGUISpinControlEx *)AddSetting(pSetting, group->GetWidth(), iControlID);
      pControl->Clear();
      vector<CStdString> timezones = g_timezone.GetTimezonesByCountry(myTimezoneCountry);
      for (unsigned int i=0; i < timezones.size(); i++)
      {
        if (timezones[i] == myTimezone)
           myTimezoneIndex = i;
        pControl->AddLabel(timezones[i], i);
      }
      pControl->SetValue(myTimezoneIndex);
      continue;
    }
#endif
    else if (strSetting.Equals("videoscreen.screenmode"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInRefreshRates(strSetting, g_guiSettings.GetResolution(), false);
      continue;
    }
    else if (strSetting.Equals("lookandfeel.skintheme"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInSkinThemes(pSetting);
      continue;
    }
    else if (strSetting.Equals("lookandfeel.skincolors"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInSkinColors(pSetting);
      continue;
    }
    /*
    FIXME: setting is hidden in GUI because not supported properly.
    else if (strSetting.Equals("videoplayer.displayresolution") || strSetting.Equals("pictures.displayresolution"))
    {
      FillInResolutions(pSetting);
    }
    */
    else if (strSetting.Equals("locale.country"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInRegions(pSetting);
      continue;
    }
    else if (strSetting.Equals("network.interface"))
    {
      FillInNetworkInterfaces(pSetting, group->GetWidth(), iControlID);
      continue;
    }
    else if (strSetting.Equals("audiooutput.audiodevice"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInAudioDevices(pSetting);
      continue;
    }
    else if (strSetting.Equals("audiooutput.passthroughdevice"))
    {
      AddSetting(pSetting, group->GetWidth(), iControlID);
      FillInAudioDevices(pSetting,true);
      continue;
    }
    AddSetting(pSetting, group->GetWidth(), iControlID);
  }

  if (m_vecSections[m_iSection]->m_strCategory == "network")
     NetworkInterfaceChanged();

  // update our settings (turns controls on/off as appropriate)
  UpdateSettings();
}

void CGUIWindowSettingsCategory::UpdateSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    pSettingControl->Update();
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
#ifdef HAVE_LIBVDPAU
    if (strSetting.Equals("videoplayer.vdpauUpscalingLevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        pControl->SetEnabled(true);
      }
    }
    else
#endif
    if (strSetting.Equals("videoscreen.resolution"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetInt("videoscreen.screen") != DM_WINDOWED);
    }
    else if (strSetting.Equals("videoscreen.screenmode"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetInt("videoscreen.screen") != DM_WINDOWED);
    }
    else if (strSetting.Equals("videoscreen.fakefullscreen"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetInt("videoscreen.screen") != DM_WINDOWED);
    }
#if (defined(__APPLE__) && !defined(__arm__)) || defined(_WIN32)
    else if (strSetting.Equals("videoscreen.blankdisplays"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if (g_Windowing.IsFullScreen())
          pControl->SetEnabled(true);
        else
          pControl->SetEnabled(false);
      }
    }
#endif
#if defined(__APPLE__) && !defined(__arm__)
    else if (strSetting.Equals("input.appleremotemode"))
    {
      int remoteMode = g_guiSettings.GetInt("input.appleremotemode");

      // if it's not disabled, start the event server or else apple remote won't work
      if ( remoteMode != APPLE_REMOTE_DISABLED )
      {
        g_guiSettings.SetBool("services.esenabled", true);
        if (!g_application.StartEventServer())
          CGUIDialogKaiToast::QueueNotification("DefaultIconWarning.png", g_localizeStrings.Get(33102), g_localizeStrings.Get(33100));
      }

      // if XBMC helper is running, prompt user before effecting change
      if ( XBMCHelper::GetInstance().IsRunning() && XBMCHelper::GetInstance().GetMode()!=remoteMode )
      {
        bool cancelled;
        if (!CGUIDialogYesNo::ShowAndGetInput(13144, 13145, 13146, 13147, -1, -1, cancelled, 10000))
        {
          // user declined, restore previous spinner state and appleremote mode
          CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
          g_guiSettings.SetInt("input.appleremotemode", XBMCHelper::GetInstance().GetMode());
          pControl->SetValue(XBMCHelper::GetInstance().GetMode());
        }
        else
        {
          // reload configuration
          XBMCHelper::GetInstance().Configure();
        }
      }
      else
      {
        // set new configuration.
        XBMCHelper::GetInstance().Configure();
      }

      if (XBMCHelper::GetInstance().ErrorStarting() == true)
      {
        // inform user about error
        CGUIDialogOK::ShowAndGetInput(13620, 13621, 20022, 20022);

        // reset spinner to disabled state
        CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
        pControl->SetValue(APPLE_REMOTE_DISABLED);
      }
    }
    else if (strSetting.Equals("input.appleremotealwayson"))
     {
       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl)
       {
         int value = g_guiSettings.GetInt("input.appleremotemode");
         if (value != APPLE_REMOTE_DISABLED)
           pControl->SetEnabled(true);
         else
           pControl->SetEnabled(false);
       }
     }
     else if (strSetting.Equals("input.appleremotesequencetime"))
     {
       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl)
       {
         int value = g_guiSettings.GetInt("input.appleremotemode");
         if (value == APPLE_REMOTE_UNIVERSAL)
           pControl->SetEnabled(true);
         else
           pControl->SetEnabled(false);
       }
     }
#endif
    else if (strSetting.Equals("filelists.allowfiledeletion"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_settings.GetCurrentProfile().filesLocked() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("filelists.showaddsourcebuttons"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.GetCurrentProfile().canWriteSources() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("masterlock.startuplock"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE);
    }
    else if (!strSetting.Equals("pvrmanager.enabled") &&
        (strSetting.Equals("pvrmanager.channelscan") || strSetting.Equals("pvrmanager.channelmanager") ||
         strSetting.Equals("pvrmenu.searchicons")))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("pvrmanager.enabled"));
    }
    else if (!strSetting.Equals("services.esenabled")
             && strSetting.Left(11).Equals("services.es"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("services.esenabled"));
    }
    else if (strSetting.Equals("audiocds.quality"))
    { // only visible if we are doing non-WAV ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_WAV &&
                                         g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_FLAC);
    }
    else if (strSetting.Equals("audiocds.bitrate"))
    { // only visible if we are ripping to CBR
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_WAV && 
                                         g_guiSettings.GetInt("audiocds.encoder") != CDDARIP_ENCODER_FLAC &&
                                         g_guiSettings.GetInt("audiocds.quality") == CDDARIP_QUALITY_CBR);
    }
    else if (strSetting.Equals("audiocds.compressionlevel"))
    { // only visible if we are doing FLAC ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiocds.encoder") == CDDARIP_ENCODER_FLAC);
    }
    else if (
             strSetting.Equals("audiooutput.passthroughdevice") ||
             strSetting.Equals("audiooutput.ac3passthrough") ||
             strSetting.Equals("audiooutput.dtspassthrough") ||
             strSetting.Equals("audiooutput.passthroughaac") ||
             strSetting.Equals("audiooutput.passthroughmp1") ||
             strSetting.Equals("audiooutput.passthroughmp2") ||
             strSetting.Equals("audiooutput.passthroughmp3"))
    { // only visible if we are in digital mode
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode")));
    }
    else if (strSetting.Equals("musicplayer.crossfade"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetString("audiooutput.audiodevice").find("wasapi:") == CStdString::npos);
    }
    else if (strSetting.Equals("musicplayer.crossfadealbumtracks"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("musicplayer.crossfade") > 0 &&
                                         g_guiSettings.GetString("audiooutput.audiodevice").find("wasapi:") == CStdString::npos);
    }
#ifdef HAS_WEB_SERVER
    else if (strSetting.Equals("services.webserverusername") ||
             strSetting.Equals("services.webserverpassword"))
    {
      CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("services.webserver"));
    }
#endif
#ifdef HAS_AIRPLAY
    else if ( strSetting.Equals("services.airplaypassword") || 
              strSetting.Equals("services.useairplaypassword"))
    {
      if (strSetting.Equals("services.airplaypassword"))
      {
        CGUIEditControl *pControl = (CGUIEditControl *)GetControl(pSettingControl->GetID());
        if (pControl)
          pControl->SetEnabled(g_guiSettings.GetBool("services.useairplaypassword"));
      }
      else//useairplaypassword
      {
        CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(pSettingControl->GetID());    
        if (pControl)
          pControl->SetEnabled(g_guiSettings.GetBool("services.airplay"));      
      }

      //set credentials to airplay server
      if (g_guiSettings.GetBool("services.airplay"))
      {
        CStdString password = g_guiSettings.GetString("services.airplaypassword");
        CAirPlayServer::SetCredentials(g_guiSettings.GetBool("services.useairplaypassword"), 
                                       password);
      }      
    }  
#endif//HAS_AIRPLAY
    else if (strSetting.Equals("network.ipaddress") || strSetting.Equals("network.subnet") || strSetting.Equals("network.gateway") || strSetting.Equals("network.dns"))
    {
#ifdef _LINUX
      bool enabled = (geteuid() == 0);
#else
      bool enabled = false;
#endif
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
      if (pControl1)
         enabled = (pControl1->GetValue() == NETWORK_STATIC);

       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl) pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("network.assignment"))
    {
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
#ifdef HAS_LINUX_NETWORK
      if (pControl1)
         pControl1->SetEnabled(geteuid() == 0);
#endif
    }
    else if (strSetting.Equals("network.essid") || strSetting.Equals("network.enc") || strSetting.Equals("network.key"))
    {
      // Get network information
      CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
      CStdString ifaceName = ifaceControl->GetLabel();
      CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);
      bool bIsWireless = iface->IsWireless();

#ifdef HAS_LINUX_NETWORK
      bool enabled = bIsWireless && (geteuid() == 0);
#else
      bool enabled = bIsWireless;
#endif
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
      if (pControl1)
         enabled &= (pControl1->GetValue() != NETWORK_DISABLED);

      if (strSetting.Equals("network.key"))
      {
         pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
         if (pControl1) enabled &= (pControl1->GetValue() != ENC_NONE);
      }

       CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
       if (pControl) pControl->SetEnabled(enabled);
    }
    else if (strSetting.Equals("network.httpproxyserver")   || strSetting.Equals("network.httpproxyport") ||
             strSetting.Equals("network.httpproxyusername") || strSetting.Equals("network.httpproxypassword"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("network.usehttpproxy"));
    }
#ifdef HAS_LINUX_NETWORK
    else if (strSetting.Equals("network.key"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
      if (pControl && pControl1)
         pControl->SetEnabled(!pControl1->IsDisabled() && pControl1->GetValue() > 0);
    }
    else if (strSetting.Equals("network.save"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(geteuid() == 0);
    }
#endif
    else if (strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpass"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.lastfmsubmit") | g_guiSettings.GetBool("scrobbler.lastfmsubmitradio"));
    }
    else if (strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpass"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("scrobbler.librefmsubmit"));
    }
    else if (strSetting.Equals("subtitles.color") || strSetting.Equals("subtitles.style") || strSetting.Equals("subtitles.charset"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(CUtil::IsUsingTTFSubtitles());
    }
    else if (strSetting.Equals("locale.charset"))
    { // TODO: Determine whether we are using a TTF font or not.
      //   CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      //   if (pControl) pControl->SetEnabled(g_guiSettings.GetString("lookandfeel.font").Right(4) == ".ttf");
    }
    else if (strSetting.Equals("screensaver.settings"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      AddonPtr addon;
      if (CAddonMgr::Get().GetAddon(g_guiSettings.GetString("screensaver.mode"), addon, ADDON_SCREENSAVER))
        pControl->SetEnabled(addon->HasSettings());
      else
        pControl->SetEnabled(false);
    }
    else if (strSetting.Equals("screensaver.preview")           ||
             strSetting.Equals("screensaver.usedimonpause")     ||
             strSetting.Equals("screensaver.usemusicvisinstead"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(!g_guiSettings.GetString("screensaver.mode").IsEmpty());
      if (strSetting.Equals("screensaver.usedimonpause") && g_guiSettings.GetString("screensaver.mode").Equals("screensaver.xbmc.builtin.dim"))
        pControl->SetEnabled(false);
    }
    else if (strSetting.Equals("musicfiles.trackformat"))
    {
      if (m_strOldTrackFormat != g_guiSettings.GetString("musicfiles.trackformat"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
      }
    }
    else if (strSetting.Equals("musicfiles.trackformatright"))
    {
      if (m_strOldTrackFormatRight != g_guiSettings.GetString("musicfiles.trackformatright"))
      {
        CUtil::DeleteMusicDatabaseDirectoryCache();
        m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
      }
    }
#ifdef HAS_TIME_SERVER
    else if (strSetting.Equals("locale.timeserveraddress"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("locale.timeserver"));
    }
#endif
    else if (strSetting.Equals("audiocds.recordingpath") || strSetting.Equals("debug.screenshotpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl && g_guiSettings.GetString(strSetting, false).IsEmpty())
        pControl->SetLabel2("");
    }
    else if (strSetting.Equals("lookandfeel.rssedit"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("lookandfeel.enablerssfeeds"));
    }
    else if (strSetting.Equals("videoplayer.pauseafterrefreshchange"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("videoplayer.adjustrefreshrate"));
    }
    else if (strSetting.Equals("videoplayer.synctype"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("videoplayer.usedisplayasclock"));
    }
    else if (strSetting.Equals("videoplayer.maxspeedadjust"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        bool enabled = (g_guiSettings.GetBool("videoplayer.usedisplayasclock")) &&
            (g_guiSettings.GetInt("videoplayer.synctype") == SYNC_RESAMPLE);
        pControl->SetEnabled(enabled);
      }
    }
    else if (strSetting.Equals("videoplayer.resamplequality"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        bool enabled = (g_guiSettings.GetBool("videoplayer.usedisplayasclock")) &&
            (g_guiSettings.GetInt("videoplayer.synctype") == SYNC_RESAMPLE);
        pControl->SetEnabled(enabled);
      }
    }
    else if (strSetting.Equals("weather.addonsettings"))
    {
      AddonPtr addon;
      if (CAddonMgr::Get().GetAddon(g_guiSettings.GetString("weather.addon"), addon, ADDON_SCRIPT_WEATHER))
      {
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl)
          pControl->SetEnabled(addon->HasSettings());
      }
    }
    else if (strSetting.Equals("input.peripherals"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl)
        pControl->SetEnabled(g_peripherals.GetNumberOfPeripherals() > 0);
    }
#if defined(_LINUX) && !defined(__APPLE__)
    else if (strSetting.Equals("audiooutput.custompassthrough"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (AUDIO_IS_BITSTREAM(g_guiSettings.GetInt("audiooutput.mode")))
      {
        if (pControl) pControl->SetEnabled(g_guiSettings.GetString("audiooutput.passthroughdevice").Equals("custom"));
      }
      else
      {
        if (pControl) pControl->SetEnabled(false);
      }
    }
    else if (strSetting.Equals("audiooutput.customdevice"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetString("audiooutput.audiodevice").Equals("custom"));
    }
#endif
  }

  g_guiSettings.SetChanged();
  g_guiSettings.NotifyObservers("settings", true);
}

void CGUIWindowSettingsCategory::OnClick(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
  if (strSetting.Equals("weather.addonsettings"))
  {
    CStdString name = g_guiSettings.GetString("weather.addon");
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(name, addon, ADDON_SCRIPT_WEATHER))
    { // TODO: maybe have ShowAndGetInput return a bool if settings changed, then only reset weather if true.
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
      g_weatherManager.Refresh();
    }
  }
  else if (strSetting.Equals("lookandfeel.rssedit"))
  {
    AddonPtr addon;
    CAddonMgr::Get().GetAddon("script.rss.editor",addon);
    if (!addon)
    {
      if (!CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(24076), g_localizeStrings.Get(24100),"RSS Editor",g_localizeStrings.Get(24101)))
        return;
      CAddonInstaller::Get().Install("script.rss.editor", true, "", false);
    }
    CBuiltins::Execute("RunScript(script.rss.editor)");
  }
  else if (pSettingControl->GetSetting()->GetType() == SETTINGS_TYPE_ADDON)
  { // prompt for the addon
    CSettingAddon *setting = (CSettingAddon *)pSettingControl->GetSetting();
    CStdString addonID = setting->GetData();
    if (CGUIWindowAddonBrowser::SelectAddonID(setting->m_type, addonID, setting->m_type == ADDON_SCREENSAVER || setting->m_type == ADDON_VIZ || setting->m_type == ADDON_SCRIPT_WEATHER) == 1)
      setting->SetData(addonID);
    else
      return;
  }
  else if (strSetting.Equals("input.peripherals"))
  {
    CGUIDialogPeripheralManager *dialog = (CGUIDialogPeripheralManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PERIPHERAL_MANAGER);
    if (dialog)
      dialog->DoModal();
    return;
  }

  // if OnClick() returns false, the setting hasn't changed or doesn't
  // require immediate update
  if (!pSettingControl->OnClick())
  {
    UpdateSettings();
    if (!pSettingControl->IsDelayed())
      return;
  }

  if (pSettingControl->IsDelayed())
  { // delayed setting
    m_delayedSetting = pSettingControl;
    m_delayedTimer.StartZero();
  }
  else
    OnSettingChanged(pSettingControl);
}

void CGUIWindowSettingsCategory::CheckForUpdates()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    if (pSettingControl->NeedsUpdate())
    {
      OnSettingChanged(pSettingControl);
      pSettingControl->Reset();
    }
  }
}

void CGUIWindowSettingsCategory::OnSettingChanged(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();

  // ok, now check the various special things we need to do
  if (pSettingControl->GetSetting()->GetType() == SETTINGS_TYPE_ADDON)
  {
    CSettingAddon *pSettingAddon = (CSettingAddon*)pSettingControl->GetSetting();
    if (pSettingAddon->m_type == ADDON_SKIN)
    {
      g_application.ReloadSkin();
    }
    else if (pSettingAddon->m_type == ADDON_SCRIPT_WEATHER)
    {
      g_weatherManager.Refresh();
    }
  }
  else if (strSetting.Equals("musicplayer.visualisation"))
  { // new visualisation choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue() == 0)
      pSettingString->SetData("None");
    else
      pSettingString->SetData(pControl->GetCurrentLabel());
  }
  else if (strSetting.Equals("debug.showloginfo"))
  {
    if (g_guiSettings.GetBool("debug.showloginfo"))
    {
      int level = std::max(g_advancedSettings.m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
      g_advancedSettings.m_logLevel = level;
      CLog::SetLogLevel(level);
      CLog::Log(LOGNOTICE, "Enabled debug logging due to GUI setting. Level %d.", level);
    }
    else
    {
      int level = std::min(g_advancedSettings.m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
      CLog::Log(LOGNOTICE, "Disabled debug logging due to GUI setting. Level %d.", level);
      g_advancedSettings.m_logLevel = level;
      CLog::SetLogLevel(level);
    }
  }
  /*else if (strSetting.Equals("musicfiles.repeat"))
  {
    g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("musicfiles.repeat") ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  }*/
  else if (strSetting.Equals("musiclibrary.cleanup"))
  {
    CMusicDatabase musicdatabase;
    musicdatabase.Clean();
    CUtil::DeleteMusicDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("videolibrary.cleanup"))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.CleanDatabase();
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("videolibrary.export"))
    CBuiltins::Execute("exportlibrary(video)");
  else if (strSetting.Equals("musiclibrary.export"))
    CBuiltins::Execute("exportlibrary(music)");
  else if (strSetting.Equals("karaoke.export") )
  {
    CContextButtons choices;
    choices.Add(1, g_localizeStrings.Get(22034));
    choices.Add(2, g_localizeStrings.Get(22035));

    int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if ( retVal > 0 )
    {
      CStdString path(g_settings.GetDatabaseFolder());
      VECSOURCES shares;
      g_mediaManager.GetLocalDrives(shares);
      if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(661), path, true))
      {
        CMusicDatabase musicdatabase;
        musicdatabase.Open();

        if ( retVal == 1 )
        {
          URIUtils::AddFileToFolder(path, "karaoke.html", path);
          musicdatabase.ExportKaraokeInfo( path, true );
        }
        else
        {
          URIUtils::AddFileToFolder(path, "karaoke.csv", path);
          musicdatabase.ExportKaraokeInfo( path, false );
        }
        musicdatabase.Close();
      }
    }
  }
  else if (strSetting.Equals("videolibrary.import"))
  {
    CStdString path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("musiclibrary.import"))
  {
    CStdString path;
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "musicdb.xml", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportFromXML(path);
      musicdatabase.Close();
    }
  }
  else if (strSetting.Equals("karaoke.importcsv"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "karaoke.csv", g_localizeStrings.Get(651) , path))
    {
      CMusicDatabase musicdatabase;
      musicdatabase.Open();
      musicdatabase.ImportKaraokeInfo(path);
      musicdatabase.Close();
    }
  }
  else if (strSetting.Equals("scrobbler.lastfmsubmit") || strSetting.Equals("scrobbler.lastfmsubmitradio") || strSetting.Equals("scrobbler.lastfmusername") || strSetting.Equals("scrobbler.lastfmpass"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.lastfmpass");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.lastfmusername");
    if ((g_guiSettings.GetBool("scrobbler.lastfmsubmit") ||
         g_guiSettings.GetBool("scrobbler.lastfmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLastfmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLastfmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Equals("scrobbler.librefmsubmit") || strSetting.Equals("scrobbler.librefmsubmitradio") || strSetting.Equals("scrobbler.librefmusername") || strSetting.Equals("scrobbler.librefmpass"))
  {
    CStdString strPassword=g_guiSettings.GetString("scrobbler.librefmpass");
    CStdString strUserName=g_guiSettings.GetString("scrobbler.librefmusername");
    if ((g_guiSettings.GetBool("scrobbler.librefmsubmit") ||
         g_guiSettings.GetBool("scrobbler.librefmsubmitradio")) &&
         !strUserName.IsEmpty() && !strPassword.IsEmpty())
    {
      CLibrefmScrobbler::GetInstance()->Init();
    }
    else
    {
      CLibrefmScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
  else if (strSetting.Equals("audiooutput.audiodevice"))
  {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
#if !defined(__APPLE__)
      g_guiSettings.SetString("audiooutput.audiodevice", m_AnalogAudioSinkMap[pControl->GetCurrentLabel()]);
#else
      g_guiSettings.SetString("audiooutput.audiodevice", pControl->GetCurrentLabel());
#endif
  }
#if defined(_LINUX)
  else if (strSetting.Equals("audiooutput.passthroughdevice"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
#if defined(_LINUX) && !defined(__APPLE__)
      g_guiSettings.SetString("audiooutput.passthroughdevice", m_DigitalAudioSinkMap[pControl->GetCurrentLabel()]);
#elif !defined(__arm__)
      g_guiSettings.SetString("audiooutput.passthroughdevice", pControl->GetCurrentLabel());
#endif
  }
#endif
#ifdef HAS_LCD
  else if (strSetting.Equals("videoscreen.haslcd"))
  {
    g_lcd->Stop();
    CLCDFactory factory;
    delete g_lcd;
    g_lcd = factory.Create();
    g_lcd->Initialize();
  }
#endif
#ifdef HAS_WEB_SERVER
  else if ( strSetting.Equals("services.webserver") || strSetting.Equals("services.webserverport"))
  {
    if (strSetting.Equals("services.webserverport"))
      ValidatePortNumber(pSettingControl, "8080", "80");
    g_application.StopWebServer();
    if (g_guiSettings.GetBool("services.webserver"))
      if (!g_application.StartWebServer())
      {
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33101), "", g_localizeStrings.Get(33100), "");
        g_guiSettings.SetBool("services.webserver", false);
      }
  }
  else if (strSetting.Equals("services.webserverusername") || strSetting.Equals("services.webserverpassword"))
  {
    g_application.m_WebServer.SetCredentials(g_guiSettings.GetString("services.webserverusername"), g_guiSettings.GetString("services.webserverpassword"));
  }
#endif
  else if (strSetting.Equals("services.zeroconf"))
  {
#ifdef HAS_ZEROCONF
    //ifdef zeroconf here because it's only found in guisettings if defined
    if(g_guiSettings.GetBool("services.zeroconf"))
    {
      CZeroconf::GetInstance()->Stop();
      CZeroconf::GetInstance()->Start();
    }
#ifdef HAS_AIRPLAY
    else
    {
      g_application.StopAirplayServer(true);
      g_guiSettings.SetBool("services.airplay", false);
      CZeroconf::GetInstance()->Stop();
    }
#endif
#endif
  }
  else if (strSetting.Equals("services.airplay"))
  {  
#ifdef HAS_AIRPLAY  
    if (g_guiSettings.GetBool("services.airplay"))
    {
#ifdef HAS_ZEROCONF
      // AirPlay needs zeroconf
      if(!g_guiSettings.GetBool("services.zeroconf"))
      {
        g_guiSettings.SetBool("services.zeroconf", true);
        CZeroconf::GetInstance()->Stop();
        CZeroconf::GetInstance()->Start();
      }
#endif //HAS_ZEROCONF
      g_application.StartAirplayServer();//will stop the server before internal
    }
    else
      g_application.StopAirplayServer(true);//will stop the server before internal    
#endif//HAS_AIRPLAY      
  }
  else if (strSetting.Equals("network.ipaddress"))
  {
    if (g_guiSettings.GetInt("network.assignment") == NETWORK_STATIC)
    {
      CStdString strDefault = g_guiSettings.GetString("network.ipaddress").Left(g_guiSettings.GetString("network.ipaddress").ReverseFind('.'))+".1";
      if (g_guiSettings.GetString("network.gateway").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.gateway",strDefault);
      if (g_guiSettings.GetString("network.dns").Equals("0.0.0.0"))
        g_guiSettings.SetString("network.dns",strDefault);

    }
  }

  else if (strSetting.Equals("network.httpproxyport"))
  {
    ValidatePortNumber(pSettingControl, "8080", "8080", false);
  }
  else if (strSetting.Equals("videoplayer.calibrate") || strSetting.Equals("videoscreen.guicalibration"))
  { // activate the video calibration screen
    g_windowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  else if (strSetting.Equals("videoscreen.testpattern"))
  { // activate the test pattern
    g_windowManager.ActivateWindow(WINDOW_TEST_PATTERN);
  }
  else if (strSetting.Equals("subtitles.height"))
  {
    if (!CUtil::IsUsingTTFSubtitles())
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
    }
  }
  else if (strSetting.Equals("subtitles.font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    CSetting *pSetting = (CSetting *)g_guiSettings.GetSetting("subtitles.height");
    FillInSubtitleHeights(pSetting, (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID()));
  }
  else if (strSetting.Equals("subtitles.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("karaoke.fontheight"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
  }
  else if (strSetting.Equals("karaoke.font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    CSetting *pSetting = (CSetting *)g_guiSettings.GetSetting("karaoke.fontheight");
    FillInSubtitleHeights(pSetting, (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID()));
  }
  else if (strSetting.Equals("karaoke.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
      newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("locale.charset"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset="DEFAULT";
    if (pControl->GetValue()!=0)
     newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && (newCharset != pSettingString->GetData() || newCharset=="DEFAULT"))
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting.Equals("lookandfeel.font"))
  { // new font choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkinFontSet = m_SkinFontSetIDs[pControl->GetCurrentLabel()];
    if (strSkinFontSet != ".svn" && strSkinFontSet != g_guiSettings.GetString("lookandfeel.font"))
    {
      g_guiSettings.SetString("lookandfeel.font", strSkinFontSet);
      g_application.ReloadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.soundskin"))
  { // new sound skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue()==0)
      g_guiSettings.SetString("lookandfeel.soundskin", "OFF");
    else if (pControl->GetValue()==1)
      g_guiSettings.SetString("lookandfeel.soundskin", "SKINDEFAULT");
    else
      g_guiSettings.SetString("lookandfeel.soundskin", pControl->GetCurrentLabel());

    g_audioManager.Enable(true);
    g_audioManager.Load();
  }
  else if (strSetting.Equals("input.enablemouse"))
  {
    g_Mouse.SetEnabled(g_guiSettings.GetBool("input.enablemouse"));
  }
  else if (strSetting.Equals("videoscreen.screen"))
  {
    DisplayMode mode = g_guiSettings.GetInt("videoscreen.screen");
    // Cascade
    FillInResolutions("videoscreen.resolution", mode, RES_DESKTOP, true);
  }
  else if (strSetting.Equals("videoscreen.resolution"))
  {
    RESOLUTION nextRes = (RESOLUTION) g_guiSettings.GetInt("videoscreen.resolution");
    // Cascade
    FillInRefreshRates("videoscreen.screenmode", nextRes, true);
  }
  else if (strSetting.Equals("videoscreen.screenmode"))
  {
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    RESOLUTION nextRes = (RESOLUTION)msg.GetParam1();

    OnRefreshRateChanged(nextRes);
  }
  else if (strSetting.Equals("videoscreen.vsync"))
  {
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
// DXMERGE: This may be useful
//    g_videoConfig.SetVSyncMode((VSYNC)msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.fakefullscreen"))
  {
    if (g_graphicsContext.IsFullScreenRoot())
      g_graphicsContext.SetVideoResolution(g_graphicsContext.GetVideoResolution(), true);
  }
  else if (strSetting.Equals("locale.language"))
  { // new language chosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strLanguage = pControl->GetCurrentLabel();
    if (strLanguage != ".svn" && strLanguage != pSettingString->GetData())
      g_guiSettings.SetLanguage(strLanguage);
  }
  else if (strSetting.Equals("lookandfeel.skintheme"))
  { //a new Theme was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinTheme;

    if (pControl->GetValue() == 0) // Use default theme
      strSkinTheme = "SKINDEFAULT";
    else
      strSkinTheme = pControl->GetCurrentLabel();

    if (strSkinTheme != pSettingString->GetData())
    {
      g_guiSettings.SetString("lookandfeel.skintheme", strSkinTheme);
      // also set the default color theme
      CStdString colorTheme(URIUtils::ReplaceExtension(strSkinTheme, ".xml"));
      if (colorTheme.Equals("Textures.xml"))
        colorTheme = "defaults.xml";
      g_guiSettings.SetString("lookandfeel.skincolors", colorTheme);
      g_application.ReloadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skincolors"))
  { //a new color was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinColor;

    if (pControl->GetValue() == 0) // Use default colors
      strSkinColor = "SKINDEFAULT";
    else
      strSkinColor = pControl->GetCurrentLabel() + ".xml";

    if (strSkinColor != pSettingString->GetData())
    {
      g_guiSettings.SetString("lookandfeel.skincolors", strSkinColor);
      g_application.ReloadSkin();
    }
  }
  else if (strSetting.Equals("videoplayer.displayresolution"))
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID);
    g_windowManager.SendMessage(msg);
    pSettingInt->SetData(msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.flickerfilter") || strSetting.Equals("videoscreen.soften"))
  { // reset display
    g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution);
  }
  else if (strSetting.Equals("screensaver.preview"))
  {
    g_application.ActivateScreenSaver(true);
  }
  else if (strSetting.Equals("screensaver.settings"))
  {
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(g_guiSettings.GetString("screensaver.mode"), addon, ADDON_SCREENSAVER))
      CGUIDialogAddonSettings::ShowAndGetInput(addon);
  }
  else if (strSetting.Equals("debug.screenshotpath") || strSetting.Equals("audiocds.recordingpath") || strSetting.Equals("subtitles.custompath") || strSetting.Equals("pvrmenu.iconpath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = g_guiSettings.GetString(strSetting,false);
    VECSOURCES shares;

    bool bWriteOnly = true;

    if (strSetting.Equals("pvrmenu.iconpath"))
    {
      bWriteOnly = false;
    }
    else if (strSetting.Equals("subtitles.custompath"))
    {
      bWriteOnly = false;
      shares = g_settings.m_videoSources;
    }

    g_mediaManager.GetNetworkLocations(shares);
    g_mediaManager.GetLocalDrives(shares);

    UpdateSettings();

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path, bWriteOnly))
    {
      pSettingString->SetData(path);
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
  else if (strSetting.Equals("locale.country"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    const CStdString& strRegion=pControl->GetCurrentLabel();
    g_langInfo.SetCurrentRegion(strRegion);
    g_guiSettings.SetString("locale.country", strRegion);
    g_weatherManager.Refresh(); // need to reset our weather, as temperatures need re-translating.
  }
#ifdef HAS_TIME_SERVER
  else if (strSetting.Equals("locale.timeserver") || strSetting.Equals("locale.timeserveraddress"))
  {
    g_application.StopTimeServer();
    if (g_guiSettings.GetBool("locale.timeserver"))
      g_application.StartTimeServer();
  }
#endif
  else if (strSetting.Equals("smb.winsserver") || strSetting.Equals("smb.workgroup") )
  {
    if (g_guiSettings.GetString("smb.winsserver") == "0.0.0.0")
      g_guiSettings.SetString("smb.winsserver", "");

    /* okey we really don't need to restarat, only deinit samba, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogOK *dlg = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!dlg) return ;
    dlg->SetHeading( g_localizeStrings.Get(14038) );
    dlg->SetLine( 0, g_localizeStrings.Get(14039) );
    dlg->SetLine( 1, g_localizeStrings.Get(14040));
    dlg->SetLine( 2, "");
    dlg->DoModal();

    if (dlg->IsConfirmed())
    {
      g_settings.Save();
      g_application.getApplicationMessenger().RestartApp();
    }
  }
  else if (strSetting.Equals("services.upnpserver"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("services.upnpserver"))
      g_application.StartUPnPServer();
    else
      g_application.StopUPnPServer();
#endif
  }
  else if (strSetting.Equals("services.upnprenderer"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("services.upnprenderer"))
      g_application.StartUPnPRenderer();
    else
      g_application.StopUPnPRenderer();
#endif
  }
  else if (strSetting.Equals("services.esenabled"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      if (!g_application.StartEventServer())
      {
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33102), "", g_localizeStrings.Get(33100), "");
        g_guiSettings.SetBool("services.esenabled", false);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(false);
      }
    }
    else
    {
      if (!g_application.StopEventServer(true, true))
      {
        g_guiSettings.SetBool("services.esenabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
#ifdef HAS_JSONRPC
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      if (!g_application.StartJSONRPCServer())
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33103), "", g_localizeStrings.Get(33100), "");
    }
    else
      g_application.StopJSONRPCServer(false);
#endif
  }
  else if (strSetting.Equals("services.esport"))
  {
#ifdef HAS_EVENT_SERVER
    ValidatePortNumber(pSettingControl, "9777", "9777");
    //restart eventserver without asking user
    if (g_application.StopEventServer(true, false))
    {
      if (!g_application.StartEventServer())
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33102), "", g_localizeStrings.Get(33100), "");
    }
#if defined(__APPLE__) && !defined(__arm__)
    //reconfigure XBMCHelper for port changes
    XBMCHelper::GetInstance().Configure();
#endif
#endif
  }
  else if (strSetting.Equals("services.esallinterfaces"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      if (g_application.StopEventServer(true, true))
      {
        if (!g_application.StartEventServer())
          CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33102), "", g_localizeStrings.Get(33100), "");
      }
      else
      {
        g_guiSettings.SetBool("services.esenabled", true);
        CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
        if (pControl) pControl->SetEnabled(true);
      }
    }
#endif
#ifdef HAS_JSONRPC
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      if (!g_application.StartJSONRPCServer())
        CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(33103), "", g_localizeStrings.Get(33100), "");
    }
    else
      g_application.StopJSONRPCServer(false);
#endif
  }
  else if (strSetting.Equals("services.esinitialdelay") ||
           strSetting.Equals("services.escontinuousdelay"))
  {
#ifdef HAS_EVENT_SERVER
    if (g_guiSettings.GetBool("services.esenabled"))
    {
      g_application.RefreshEventServer();
    }
#endif
  }
  else if (strSetting.Equals("pvrmanager.enabled"))
  {
    if (g_guiSettings.GetBool("pvrmanager.enabled"))
      g_application.StartPVRManager();
    else
      g_application.StopPVRManager();
  }
  else if (strSetting.Equals("masterlock.lockcode"))
  {
    // Now Prompt User to enter the old and then the new MasterCode!
    if(g_passwordManager.SetMasterLockMode())
    {
      // We asked for the master password and saved the new one!
      // Nothing todo here
    }
  }
  else if (strSetting.Equals("network.interface"))
  {
     NetworkInterfaceChanged();
  }
#ifdef HAS_LINUX_NETWORK
  else if (strSetting.Equals("network.save"))
  {
     NetworkAssignment iAssignment;
     CStdString sIPAddress;
     CStdString sNetworkMask;
     CStdString sDefaultGateway;
     CStdString sWirelessNetwork;
     CStdString sWirelessKey;
     CStdString sDns;
     EncMode iWirelessEnc;
     CStdString ifaceName;

     CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
     ifaceName = ifaceControl->GetLabel();
     CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);

     // Update controls with information
     CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
     if (pControl1) iAssignment = (NetworkAssignment) pControl1->GetValue();
     CGUIButtonControl* pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.ipaddress")->GetID());
     if (pControl2) sIPAddress = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.subnet")->GetID());
     if (pControl2) sNetworkMask = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.gateway")->GetID());
     if (pControl2) sDefaultGateway = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.dns")->GetID());
     if (pControl2) sDns = pControl2->GetLabel2();
     pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
     if (pControl1) iWirelessEnc = (EncMode) pControl1->GetValue();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.essid")->GetID());
     if (pControl2) sWirelessNetwork = pControl2->GetLabel2();
     pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.key")->GetID());
     if (pControl2) sWirelessKey = pControl2->GetLabel2();

     CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
     pDlgProgress->SetLine(0, "");
     pDlgProgress->SetLine(1, g_localizeStrings.Get(784));
     pDlgProgress->SetLine(2, "");
     pDlgProgress->StartModal();
     pDlgProgress->Progress();

     std::vector<CStdString> nameServers;
     nameServers.push_back(sDns);
     g_application.getNetwork().SetNameServers(nameServers);
     iface->SetSettings(iAssignment, sIPAddress, sNetworkMask, sDefaultGateway, sWirelessNetwork, sWirelessKey, iWirelessEnc);

     pDlgProgress->Close();

     if (iAssignment == NETWORK_DISABLED)
        CGUIDialogOK::ShowAndGetInput(0, 788, 0, 0);
     else if (iface->IsConnected())
        CGUIDialogOK::ShowAndGetInput(0, 785, 0, 0);
     else
        CGUIDialogOK::ShowAndGetInput(0, 786, 0, 0);
  }
  else if (strSetting.Equals("network.essid"))
  {
    CGUIDialogAccessPoints *dialog = (CGUIDialogAccessPoints *)g_windowManager.GetWindow(WINDOW_DIALOG_ACCESS_POINTS);
    if (dialog)
    {
       CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
       dialog->SetInterfaceName(pControl->GetLabel());
       dialog->DoModal();

       if (dialog->WasItemSelected())
       {
          CGUIButtonControl* pControl2 = (CGUIButtonControl *)GetControl(GetSetting("network.essid")->GetID());
          if (pControl2) pControl2->SetLabel2(dialog->GetSelectedAccessPointEssId());
          pControl = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
          if (pControl) pControl->SetValue(dialog->GetSelectedAccessPointEncMode());
       }
    }
  }
#endif
#ifdef _LINUX
  else if (strSetting.Equals("locale.timezonecountry"))
  {
    CGUISpinControlEx *pControlCountry = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString country = pControlCountry->GetCurrentLabel();

    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezone")->GetID());
    pControl->Clear();
    vector<CStdString> timezones = g_timezone.GetTimezonesByCountry(country);
    for (unsigned int i=0; i < timezones.size(); i++)
    {
      pControl->AddLabel(timezones[i], i);
    }

    g_timezone.SetTimezone(pControl->GetLabel());
    g_guiSettings.SetString("locale.timezonecountry",pControlCountry->GetLabel().c_str());

    CGUISpinControlEx *tzControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezone")->GetID());
    g_guiSettings.SetString("locale.timezone", tzControl->GetLabel().c_str());
  }
  else  if (strSetting.Equals("locale.timezone"))
  {
     CGUISpinControlEx *tzControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezone")->GetID());
     g_timezone.SetTimezone(tzControl->GetLabel());
     g_guiSettings.SetString("locale.timezone", tzControl->GetLabel().c_str());

     tzControl = (CGUISpinControlEx *)GetControl(GetSetting("locale.timezonecountry")->GetID());
     g_guiSettings.SetString("locale.timezonecountry", tzControl->GetLabel().c_str());
  }
#endif
  else if (strSetting.Equals("lookandfeel.skinzoom"))
  {
    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
  }
  else if (strSetting.Equals("videolibrary.flattentvshows") ||
           strSetting.Equals("videolibrary.removeduplicates"))
  {
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("pvrmenu.searchicons") && g_PVRManager.IsStarted())
  {
    g_PVRManager.SearchMissingChannelIcons();
  }
  else if (strSetting.Equals("pvrmanager.resetdb"))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(19098, 19186, 750, 0))
      g_PVRManager.ResetDatabase();
  }
  else if (strSetting.Equals("epg.resetepg"))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(19098, 19188, 750, 0))
      g_PVRManager.ResetEPG();
  }
  else if (strSetting.Equals("pvrmanager.channelscan") && g_PVRManager.IsStarted())
  {
    if (CGUIDialogYesNo::ShowAndGetInput(19098, 19118, 19194, 0))
      g_PVRManager.StartChannelScan();
  }
  else if (strSetting.Equals("pvrmanager.channelmanager") && g_PVRManager.IsStarted())
  {
    CGUIDialogPVRChannelManager *dialog = (CGUIDialogPVRChannelManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
    if (dialog)
    {
       dialog->DoModal();
    }
  }

  UpdateSettings();
}

void CGUIWindowSettingsCategory::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_vecSections.clear();
  FreeSettingsControls();
}

void CGUIWindowSettingsCategory::FreeSettingsControls()
{
  // clear the settings group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (control)
  {
    control->FreeResources();
    control->ClearAll();
  }

  for(int i = 0; (size_t)i < m_vecSettings.size(); i++)
  {
    delete m_vecSettings[i];
  }
  m_vecSettings.clear();
}

CGUIControl* CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, float width, int &iControlID)
{
  if (!pSetting->IsVisible()) return NULL;  // not displayed in current session
  CBaseSettingControl *pSettingControl = NULL;
  CGUIControl *pControl = NULL;
  if (pSetting->GetControlType() == CHECKMARK_CONTROL)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return NULL;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return NULL;
    pControl->SetWidth(width);
    ((CGUISpinControlEx *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
    pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SEPARATOR_CONTROL && m_pOriginalImage)
  {
    pControl = new CGUIImage(*m_pOriginalImage);
    if (!pControl) return NULL;
    pControl->SetWidth(width);
    pSettingControl = new CSeparatorSettingControl((CGUIImage *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == EDIT_CONTROL_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_HIDDEN_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_MD5_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_NUMBER_INPUT ||
           pSetting->GetControlType() == EDIT_CONTROL_IP_INPUT)
  {
    pControl = new CGUIEditControl(*m_pOriginalEdit);
    if (!pControl) return NULL;
    ((CGUIEditControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIEditControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CEditSettingControl((CGUIEditControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() != SEPARATOR_CONTROL) // button control
  {
    pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl) return NULL;
    ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CButtonSettingControl((CGUIButtonControl *)pControl, iControlID, pSetting);
  }
  if (!pControl)
  {
    delete pSettingControl;
    return NULL;
  }
  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
    m_vecSettings.push_back(pSettingControl);
  }
  return pControl;
}

void CGUIWindowSettingsCategory::FrameMove()
{
  if (m_delayedSetting && m_delayedTimer.GetElapsedMilliseconds() > 3000)
  { // we send a thread message so that it's processed the following frame (some settings won't
    // like being changed during Render())
    CGUIMessage message(GUI_MSG_UPDATE_ITEM, GetID(), GetID());
    g_windowManager.SendThreadMessage(message, GetID());
    m_delayedTimer.Stop();
  }
  CGUIWindow::FrameMove();
}

void CGUIWindowSettingsCategory::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_BUTTONS + m_iSection);
  if (control && !control->HasFocus())
  {
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetAlpha(0x80);
      bAlphaFaded = true;
    }
    else if (control->GetControlType() == CGUIControl::GUICONTROL_TOGGLEBUTTON)
    {
      control->SetFocus(true);
      ((CGUIButtonControl *)control)->SetSelected(true);
      bAlphaFaded = true;
    }
  }
  CGUIWindow::DoProcess(currentTime, dirtyregions);
  if (bAlphaFaded)
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      ((CGUIButtonControl *)control)->SetAlpha(0xFF);
    else
      ((CGUIButtonControl *)control)->SetSelected(false);
  }
}

void CGUIWindowSettingsCategory::Render()
{
  CGUIWindow::Render();
}

void CGUIWindowSettingsCategory::FillInSubtitleHeights(CSetting *pSetting, CGUISpinControlEx *pControl)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  if (CUtil::IsUsingTTFSubtitles())
  { // easy - just fill as per usual
    CStdString strLabel;
    for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i += pSettingInt->m_iStep)
    {
      if (pSettingInt->m_iFormat > -1)
      {
        CStdString strFormat = g_localizeStrings.Get(pSettingInt->m_iFormat);
        strLabel.Format(strFormat, i);
      }
      else
        strLabel.Format(pSettingInt->m_strFormat, i);
      pControl->AddLabel(strLabel, i);
    }
    pControl->SetValue(pSettingInt->GetData());
  }
}

void CGUIWindowSettingsCategory::FillInSubtitleFonts(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentFont = 0;
  int iFont = 0;

  // find TTF fonts
  {
    CFileItemList items;
    CFileItemList items2;
    CDirectory::GetDirectory("special://home/media/Fonts/", items2);

    if (CDirectory::GetDirectory("special://xbmc/media/Fonts/", items))
    {
      items.Append(items2);
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];

        if (!pItem->m_bIsFolder)
        {

          if ( !URIUtils::GetExtension(pItem->GetLabel()).Equals(".ttf") ) continue;
          if (pItem->GetLabel().Equals(pSettingString->GetData(), false))
            iCurrentFont = iFont;

          pControl->AddLabel(pItem->GetLabel(), iFont++);
        }

      }
    }
  }
  pControl->SetValue(iCurrentFont);
}

void CGUIWindowSettingsCategory::FillInSkinFonts(CSetting *pSetting)
{
  CBaseSettingControl *setting = GetSetting(pSetting->GetSetting());
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(setting->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  setting->SetDelayed();

  m_SkinFontSetIDs.clear();
  int iSkinFontSet = 0;

  CStdString strPath = g_SkinInfo->GetSkinPath("Font.xml");

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Couldn't load %s", strPath.c_str());
    return ;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts"))
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>", strPath.c_str());
    return ;
  }

  const TiXmlNode *pChild = pRootElement->FirstChild();
  strValue = pChild->Value();
  if (strValue == "fontset")
  {
    while (pChild)
    {
      strValue = pChild->Value();
      if (strValue == "fontset")
      {
        const char* idAttr = ((TiXmlElement*) pChild)->Attribute("id");
        const char* idLocAttr = ((TiXmlElement*) pChild)->Attribute("idloc");
        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        bool isUnicode=(unicodeAttr && stricmp(unicodeAttr, "true") == 0);

        bool isAllowed=true;
        if (g_langInfo.ForceUnicodeFont() && !isUnicode)
          isAllowed=false;

        if (idAttr != NULL && isAllowed)
        {
          if (idLocAttr) 
          {
            pControl->AddLabel(g_localizeStrings.Get(atoi(idLocAttr)), iSkinFontSet); 
            m_SkinFontSetIDs[g_localizeStrings.Get(atoi(idLocAttr))] = idAttr;
          }
          else
          {
            pControl->AddLabel(idAttr, iSkinFontSet);
            m_SkinFontSetIDs[idAttr] = idAttr;
          }
          if (strcmpi(idAttr, g_guiSettings.GetString("lookandfeel.font").c_str()) == 0)
            pControl->SetValue(iSkinFontSet);
          iSkinFontSet++;
        }
      }
      pChild = pChild->NextSibling();
    }

  }
  else
  {
    // Since no fontset is defined, there is no selection of a fontset, so disable the component
    pControl->AddLabel(g_localizeStrings.Get(13278), 1);
    pControl->SetValue(1);
    pControl->SetEnabled(false);
  }
}

void CGUIWindowSettingsCategory::FillInSoundSkins(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  //find skins...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/sounds/", items);
  CDirectory::GetDirectory("special://home/sounds/", items);

  int iCurrentSoundSkin = 0;
  int iSoundSkin = 0;
  vector<CStdString> vecSoundSkins;
  int i;
  for (i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecSoundSkins.push_back(pItem->GetLabel());
    }
  }

  pControl->AddLabel(g_localizeStrings.Get(474), iSoundSkin++); // Off
  pControl->AddLabel(g_localizeStrings.Get(15109), iSoundSkin++); // Skin Default

  if (g_guiSettings.GetString("lookandfeel.soundskin")=="SKINDEFAULT")
    iCurrentSoundSkin=1;

  sort(vecSoundSkins.begin(), vecSoundSkins.end(), sortstringbyname());
  for (i = 0; i < (int) vecSoundSkins.size(); ++i)
  {
    CStdString strSkin = vecSoundSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.soundskin").c_str()) == 0)
    {
      iCurrentSoundSkin = iSoundSkin;
    }
    pControl->AddLabel(strSkin, iSoundSkin++);
  }
  pControl->SetValue(iCurrentSoundSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInCharSets(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  int iCurrentCharset = 0;
  vector<CStdString> vecCharsets = g_charsetConverter.getCharsetLabels();

  CStdString strCurrentCharsetLabel="DEFAULT";
  if (pSettingString->GetData()!="DEFAULT")
    strCurrentCharsetLabel = g_charsetConverter.getCharsetLabelByName(pSettingString->GetData());

  sort(vecCharsets.begin(), vecCharsets.end(), sortstringbyname());

  vecCharsets.insert(vecCharsets.begin(), g_localizeStrings.Get(13278)); // "Default"

  bool bIsAuto=(pSettingString->GetData()=="DEFAULT");

  for (int i = 0; i < (int) vecCharsets.size(); ++i)
  {
    CStdString strCharsetLabel = vecCharsets[i];

    if (!bIsAuto && strCharsetLabel == strCurrentCharsetLabel)
      iCurrentCharset = i;

    pControl->AddLabel(strCharsetLabel, i);
  }

  pControl->SetValue(iCurrentCharset);
}

DisplayMode CGUIWindowSettingsCategory::FillInScreens(CStdString strSetting, RESOLUTION res)
{
  DisplayMode mode;
  if (res == RES_WINDOW)
    mode = DM_WINDOWED;
  else
    mode = g_settings.m_ResInfo[res].iScreen;

  // we expect "videoscreen.screen" but it might be hidden on some platforms,
  // so check that we actually have a visable control.
  CBaseSettingControl *control = GetSetting(strSetting);
  if (control)
  {
    control->SetDelayed();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(control->GetID());
    pControl->Clear();

    CStdString strScreen;
    if (g_advancedSettings.m_canWindowed)
      pControl->AddLabel(g_localizeStrings.Get(242), -1);

    for (int idx = 0; idx < g_Windowing.GetNumScreens(); idx++)
    {
      strScreen.Format(g_localizeStrings.Get(241), g_settings.m_ResInfo[RES_DESKTOP + idx].iScreen + 1);
      pControl->AddLabel(strScreen, g_settings.m_ResInfo[RES_DESKTOP + idx].iScreen);
    }
    pControl->SetValue(mode);
    g_guiSettings.SetInt("videoscreen.screen", mode);
  }

  return mode;
}

void CGUIWindowSettingsCategory::FillInResolutions(CStdString strSetting, DisplayMode mode, RESOLUTION res, bool UserChange)
{
  CBaseSettingControl *control = GetSetting(strSetting);
  control->SetDelayed();
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(control->GetID());

  pControl->Clear();

  RESOLUTION spinres = RES_INVALID; // index of the resolution in the spinner that has same screen/width/height as res

  if (mode == DM_WINDOWED)
  {
    pControl->AddLabel(g_localizeStrings.Get(242), RES_WINDOW);
    spinres = RES_WINDOW;
  }
  else
  {
    vector<RESOLUTION_WHR> resolutions = g_Windowing.ScreenResolutions(mode);

    for (unsigned int idx = 0; idx < resolutions.size(); idx++)
    {
      CStdString strRes;
      strRes.Format("%dx%d", resolutions[idx].width, resolutions[idx].height);
      pControl->AddLabel(strRes, resolutions[idx].ResInfo_Index);

      RESOLUTION_INFO res1 = g_settings.m_ResInfo[res];
      RESOLUTION_INFO res2 = g_settings.m_ResInfo[resolutions[idx].ResInfo_Index];
      if (res1.iScreen == res2.iScreen && res1.iWidth == res2.iWidth && res1.iHeight == res2.iHeight)
        spinres = (RESOLUTION) resolutions[idx].ResInfo_Index;
    }
  }

  if (UserChange)
  {
    // Auto-select the windowed or desktop resolution of the screen
    int autoresolution = RES_DESKTOP;
    if (mode == DM_WINDOWED)
    {
      autoresolution = RES_WINDOW;
    }
    else
    {
      for (int idx=0; idx < g_Windowing.GetNumScreens(); idx++)
        if (g_settings.m_ResInfo[RES_DESKTOP + idx].iScreen == mode)
        {
          autoresolution = RES_DESKTOP + idx;
          break;
        }
    }
    pControl->SetValue(autoresolution);

    // Cascade
    FillInRefreshRates("videoscreen.screenmode", (RESOLUTION) autoresolution, true);
  }
  else
  {
    // select the entry equivalent to the resolution passed by the res parameter
    pControl->SetValue(spinres);
  }
}

void CGUIWindowSettingsCategory::FillInRefreshRates(CStdString strSetting, RESOLUTION res, bool UserChange)
{
  // The only meaningful parts of res here are iScreen, iWidth, iHeight

  vector<REFRESHRATE> refreshrates;
  if (res > RES_WINDOW)
    refreshrates = g_Windowing.RefreshRates(g_settings.m_ResInfo[res].iScreen, g_settings.m_ResInfo[res].iWidth, g_settings.m_ResInfo[res].iHeight);

  // The control setting doesn't exist when not in standalone mode, don't manipulate it
  CBaseSettingControl *control = GetSetting(strSetting);
  CGUISpinControlEx *pControl= NULL;

  // Populate
  if (control)
  {
    control->SetDelayed();
    pControl = (CGUISpinControlEx *)GetControl(control->GetID());
    pControl->Clear();

    if (res == RES_WINDOW)
    {
      pControl->AddLabel(g_localizeStrings.Get(242), RES_WINDOW);
    }
    else
    {
      for (unsigned int idx = 0; idx < refreshrates.size(); idx++)
      {
        CStdString strRR;
        strRR.Format("%.02f%s", refreshrates[idx].RefreshRate, refreshrates[idx].Interlaced ? "i" : "");
        pControl->AddLabel(strRR, refreshrates[idx].ResInfo_Index);
      }
    }
  }

  // Select a rate
  if (UserChange)
  {
    RESOLUTION newresolution;
    if (res == RES_WINDOW)
      newresolution = RES_WINDOW;
    else
      newresolution = (RESOLUTION) g_Windowing.DefaultRefreshRate(g_settings.m_ResInfo[res].iScreen, refreshrates).ResInfo_Index;

    if (pControl)
      pControl->SetValue(newresolution);

    OnRefreshRateChanged(newresolution);
  }
  else
  {
    if (pControl)
      pControl->SetValue(res);
  }
}

void CGUIWindowSettingsCategory::OnRefreshRateChanged(RESOLUTION nextRes)
{
  RESOLUTION lastRes = g_graphicsContext.GetVideoResolution();
  bool cancelled = false;

  g_guiSettings.SetResolution(nextRes);
  g_graphicsContext.SetVideoResolution(nextRes);

  if (!CGUIDialogYesNo::ShowAndGetInput(13110, 13111, 20022, 20022, -1, -1, cancelled, 10000))
  {
    g_guiSettings.SetResolution(lastRes);
    g_graphicsContext.SetVideoResolution(lastRes);

    DisplayMode mode = FillInScreens("videoscreen.screen", lastRes);
    FillInResolutions("videoscreen.resolution", mode, lastRes, false);
    FillInRefreshRates("videoscreen.screenmode", lastRes, false);
  }
}

void CGUIWindowSettingsCategory::FillInLanguages(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString *)pSetting;
  CBaseSettingControl *setting = GetSetting(pSetting->GetSetting());
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(setting->GetID());
  setting->SetDelayed();
  pControl->Clear();

  //find languages...
  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/language/", items);

  int iCurrentLang = 0;
  int iLanguage = 0;
  vector<CStdString> vecLanguage;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecLanguage.push_back(pItem->GetLabel());
    }
  }

  sort(vecLanguage.begin(), vecLanguage.end(), sortstringbyname());
  for (unsigned int i = 0; i < vecLanguage.size(); ++i)
  {
    CStdString strLanguage = vecLanguage[i];
    if (strcmpi(strLanguage.c_str(), pSettingString->GetData().c_str()) == 0)
      iCurrentLang = iLanguage;
    pControl->AddLabel(strLanguage, iLanguage++);
  }

  pControl->SetValue(iCurrentLang);
}

void CGUIWindowSettingsCategory::FillInRegions(CSetting *pSetting)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iCurrentRegion=0;
  CStdStringArray regions;
  g_langInfo.GetRegionNames(regions);

  CStdString strCurrentRegion=g_langInfo.GetCurrentRegion();

  sort(regions.begin(), regions.end(), sortstringbyname());

  for (int i = 0; i < (int) regions.size(); ++i)
  {
    const CStdString& strRegion = regions[i];

    if (strRegion == strCurrentRegion)
      iCurrentRegion = i;

    pControl->AddLabel(strRegion, i);
  }

  pControl->SetValue(iCurrentRegion);
}

CBaseSettingControl *CGUIWindowSettingsCategory::GetSetting(const CStdString &strSetting)
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    if (m_vecSettings[i]->GetSetting()->GetSetting() == strSetting)
      return m_vecSettings[i];
  }
  return NULL;
}

void CGUIWindowSettingsCategory::FillInSkinThemes(CSetting *pSetting)
{
  // There is a default theme (just Textures.xpr/xbt)
  // any other *.xpr|*.xbt files are additional themes on top of this one.
  CSettingString *pSettingString = (CSettingString *)pSetting;
  CBaseSettingControl *setting = GetSetting(pSetting->GetSetting());
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(setting->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skintheme");
  setting->SetDelayed();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT" The standard Textures.xpr/xbt will be used

  CStdString strDefaultTheme = pSettingString->GetData();

  // Search for Themes in the Current skin!
  vector<CStdString> vecTheme;
  CUtil::GetSkinThemes(vecTheme);

  // Remove the extension from the current Theme (backward compat)
  URIUtils::RemoveExtension(strSettingString);

  // Sort the Themes for GUI and list them
  int iCurrentTheme = 0;
  for (int i = 0; i < (int) vecTheme.size(); ++i)
  {
    CStdString strTheme = vecTheme[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strTheme.CompareNoCase(strSettingString) == 0 )
      iCurrentTheme = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strTheme, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentTheme);
}

void CGUIWindowSettingsCategory::FillInSkinColors(CSetting *pSetting)
{
  // There is a default theme (just defaults.xml)
  // any other *.xml files are additional color themes on top of this one.
  CBaseSettingControl *setting = GetSetting(pSetting->GetSetting());
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(setting->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skincolors");
  setting->SetDelayed();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standard defaults.xml will be used!

  // Search for colors in the Current skin!
  vector<CStdString> vecColors;

  CStdString strPath;
  URIUtils::AddFileToFolder(g_SkinInfo->Path(),"colors",strPath);

  CFileItemList items;
  CDirectory::GetDirectory(PTH_IC(strPath), items, ".xml");
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder && pItem->GetLabel().CompareNoCase("defaults.xml") != 0)
    { // not the default one
      CStdString strLabel = pItem->GetLabel();
      vecColors.push_back(strLabel.Mid(0, strLabel.size() - 4));
    }
  }
  sort(vecColors.begin(), vecColors.end(), sortstringbyname());

  // Remove the .xml extension from the Themes
  if (URIUtils::GetExtension(strSettingString) == ".xml")
    URIUtils::RemoveExtension(strSettingString);

  int iCurrentColor = 0;
  for (int i = 0; i < (int) vecColors.size(); ++i)
  {
    CStdString strColor = vecColors[i];
    // Is the Current Theme our Used Theme! If yes set the ID!
    if (strColor.CompareNoCase(strSettingString) == 0 )
      iCurrentColor = i + 1; // 1: #of Predefined Theme [Label]
    pControl->AddLabel(strColor, i + 1);
  }
  // Set the Choosen Theme
  pControl->SetValue(iCurrentColor);
}

void CGUIWindowSettingsCategory::FillInStartupWindow(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  const vector<CSkinInfo::CStartupWindow> &startupWindows = g_SkinInfo->GetStartupWindows();

  // TODO: How should we localize this?
  // In the long run there is no way to do it really without the skin having some
  // translation information built in to it, which isn't really feasible.

  // Alternatively we could lookup the strings in the english strings file to get
  // their id and then get the string from that

  // easier would be to have the skinner use the "name" as the label number.

  // eg <window id="0">513</window>

  bool currentSettingFound(false);
  for (vector<CSkinInfo::CStartupWindow>::const_iterator it = startupWindows.begin(); it != startupWindows.end(); it++)
  {
    CStdString windowName((*it).m_name);
    if (StringUtils::IsNaturalNumber(windowName))
      windowName = g_localizeStrings.Get(atoi(windowName.c_str()));
    int windowID((*it).m_id);
    pControl->AddLabel(windowName, windowID);
    if (pSettingInt->GetData() == windowID)
      currentSettingFound = true;
  }

  // ok, now check whether our current option is one of these
  // and set it's value
  if (!currentSettingFound)
  { // nope - set it to the "default" option - the first one
    pSettingInt->SetData(startupWindows[0].m_id);
  }
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::OnInitWindow()
{
  m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
  m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
  SetupControls();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowSettingsCategory::FillInViewModes(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->AddLabel("Auto", DEFAULT_VIEW_AUTO);
  bool found(false);
  int foundType = 0;
  CGUIWindow *window = g_windowManager.GetWindow(windowID);
  if (window)
  {
    window->Initialize();
    for (int i = 50; i < 60; i++)
    {
      CGUIBaseContainer *control = (CGUIBaseContainer *)window->GetControl(i);
      if (control)
      {
        int type = (control->GetType() << 16) | i;
        pControl->AddLabel(control->GetLabel(), type);
        if (type == pSettingInt->GetData())
          found = true;
        else if ((type >> 16) == (pSettingInt->GetData() >> 16))
          foundType = type;
      }
    }
    window->ClearAll();
  }
  if (!found)
    pSettingInt->SetData(foundType ? foundType : (DEFAULT_VIEW_AUTO));
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInSortMethods(CSetting *pSetting, int windowID)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CFileItemList items("C:");
  CGUIViewState *state = CGUIViewState::GetViewState(windowID, items);
  if (state)
  {
    bool found(false);
    vector< pair<int,int> > sortMethods;
    state->GetSortMethods(sortMethods);
    for (unsigned int i = 0; i < sortMethods.size(); i++)
    {
      pControl->AddLabel(g_localizeStrings.Get(sortMethods[i].second), sortMethods[i].first);
      if (sortMethods[i].first == pSettingInt->GetData())
        found = true;
    }
    if (!found && sortMethods.size())
      pSettingInt->SetData(sortMethods[0].first);
  }
  pControl->SetValue(pSettingInt->GetData());
  delete state;
}

void CGUIWindowSettingsCategory::FillInNetworkInterfaces(CSetting *pSetting, float groupWidth, int &iControlID)
{
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)AddSetting(pSetting, groupWidth, iControlID);
  pControl->Clear();

  // query list of interfaces
  vector<CStdString> vecInterfaces;
  std::vector<CNetworkInterface*>& ifaces = g_application.getNetwork().GetInterfaceList();
  std::vector<CNetworkInterface*>::const_iterator iter = ifaces.begin();
  while (iter != ifaces.end())
  {
    CNetworkInterface* iface = *iter;
    vecInterfaces.push_back(iface->GetName());
    ++iter;
  }
  sort(vecInterfaces.begin(), vecInterfaces.end(), sortstringbyname());

  int iInterface = 0;
  for (unsigned int i = 0; i < vecInterfaces.size(); ++i)
    pControl->AddLabel(vecInterfaces[i], iInterface++);
}

void CGUIWindowSettingsCategory::FillInEpgGuideView(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(19029), GUIDE_VIEW_CHANNEL);
  pControl->AddLabel(g_localizeStrings.Get(19030), GUIDE_VIEW_NOW);
  pControl->AddLabel(g_localizeStrings.Get(19031), GUIDE_VIEW_NEXT);
  pControl->AddLabel(g_localizeStrings.Get(19032), GUIDE_VIEW_TIMELINE);

  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInPvrStartLastChannel(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(106),   START_LAST_CHANNEL_OFF);
  pControl->AddLabel(g_localizeStrings.Get(19190), START_LAST_CHANNEL_MIN);
  pControl->AddLabel(g_localizeStrings.Get(107),   START_LAST_CHANNEL_ON);

  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInAudioDevices(CSetting* pSetting, bool Passthrough)
{
#if defined(__APPLE__)
  #if defined(__arm__)
    if (Passthrough)
      return;
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
    pControl->Clear();

    IOSCoreAudioDeviceList deviceList;
    CIOSCoreAudioHardware::GetOutputDevices(&deviceList);

    // This will cause FindAudioDevice to fall back to the system default as configured in 'System Preferences'
    if (CIOSCoreAudioHardware::GetDefaultOutputDevice())
      pControl->AddLabel("Default Output Device", 0);

    int activeDevice = 0;
    CStdString deviceName;
    for (int i = pControl->GetMaximum(); !deviceList.empty(); i++)
    {
      CIOSCoreAudioDevice device(deviceList.front());
      pControl->AddLabel(device.GetName(deviceName), i);

      // Tag this one
      if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(deviceName))
        activeDevice = i; 

      deviceList.pop_front();
    }
    pControl->SetValue(activeDevice);
  #else
    if (Passthrough)
      return;
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
    pControl->Clear();

    CoreAudioDeviceList deviceList;
    CCoreAudioHardware::GetOutputDevices(&deviceList);

    // This will cause FindAudioDevice to fall back to the system default as configured in 'System Preferences'
    if (CCoreAudioHardware::GetDefaultOutputDevice())
      pControl->AddLabel("Default Output Device", 0);

    int activeDevice = 0;
    CStdString deviceName;
    for (int i = pControl->GetMaximum(); !deviceList.empty(); i++)
    {
      CCoreAudioDevice device(deviceList.front());
      pControl->AddLabel(device.GetName(deviceName), i);

      if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(deviceName))
        activeDevice = i; // Tag this one

      deviceList.pop_front();
    }
    pControl->SetValue(activeDevice);
  #endif
#else
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  CStdString currentDevice = Passthrough ? g_guiSettings.GetString("audiooutput.passthroughdevice") : g_guiSettings.GetString("audiooutput.audiodevice");

  if (Passthrough)
  {
    m_DigitalAudioSinkMap.clear();
    m_DigitalAudioSinkMap["Error - no devices found"] = "null:";
    m_DigitalAudioSinkMap[g_localizeStrings.Get(636)] = "custom";
  }
  else
  {
    m_AnalogAudioSinkMap.clear();
    m_AnalogAudioSinkMap["Error - no devices found"] = "null:";
    m_AnalogAudioSinkMap[g_localizeStrings.Get(636)] = "custom";
  }

  int numberSinks = 0;

  int selectedValue = -1;
  AudioSinkList sinkList;
  CAudioRendererFactory::EnumerateAudioSinks(sinkList, Passthrough);
  if (sinkList.size()==0)
  {
    pControl->AddLabel("Error - no devices found", 0);
    numberSinks = 1;
    selectedValue = 0;
  }
  else
  {
    AudioSinkList::const_iterator iter = sinkList.begin();
    for (int i=0; iter != sinkList.end(); iter++)
    {
      CStdString label = (*iter).first;
      CStdString sink  = (*iter).second;
      pControl->AddLabel(label.c_str(), i);

      if (currentDevice.Equals(sink))
        selectedValue = i;

      if (Passthrough)
        m_DigitalAudioSinkMap[label] = sink;
      else
        m_AnalogAudioSinkMap[label] = sink;

      i++;
    }

    numberSinks = sinkList.size();
  }

#ifdef _LINUX
  if (currentDevice.Equals("custom"))
    selectedValue = numberSinks;

  pControl->AddLabel(g_localizeStrings.Get(636), numberSinks++);
#endif

  if (selectedValue < 0)
  {
    CLog::Log(LOGWARNING, "Failed to find previously selected audio sink");
    pControl->AddLabel(currentDevice, numberSinks);
    pControl->SetValue(numberSinks);
  }
  else
    pControl->SetValue(selectedValue);
#endif
}

void CGUIWindowSettingsCategory::NetworkInterfaceChanged(void)
{
  return;

   NetworkAssignment iAssignment;
   CStdString sIPAddress;
   CStdString sNetworkMask;
   CStdString sDefaultGateway;
   CStdString sWirelessNetwork;
   CStdString sWirelessKey;
   EncMode iWirelessEnc;
   bool bIsWireless;
   CStdString ifaceName;

   // Get network information
   CGUISpinControlEx *ifaceControl = (CGUISpinControlEx *)GetControl(GetSetting("network.interface")->GetID());
   ifaceName = ifaceControl->GetLabel();
   CNetworkInterface* iface = g_application.getNetwork().GetInterfaceByName(ifaceName);
   iface->GetSettings(iAssignment, sIPAddress, sNetworkMask, sDefaultGateway, sWirelessNetwork, sWirelessKey, iWirelessEnc);
   bIsWireless = iface->IsWireless();

   CStdString dns;
   std::vector<CStdString> dnss = g_application.getNetwork().GetNameServers();
   if (dnss.size() >= 1)
      dns = dnss[0];

   // Update controls with information
   CGUISpinControlEx* pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.assignment")->GetID());
   if (pControl1) pControl1->SetValue(iAssignment);
   GetSetting("network.dns")->GetSetting()->FromString(dns);
   if (iAssignment == NETWORK_STATIC || iAssignment == NETWORK_DISABLED)
   {
     GetSetting("network.ipaddress")->GetSetting()->FromString(sIPAddress);
     GetSetting("network.subnet")->GetSetting()->FromString(sNetworkMask);
     GetSetting("network.gateway")->GetSetting()->FromString(sDefaultGateway);
   }
   else
   {
     GetSetting("network.ipaddress")->GetSetting()->FromString(iface->GetCurrentIPAddress());
     GetSetting("network.subnet")->GetSetting()->FromString(iface->GetCurrentNetmask());
     GetSetting("network.gateway")->GetSetting()->FromString(iface->GetCurrentDefaultGateway());
   }

   pControl1 = (CGUISpinControlEx *)GetControl(GetSetting("network.enc")->GetID());
   if (pControl1) pControl1->SetValue(iWirelessEnc);

   if (bIsWireless)
   {
      GetSetting("network.essid")->GetSetting()->FromString(sWirelessNetwork);
      GetSetting("network.key")->GetSetting()->FromString(sWirelessKey);
   }
   else
   {
      GetSetting("network.essid")->GetSetting()->FromString("");
      GetSetting("network.key")->GetSetting()->FromString("");
   }
}

void CGUIWindowSettingsCategory::ValidatePortNumber(CBaseSettingControl* pSettingControl, const CStdString& userPort, const CStdString& privPort, bool listening/*=true*/)
{
  CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
  // check that it's a valid port
  int port = atoi(pSetting->GetData().c_str());
#ifdef _LINUX
  if (listening && geteuid() != 0 && (port < 1024 || port > 65535))
  {
    CGUIDialogOK::ShowAndGetInput(257, 850, 852, -1);
    pSetting->SetData(userPort.c_str());
  }
  else
#endif
  if (port <= 0 || port > 65535)
  {
    CGUIDialogOK::ShowAndGetInput(257, 850, 851, -1);
    pSetting->SetData(privPort.c_str());
  }
}
