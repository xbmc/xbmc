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
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "FileSystem/HDDirectory.h"
#include "Util.h"
#include "GUILabelControl.h"
#include "GUICheckMarkControl.h"
#include "utils/Weather.h"
#include "MusicDatabase.h"
#include "ProgramDatabase.h"
#include "ViewDatabase.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/LED.h"
#include "utils/FanController.h"
#include "xbox/XKHDD.h"
#endif
#ifdef _LINUX
#include <dlfcn.h>
#endif
#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif
#include "PlayListPlayer.h"
#include "SkinInfo.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "lib/libscrobbler/scrobbler.h"
#include "GUIPassword.h"
#include "utils/GUIInfoManager.h"
#include "GUIDialogGamepad.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "GUIFontManager.h"
#include "GUIDialogContextMenu.h"
#include "GUIWindowPrograms.h"
#include "MediaManager.h"
#include "xbox/Network.h"
#include "lib/libGoAhead/WebServer.h"
#include "GUIControlGroupList.h"
#include "XBTimeZone.h"

using namespace DIRECTORY;

#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#define CATEGORY_GROUP_ID               3
#define SETTINGS_GROUP_ID               5
#endif

#define CONTROL_GROUP_BUTTONS           0
#define CONTROL_GROUP_SETTINGS          1
#define CONTROL_SETTINGS_LABEL          2
#define CONTROL_BUTTON_AREA             3
#define CONTROL_BUTTON_GAP              4
#define CONTROL_AREA                    5
#define CONTROL_GAP                     6
#define CONTROL_DEFAULT_BUTTON          7
#define CONTROL_DEFAULT_RADIOBUTTON     8
#define CONTROL_DEFAULT_SPIN            9
#define CONTROL_DEFAULT_CATEGORY_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_START_BUTTONS           30
#define CONTROL_START_CONTROL           50

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
    : CGUIWindow(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml")
{
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalButton = NULL;
  m_pOriginalCategoryButton = NULL;
  m_pOriginalImage = NULL;
  // set the correct ID range...
  m_dwIDRange = 8;
  m_iScreen = 0;
  // set the network settings so that we don't reset them unnecessarily
  m_iNetworkAssignment = -1;
  m_strErrorMessage = "";
  m_strOldTrackFormat = "";
  m_strOldTrackFormatRight = "";
  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
  m_returningFromSkinLoad = false;
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory(void)
{}

bool CGUIWindowSettingsCategory::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    g_settings.Save();
    if (m_iWindowBeforeJump!=WINDOW_INVALID)
    {
      JumpToPreviousSection();
      return true;
    }
    m_lastControlID = 0; // don't save the control as we go to a different window each time
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      /*   if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size())
         {
          // change the setting...
          m_iSection = iControl-CONTROL_START_BUTTONS;
          CheckNetworkSettings();
          CreateSettings();
          return true;
         }*/
      for (unsigned int i = 0; i < m_vecSettings.size(); i++)
      {
        if (m_vecSettings[i]->GetID() == iControl)
          OnClick(m_vecSettings[i]);
      }
    }
    break;
  case GUI_MSG_FOCUSED:
    {
      CGUIWindow::OnMessage(message);
      unsigned int focusedControl = GetFocusedControlID();
      if (focusedControl >= CONTROL_START_BUTTONS && focusedControl < CONTROL_START_BUTTONS + m_vecSections.size() &&
          focusedControl - CONTROL_START_BUTTONS != m_iSection)
      {
        if (m_vecSections[focusedControl-CONTROL_START_BUTTONS]->m_strCategory == "masterlock")
        {
          if (!g_passwordManager.IsMasterLockUnlocked(true))
          { // unable to go to this category - focus the previous one
            SET_CONTROL_FOCUS(CONTROL_START_BUTTONS + m_iSection, 0);
            return false;
          }
        }
        m_iSection = focusedControl - CONTROL_START_BUTTONS;
        CheckNetworkSettings();

        CreateSettings();
      }
      return true;
    }
  case GUI_MSG_LOAD_SKIN:
    {
      // Do we need to reload the language file
      if (!m_strNewLanguage.IsEmpty())
      {
        g_guiSettings.SetString("locale.language", m_strNewLanguage);
        g_settings.Save();

        CStdString strLangInfoPath;
        strLangInfoPath.Format("Q:\\language\\%s\\langinfo.xml", m_strNewLanguage.c_str());
        g_langInfo.Load(_P(strLangInfoPath));

        if (g_langInfo.ForceUnicodeFont() && !g_fontManager.IsFontSetUnicode())
        {
          CLog::Log(LOGINFO, "Language needs a ttf font, loading first ttf font available");
          CStdString strFontSet;
          if (g_fontManager.GetFirstFontSetUnicode(strFontSet))
          {
            m_strNewSkinFontSet=strFontSet;
          }
          else
            CLog::Log(LOGERROR, "No ttf font found but needed.", strFontSet.c_str());
        }

        g_charsetConverter.reset();

        CStdString strLanguagePath;
        strLanguagePath.Format("Q:\\language\\%s\\strings.xml", m_strNewLanguage.c_str());
        g_localizeStrings.Load(_P(strLanguagePath));

        // also tell our weather to reload, as this must be localized
        g_weatherManager.ResetTimer();
      }

      // Do we need to reload the skin font set
      if (!m_strNewSkinFontSet.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.font", m_strNewSkinFontSet);
        g_settings.Save();
      }

      // Reload another skin
      if (!m_strNewSkin.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skin", m_strNewSkin);
        g_settings.Save();
      }

      // Reload a skin theme
      if (!m_strNewSkinTheme.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skintheme", m_strNewSkinTheme);
        // also set the default color theme
        CStdString colorTheme(m_strNewSkinTheme);
        CUtil::ReplaceExtension(colorTheme, ".xml", colorTheme);
        if (colorTheme.Equals("Textures.xml"))
          colorTheme = "default.xml";
        g_guiSettings.SetString("lookandfeel.skincolors", colorTheme);
        g_settings.Save();
      }

      // Reload a skin color
      if (!m_strNewSkinColors.IsEmpty())
      {
        g_guiSettings.SetString("lookandfeel.skincolors", m_strNewSkinColors);
        g_settings.Save();
      }

      // Reload a resolution
      if (m_NewResolution != INVALID)
      {
        g_guiSettings.SetInt("videoscreen.resolution", m_NewResolution);
        //set the gui resolution, if newRes is AUTORES newRes will be set to the highest available resolution
        g_graphicsContext.SetVideoResolution(m_NewResolution, TRUE);
        //set our lookandfeelres to the resolution set in graphiccontext
        g_guiSettings.m_LookAndFeelResolution = m_NewResolution;
      }

      if (IsActive())
        m_returningFromSkinLoad = true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      if (message.GetParam1() != WINDOW_INVALID && !m_returningFromSkinLoad)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iSection = 0;
        ResetControlStates();
      }
      m_returningFromSkinLoad = false;
      m_iScreen = (int)message.GetParam2() - (int)m_dwWindowId;
      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      // Hardware based stuff
      // TODO: This should be done in a completely separate screen
      // to give warning to the user that it writes to the EEPROM.
      if ((g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL))
      {
        g_audioConfig.SetAC3Enabled(g_guiSettings.GetBool("audiooutput.ac3passthrough"));
        g_audioConfig.SetDTSEnabled(g_guiSettings.GetBool("audiooutput.dtspassthrough"));
        if (g_audioConfig.NeedsSave())
        { // should we perhaps show a dialog here?
          g_audioConfig.Save();
        }
      }
      switch(g_guiSettings.GetInt("videooutput.aspect"))
      {
      case VIDEO_NORMAL:
        g_videoConfig.SetNormal();
        break;
      case VIDEO_LETTERBOX:
        g_videoConfig.SetLetterbox(true);
        break;
      case VIDEO_WIDESCREEN:
        g_videoConfig.SetWidescreen(true);
        break;
      }
      g_videoConfig.Set480p(g_guiSettings.GetBool("videooutput.hd480p"));
      g_videoConfig.Set720p(g_guiSettings.GetBool("videooutput.hd720p"));
      g_videoConfig.Set1080i(g_guiSettings.GetBool("videooutput.hd1080i"));

      if (g_videoConfig.NeedsSave())
        g_videoConfig.Save();

      if (g_timezone.GetTimeZoneIndex() != g_guiSettings.GetInt("locale.timezone"))
        g_timezone.SetTimeZoneIndex(g_guiSettings.GetInt("locale.timezone"));

      if (g_timezone.GetDST() != g_guiSettings.GetBool("locale.usedst"))
        g_timezone.SetDST(g_guiSettings.GetBool("locale.usedst"));

      CheckNetworkSettings();
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
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalButton->SetVisible(false);
  m_pOriginalCategoryButton->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);
  // setup our control groups...
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
  if (!group || group->GetControlType() != CGUIControl::GUICONTROL_GROUPLIST)
  {
    // get the area to use...
    CGUIControl *area = (CGUIControl *)GetControl(CONTROL_BUTTON_AREA);
    const CGUIControl *gap = GetControl(CONTROL_BUTTON_GAP);
    if (!area || !gap)
      return;
    Remove(CONTROL_BUTTON_AREA);
    group = new CGUIControlGroupList(GetID(), CATEGORY_GROUP_ID, area->GetXPosition(), area->GetYPosition(),
                                     area->GetWidth(), 1080, gap->GetHeight() - m_pOriginalCategoryButton->GetHeight(),
                                     0, VERTICAL, false);
    group->SetNavigation(CATEGORY_GROUP_ID, CATEGORY_GROUP_ID, SETTINGS_GROUP_ID, SETTINGS_GROUP_ID);
    Insert(group, gap);
    area->FreeResources();
    delete area;
  }
#endif
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
    if (m_vecSections[i]->m_dwLabelID == 12360 && g_settings.m_iLastLoadedProfileIndex != 0)
      continue;
    CGUIButtonControl *pButton = new CGUIButtonControl(*m_pOriginalCategoryButton);
    pButton->SetLabel(g_localizeStrings.Get(m_vecSections[i]->m_dwLabelID));
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
  m_dwDefaultFocusControlID = CONTROL_START_BUTTONS;
}

void CGUIWindowSettingsCategory::CreateSettings()
{
  FreeSettingsControls();

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (!group || group->GetControlType() != CGUIControl::GUICONTROL_GROUPLIST)
  {
    CGUIControl *area = (CGUIControl *)GetControl(CONTROL_AREA);
    const CGUIControl *gap = GetControl(CONTROL_GAP);
    Remove(CONTROL_AREA);
    group = new CGUIControlGroupList(GetID(), SETTINGS_GROUP_ID, area->GetXPosition(), area->GetYPosition(),
                                     area->GetWidth(), 1080, gap->GetHeight() - m_pOriginalButton->GetHeight(), 0, VERTICAL, false);
    group->SetNavigation(SETTINGS_GROUP_ID, SETTINGS_GROUP_ID, CATEGORY_GROUP_ID, CATEGORY_GROUP_ID);
    Insert(group, gap);
    area->FreeResources();
    delete area;
  }
#endif
  if (!group)
    return;
  vecSettings settings;
  g_guiSettings.GetSettingsGroup(m_vecSections[m_iSection]->m_strCategory, settings);
  int iControlID = CONTROL_START_CONTROL;
  for (unsigned int i = 0; i < settings.size(); i++)
  {
    CSetting *pSetting = settings[i];
    AddSetting(pSetting, group->GetWidth(), iControlID);
    CStdString strSetting = pSetting->GetSetting();
    if (strSetting.Equals("myprograms.ntscmode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        pControl->AddLabel(g_localizeStrings.Get(16106 + i), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("mymusic.visualisation"))
    {
      FillInVisualisations(pSetting, GetSetting(pSetting->GetSetting())->GetID());
    }
    else if (strSetting.Equals("karaoke.port0voicemask"))
    {
      FillInVoiceMasks(0, pSetting);
    }
    else if (strSetting.Equals("karaoke.port1voicemask"))
    {
      FillInVoiceMasks(1, pSetting);
    }
    else if (strSetting.Equals("karaoke.port2voicemask"))
    {
      FillInVoiceMasks(2, pSetting);
    }
    else if (strSetting.Equals("karaoke.port3voicemask"))
    {
      FillInVoiceMasks(3, pSetting);
    }
    else if (strSetting.Equals("audiooutput.mode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(338), AUDIO_ANALOG);
      if (g_audioConfig.HasDigitalOutput())
        pControl->AddLabel(g_localizeStrings.Get(339), AUDIO_DIGITAL);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videooutput.aspect"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21375), VIDEO_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(21376), VIDEO_LETTERBOX);
      pControl->AddLabel(g_localizeStrings.Get(21377), VIDEO_WIDESCREEN);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("cddaripper.encoder"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("Lame", CDDARIP_ENCODER_LAME);
      pControl->AddLabel("Vorbis", CDDARIP_ENCODER_VORBIS);
      pControl->AddLabel("Wav", CDDARIP_ENCODER_WAV);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("cddaripper.quality"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(604), CDDARIP_QUALITY_CBR);
      pControl->AddLabel(g_localizeStrings.Get(601), CDDARIP_QUALITY_MEDIUM);
      pControl->AddLabel(g_localizeStrings.Get(602), CDDARIP_QUALITY_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(603), CDDARIP_QUALITY_EXTREME);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lcd.type"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), LCD_TYPE_NONE);
      pControl->AddLabel("LCD - HD44780", LCD_TYPE_LCD_HD44780);
      pControl->AddLabel("LCD - KS0073", LCD_TYPE_LCD_KS0073);
      pControl->AddLabel("VFD", LCD_TYPE_VFD);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lcd.modchip"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("SmartXX", MODCHIP_SMARTXX);
      pControl->AddLabel("Xenium", MODCHIP_XENIUM);
      pControl->AddLabel("Xecuter3", MODCHIP_XECUTER3);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("harddisk.aamlevel"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21388), AAM_QUIET);
      pControl->AddLabel(g_localizeStrings.Get(21387), AAM_FAST);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("harddisk.apmlevel"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(21391), APM_HIPOWER);
      pControl->AddLabel(g_localizeStrings.Get(21392), APM_LOPOWER);
      pControl->AddLabel(g_localizeStrings.Get(21393), APM_HIPOWER_STANDBY);
      pControl->AddLabel(g_localizeStrings.Get(21394), APM_LOPOWER_STANDBY);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.targettemperature"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        CTemperature temp=CTemperature::CreateFromCelsius(i);
        pControl->AddLabel(temp.ToString(), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.fanspeed"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      CStdString strPercentMask = g_localizeStrings.Get(14047);
      for (int i = 5; i <= pSettingInt->m_iMax; i += 5)
      {
        CStdString strLabel;
        strLabel.Format(strPercentMask.c_str(), i*2);
        pControl->AddLabel(strLabel, i);
      }
      pControl->SetValue(int(pSettingInt->GetData()));
    }
    else if (strSetting.Equals("harddisk.remoteplayspindown"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(474), SPIN_DOWN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(475), SPIN_DOWN_MUSIC);
      pControl->AddLabel(g_localizeStrings.Get(13002), SPIN_DOWN_VIDEO);
      pControl->AddLabel(g_localizeStrings.Get(476), SPIN_DOWN_BOTH);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("servers.webserverpassword"))
    { 
#ifdef HAS_WEB_SERVER
      // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetPassword());
        g_settings.Save();
      }
#endif
    }
    else if (strSetting.Equals("network.assignment"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(718), NETWORK_DASH);
      pControl->AddLabel(g_localizeStrings.Get(716), NETWORK_DHCP);
      pControl->AddLabel(g_localizeStrings.Get(717), NETWORK_STATIC);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.style"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(738), FONT_STYLE_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(739), FONT_STYLE_BOLD);
      pControl->AddLabel(g_localizeStrings.Get(740), FONT_STYLE_ITALICS);
      pControl->AddLabel(g_localizeStrings.Get(741), FONT_STYLE_BOLD_ITALICS);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.color"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = SUBTITLE_COLOR_START; i <= SUBTITLE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(760 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("subtitles.height"))
    {
      FillInSubtitleHeights(pSetting);
    }
    else if (strSetting.Equals("subtitles.font"))
    {
      FillInSubtitleFonts(pSetting);
    }
    else if (strSetting.Equals("subtitles.charset") || strSetting.Equals("locale.charset"))
    {
      FillInCharSets(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.font"))
    {
      FillInSkinFonts(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skin"))
    {
      FillInSkins(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.soundskin"))
    {
      FillInSoundSkins(pSetting);
    }
    else if (strSetting.Equals("locale.language"))
    {
      FillInLanguages(pSetting);
    }
    else if (strSetting.Equals("locale.timezone"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i=0; i < g_timezone.GetNumberOfTimeZones(); i++)
        pControl->AddLabel(g_timezone.GetTimeZoneString(i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoscreen.resolution"))
    {
      FillInResolutions(pSetting, false);
    }
    else if (strSetting.Equals("videoscreen.vsync"))
    {
      FillInVSyncs(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skintheme"))
    {
      FillInSkinThemes(pSetting);
    }
    else if (strSetting.Equals("lookandfeel.skincolors"))
    {
      FillInSkinColors(pSetting);
    }
    else if (strSetting.Equals("screensaver.mode"))
    {
      FillInScreenSavers(pSetting);
    }
    else if (strSetting.Equals("videoplayer.displayresolution") || strSetting.Equals("pictures.displayresolution"))
    {
      FillInResolutions(pSetting, true);
    }
    else if (strSetting.Equals("videoplayer.framerateconversions"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(231), FRAME_RATE_LEAVE_AS_IS); // "None"
      pControl->AddLabel(g_videoConfig.HasPAL() ? g_localizeStrings.Get(12380) : g_localizeStrings.Get(12381), FRAME_RATE_CONVERT); // "Play PAL videos at NTSC rates" or "Play NTSC videos at PAL rates"
      if (g_videoConfig.HasPAL60())
        pControl->AddLabel(g_localizeStrings.Get(12382), FRAME_RATE_USE_PAL60); // "Play NTSC videos in PAL60"
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.ledcolour"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13340), LED_COLOUR_NO_CHANGE);
      pControl->AddLabel(g_localizeStrings.Get(13341), LED_COLOUR_GREEN);
      pControl->AddLabel(g_localizeStrings.Get(13342), LED_COLOUR_ORANGE);
      pControl->AddLabel(g_localizeStrings.Get(13343), LED_COLOUR_RED);
      pControl->AddLabel(g_localizeStrings.Get(13344), LED_COLOUR_CYCLE);
      pControl->AddLabel(g_localizeStrings.Get(351), LED_COLOUR_OFF);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("system.leddisableonplayback") || strSetting.Equals("lcd.disableonplayback"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), LED_PLAYBACK_OFF);     // No
      pControl->AddLabel(g_localizeStrings.Get(13002), LED_PLAYBACK_VIDEO);   // Video Only
      pControl->AddLabel(g_localizeStrings.Get(475), LED_PLAYBACK_MUSIC);    // Music Only
      pControl->AddLabel(g_localizeStrings.Get(476), LED_PLAYBACK_VIDEO_MUSIC); // Video & Music
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("videoplayer.rendermethod"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13355), RENDER_LQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(13356), RENDER_OVERLAYS);
      pControl->AddLabel(g_localizeStrings.Get(13357), RENDER_HQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(21397), RENDER_HQ_RGB_SHADERV2);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("musicplayer.replaygaintype"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), REPLAY_GAIN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(639), REPLAY_GAIN_TRACK);
      pControl->AddLabel(g_localizeStrings.Get(640), REPLAY_GAIN_ALBUM);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("lookandfeel.startupwindow"))
    {
      FillInStartupWindow(pSetting);
    }
    else if (strSetting.Equals("servers.ftpserveruser"))
    {
      FillInFTPServerUser(pSetting);
    }
    else if (strSetting.Equals("autodetect.nickname"))
    {
#ifdef HAS_XBOX_HARDWARE
      CStdString strXboxNickNameOut;
      if (CUtil::GetXBOXNickName(strXboxNickNameOut))
        g_guiSettings.SetString("autodetect.nickname", strXboxNickNameOut.c_str());
#endif
    }
    else if (strSetting.Equals("videoplayer.externaldvdplayer"))
    {
      CSettingString *pSettingString = (CSettingString *)pSetting;
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      if (pSettingString->GetData().IsEmpty())
        pControl->SetLabel2(g_localizeStrings.Get(20009));
    }
    else if (strSetting.Equals("locale.country"))
    {
      FillInRegions(pSetting);
    }
    else if (strSetting.Equals("musicfiles.viewmode"))
    {
      FillInViewModes(pSetting, WINDOW_MUSIC_FILES);
    }
    else if (strSetting.Equals("myvideos.viewmode"))
    {
      FillInViewModes(pSetting, WINDOW_VIDEO_FILES);
    }
    else if (strSetting.Equals("programfiles.viewmode"))
    {
      FillInViewModes(pSetting, WINDOW_PROGRAMS);
    }
    else if (strSetting.Equals("pictures.viewmode"))
    {
      FillInViewModes(pSetting, WINDOW_PICTURES);
    }
    else if (strSetting.Equals("musicfiles.sortmethod"))
    {
      FillInSortMethods(pSetting, WINDOW_MUSIC_FILES);
    }
    else if (strSetting.Equals("myvideos.sortmethod"))
    {
      FillInSortMethods(pSetting, WINDOW_VIDEO_FILES);
    }
    else if (strSetting.Equals("programfiles.sortmethod"))
    {
      FillInSortMethods(pSetting, WINDOW_PROGRAMS);
    }
    else if (strSetting.Equals("pictures.sortmethod"))
    {
      FillInSortMethods(pSetting, WINDOW_PICTURES);
    }
    else if (strSetting.Equals("musicfiles.sortorder") ||
             strSetting.Equals("myvideos.sortorder") ||
             strSetting.Equals("pictures.sortorder") ||
             strSetting.Equals("programfiles.sortorder"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(584), SORT_ORDER_ASC);
      pControl->AddLabel(g_localizeStrings.Get(585), SORT_ORDER_DESC);
      pControl->SetValue(pSettingInt->GetData());
    }
  }
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
    if (strSetting.Equals("filelists.allowfiledeletion"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("filelists.disableaddsourcebuttons"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources() || g_passwordManager.bMasterUser);
    }
    else if (strSetting.Equals("myprograms.ntscmode"))
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("myprograms.gameautoregion"));
    }
    else if (strSetting.Equals("xlinkkai.enablenotifications") || strSetting.Equals("xlinkkai.username") || strSetting.Equals("xlinkkai.password") || strSetting.Equals("xlinkkai.server"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("xlinkkai.enabled"));
    }
    else if (strSetting.Equals("masterlock.startuplock") || strSetting.Equals("masterlock.enableshutdown") || strSetting.Equals("masterlock.automastermode"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE);
    }
    else if (strSetting.Equals("masterlock.loginlock"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.bUseLoginScreen);
    }
    else if (strSetting.Equals("screensaver.uselock"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE && !g_guiSettings.GetString("screensaver.mode").Equals("Black"));
    }
    else if (strSetting.Equals("upnp.musicshares") || strSetting.Equals("upnp.videoshares") || strSetting.Equals("upnp.pictureshares"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("upnp.server"));
    }
    else if (strSetting.Equals("mymusic.clearplaylistsonend"))
    { // disable repeat and repeat one if clear playlists is enabled
      if (g_guiSettings.GetBool("mymusic.clearplaylistsonend"))
      {
        g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC, PLAYLIST::REPEAT_NONE);
        g_stSettings.m_bMyMusicPlaylistRepeat = false;
        g_settings.Save();
      }
    }
    else if (strSetting.Equals("cddaripper.quality"))
    { // only visible if we are doing non-WAV ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("cddaripper.encoder") != CDDARIP_ENCODER_WAV);
    }
    else if (strSetting.Equals("cddaripper.bitrate"))
    { // only visible if we are ripping to CBR
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled((g_guiSettings.GetInt("cddaripper.encoder") != CDDARIP_ENCODER_WAV) &&
                                           (g_guiSettings.GetInt("cddaripper.quality") == CDDARIP_QUALITY_CBR));
    }
    else if (strSetting.Equals("musicplayer.outputtoallspeakers") || strSetting.Equals("audiooutput.ac3passthrough") || strSetting.Equals("audiooutput.dtspassthrough"))
    { // only visible if we are in digital mode
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("audiooutput.mode") == AUDIO_DIGITAL);
    }
    else if (strSetting.Equals("videooutput.hd480p") || strSetting.Equals("videooutput.hd720p") || strSetting.Equals("videooutput.hd1080i"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      // disable if we do not have the HDTV pack and are not NTSC
#ifdef HAS_XBOX_HARDWARE
      if (pControl) pControl->SetEnabled(g_videoConfig.HasNTSC() && g_videoConfig.HasHDPack());
#endif
    }
    else if (strSetting.Equals("musicplayer.crossfadealbumtracks"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("musicplayer.crossfade") > 0);
    }
    else if (strSetting.Left(12).Equals("karaoke.port") || strSetting.Equals("karaoke.volume"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("karaoke.voiceenabled"));
    }
    else if (strSetting.Equals("system.fanspeed"))
    { // only visible if we have fancontrolspeed enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("system.fanspeedcontrol"));
    }
    else if (strSetting.Equals("system.targettemperature"))
    { // only visible if we have autotemperature enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("system.autotemperature"));
    }
    else if (strSetting.Equals("harddisk.remoteplayspindowndelay"))
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("harddisk.remoteplayspindown") != SPIN_DOWN_NONE);
    }
    else if (strSetting.Equals("harddisk.remoteplayspindownminduration"))
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("harddisk.remoteplayspindown") != SPIN_DOWN_NONE);
    }
    else if (strSetting.Equals("servers.ftpserveruser") || strSetting.Equals("servers.ftpserverpassword") || strSetting.Equals("servers.ftpautofatx"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("servers.ftpserver"));
    }
    else if (strSetting.Equals("servers.webserverpassword"))
    { // Fill in a blank pass if we don't have it
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (((CSettingString *)pSettingControl->GetSetting())->GetData().size() == 0 && pControl)
      {
        pControl->SetLabel2(g_localizeStrings.Get(734));
        pControl->SetEnabled(g_guiSettings.GetBool("servers.webserver"));
      }
    }
    else if (strSetting.Equals("servers.webserverport"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("servers.webserver"));
    }
    else if (strSetting.Equals("network.ipaddress") || strSetting.Equals("network.subnet") || strSetting.Equals("network.gateway") || strSetting.Equals("network.dns"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if (g_guiSettings.GetInt("network.assignment") != NETWORK_STATIC)
        {
          //We are in non Static Mode! Setting the Received IP Information
          if(strSetting.Equals("network.ipaddress"))
            pControl->SetLabel2(g_network.m_networkinfo.ip);
          else if(strSetting.Equals("network.subnet"))
            pControl->SetLabel2(g_network.m_networkinfo.subnet);
          else if(strSetting.Equals("network.gateway"))
            pControl->SetLabel2(g_network.m_networkinfo.gateway);
          else if(strSetting.Equals("network.dns"))
            pControl->SetLabel2(g_network.m_networkinfo.DNS1);
        }
        pControl->SetEnabled(g_guiSettings.GetInt("network.assignment") == NETWORK_STATIC);
      }
    }
    else if (strSetting.Equals("Network.httpproxyserver") || strSetting.Equals("Network.httpproxyport"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("network.usehttpproxy"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.verticaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.horizontaldeblocklevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.horizontaldeblocking") &&
                           g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.verticaldeblocking") || strSetting.Equals("postprocessing.horizontaldeblocking") || strSetting.Equals("postprocessing.autobrightnesscontrastlevels") || strSetting.Equals("postprocessing.dering"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable") &&
                           !g_guiSettings.GetBool("postprocessing.auto"));
    }
    else if (strSetting.Equals("postprocessing.auto"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("postprocessing.enable"));
    }
    else if (strSetting.Equals("VideoPlayer.InvertFieldSync"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("VideoPlayer.FieldSync"));
    }
    else if (strSetting.Equals("subtitles.color") || strSetting.Equals("subtitles.style") || strSetting.Equals("subtitles.charset"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(CUtil::IsUsingTTFSubtitles());
    }
    else if (strSetting.Equals("subtitles.flipbidicharset"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      CStdString strCharset=g_langInfo.GetSubtitleCharSet();
      pControl->SetEnabled( /*CUtil::IsUsingTTFSubtitles() &&*/ g_charsetConverter.isBidiCharset(strCharset));
    }
    else if (strSetting.Equals("locale.charset"))
    { // TODO: Determine whether we are using a TTF font or not.
      //   CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      //   if (pControl) pControl->SetEnabled(g_guiSettings.GetString("lookandfeel.font").Right(4) == ".ttf");
    }
    else if (strSetting.Equals("screensaver.dimlevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "Dim");
    }
    else if (strSetting.Equals("screensaver.slideshowpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow");
    }
    else if (strSetting.Equals("screensaver.slideshowshuffle"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") == "SlideShow");
    }
    else if (strSetting.Equals("screensaver.preview"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("screensaver.mode") != "None");
    }
    else if (strSetting.Left(16).Equals("weather.areacode"))
    {
      CSettingString *pSetting = (CSettingString *)GetSetting(strSetting)->GetSetting();
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetLabel2(g_weatherManager.GetAreaCity(pSetting->GetData()));
    }
    else if (strSetting.Equals("system.leddisableonplayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      // LED_COLOUR_NO_CHANGE: we can't disable the LED on playback, 
      //                       we have no previos reference LED COLOUR, to set the LED colour back
      pControl->SetEnabled(g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_NO_CHANGE && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_OFF);
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
    else if (strSetting.Equals("locale.timeaddress"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("locale.timeserver"));
    }
    else if (strSetting.Equals("locale.time") || strSetting.Equals("locale.date"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("locale.timeserver"));
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("locale.time"))
        time = g_infoManager.GetTime();
      else
        time = g_infoManager.GetDate();
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
    else if (strSetting.Equals("autodetect.nickname") || strSetting.Equals("autodetect.senduserpw"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("autodetect.onoff") && (g_settings.m_iLastLoadedProfileIndex == 0));
    }
    else if ( strSetting.Equals("autodetect.popupinfo"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("autodetect.onoff"));
    }
    else if (strSetting.Equals("videoplayer.externaldvdplayer"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("videoplayer.useexternaldvdplayer"));
    }
#ifdef HAS_TRAINER
    else if (strSetting.Equals("myprograms.trainerpath") || strSetting.Equals("cddaripper.path") || strSetting.Equals("mymusic.recordingpath") || strSetting.Equals("pictures.screenshotpath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl && g_guiSettings.GetString(strSetting, false).IsEmpty())
        pControl->SetLabel2("");
    }
#endif
    else if (strSetting.Equals("myprograms.dashboard"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("myprograms.usedashpath"));
    }
    else if (strSetting.Equals("lcd.enableonpaused"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.disableonplayback") != LED_PLAYBACK_OFF && g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE);
    }
    else if (strSetting.Equals("system.ledenableonpaused"))
    {
      // LED_COLOUR_NO_CHANGE: we can't enable LED on paused, 
      //                       we have no previos reference LED COLOUR, to set the LED colour back
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("system.leddisableonplayback") != LED_PLAYBACK_OFF && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_OFF && g_guiSettings.GetInt("system.ledcolour") != LED_COLOUR_NO_CHANGE);
    }
    else if (strSetting.Equals("lcd.modchip") || strSetting.Equals("lcd.backlight") || strSetting.Equals("lcd.disableonplayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE);
    }
    else if (strSetting.Equals("lcd.contrast"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      // X3 can't controll the Contrast via software graying out!
      if(g_guiSettings.GetInt("lcd.type") != LCD_TYPE_NONE)
      {
        if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("lcd.modchip") != MODCHIP_XECUTER3);
      }
      else 
      { 
        if (pControl) pControl->SetEnabled(false); 
      }
    }
    else if (strSetting.Equals("lookandfeel.soundsduringplayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetString("lookandfeel.soundskin") != "OFF");
    }
  }
}

void CGUIWindowSettingsCategory::UpdateRealTimeSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
    if (strSetting.Equals("locale.time") || strSetting.Equals("locale.date"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("locale.timeserver"));
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("locale.time"))
        time = g_infoManager.GetTime();
      else
        time = g_infoManager.GetDate();
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
  }
}

void CGUIWindowSettingsCategory::OnClick(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
  if (strSetting.Left(16).Equals("weather.areacode"))
  {
    CStdString strSearch;
    if (CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(14024), false))
    {
      strSearch.Replace(" ", "+");
      CStdString strResult = ((CSettingString *)pSettingControl->GetSetting())->GetData();
      if (g_weatherManager.GetSearchResults(strSearch, strResult))
        ((CSettingString *)pSettingControl->GetSetting())->SetData(strResult);
      g_weatherManager.ResetTimer();
    }
  }

  // if OnClick() returns false, the setting hasn't changed
  if (!pSettingControl->OnClick()) // call the control to do it's thing
    return;

  // ok, now check the various special things we need to do
  if (strSetting.Equals("mymusic.visualisation"))
  { // new visualisation choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue() == 0)
      pSettingString->SetData("None");
    else
      pSettingString->SetData(pControl->GetCurrentLabel() + ".vis");
  }
  /*else if (strSetting.Equals("musicfiles.repeat"))
  {
    g_playlistPlayer.SetRepeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("musicfiles.repeat") ? PLAYLIST::REPEAT_ALL : PLAYLIST::REPEAT_NONE);
  }*/
  else if (strSetting.Equals("karaoke.port0voicemask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port0voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(0, g_guiSettings.GetSetting("karaoke.port0voicemask"));
  }
  else if (strSetting.Equals("karaoke.port1voicemask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port1voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(1, g_guiSettings.GetSetting("karaoke.port1voicemask"));
  }
  else if (strSetting.Equals("karaoke.port2voicemask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port2voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(2, g_guiSettings.GetSetting("karaoke.port2voicemask"));
  }
  else if (strSetting.Equals("karaoke.port2voicemask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("karaoke.port3voicemask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(3, g_guiSettings.GetSetting("karaoke.port3voicemask"));
  }
  else if (strSetting.Equals("mymusic.cleanupmusiclibrary"))
  {
    CMusicDatabase musicdatabase;
    musicdatabase.Clean();
    CUtil::DeleteMusicDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("myvideos.cleanupvideolibrary"))
  {
    if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.CleanDatabase();
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("myvideos.exportvideolibrary"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(651), path, true))
    {
      CUtil::AddFileToFolder(path, "videodb.xml", path);
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ExportToXML(path);
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("myvideos.importvideolibrary"))
  {
    CStdString path(g_settings.GetDatabaseFolder());
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "videodb.xml", g_localizeStrings.Get(651) , path))
    {
      CVideoDatabase videodatabase;
      videodatabase.Open();
      videodatabase.ImportFromXML(path);
      videodatabase.Close();
    }
  }
  else if (strSetting.Equals("musicplayer.jumptoaudiohardware") || strSetting.Equals("videoplayer.jumptoaudiohardware"))
  {
    JumpToSection(WINDOW_SETTINGS_SYSTEM, "audiooutput");
  }
  else if (strSetting.Equals("musicplayer.jumptocache") || strSetting.Equals("videoplayer.jumptocache"))
  {
    JumpToSection(WINDOW_SETTINGS_SYSTEM, "cache");
  }
  else if (strSetting.Equals("weather.jumptolocale"))
  {
    JumpToSection(WINDOW_SETTINGS_APPEARANCE, "locale");
  }
  else if (strSetting.Equals("lastfm.enable") || strSetting.Equals("lastfm.username") || strSetting.Equals("lastfm.password"))
  {
    if (g_guiSettings.GetBool("lastfm.enable"))
    {
      CStdString strPassword=g_guiSettings.GetString("lastfm.password");
      CStdString strUserName=g_guiSettings.GetString("lastfm.username");
      if (!strUserName.IsEmpty() || !strPassword.IsEmpty())
        CScrobbler::GetInstance()->Init();
    }
    else
    {
      CScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting.Equals("musicplayer.outputtoallspeakers"))
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();

    if (!g_application.IsPlaying())
    {
      g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("musicplayer.replaygaintype");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("musicplayer.replaygainpreamp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("musicplayer.replaygainnogainpreamp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("musicplayer.replaygainavoidclipping");
  }
#ifdef HAS_KAI
  else if (strSetting.Equals("xlinkkai.enabled"))
  {
    if (g_guiSettings.GetBool("xlinkkai.enabled"))
      g_application.StartKai();
    else
      g_application.StopKai();
  }
#endif
#ifdef HAS_LCD
  else if (strSetting.Equals("lcd.type"))
  {
    g_lcd->Initialize();
  }
  else if (strSetting.Equals("lcd.backlight"))
  {
    g_lcd->SetBackLight(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Equals("lcd.modchip"))
  {
    g_lcd->Stop();
    CLCDFactory factory;
    delete g_lcd;
    g_lcd = factory.Create();
    g_lcd->Initialize();
  }
  else if (strSetting.Equals("lcd.contrast"))
  {
    g_lcd->SetContrast(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
#endif
#ifdef HAS_XBOX_HARDWARE
  else if (strSetting.Equals("system.targettemperature"))
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    CFanController::Instance()->SetTargetTemperature(pSetting->GetData());
  }
  else if (strSetting.Equals("system.fanspeed"))
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    int iSpeed = (RESOLUTION)msg.GetParam1();
    g_guiSettings.SetInt("system.fanspeed", iSpeed);
    CFanController::Instance()->SetFanSpeed(iSpeed);
  }
  else if (strSetting.Equals("system.autotemperature"))
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
    if (pSetting->GetData())
    {
      g_guiSettings.SetBool("system.fanspeedcontrol", false);
      CFanController::Instance()->Start(g_guiSettings.GetInt("system.targettemperature"));
    }
    else
      CFanController::Instance()->Stop();
  }
  else if (strSetting.Equals("system.fanspeedcontrol"))
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
    if (pSetting->GetData())
    {
      g_guiSettings.SetBool("system.autotemperature", false);
      CFanController::Instance()->Stop();
      CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("system.fanspeed"));
    }
    else
      CFanController::Instance()->RestoreStartupSpeed();
  }
  else if (strSetting.Equals("harddisk.aamlevel"))
  {
    CSettingInt * pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int setting_level = pSetting->GetData();

    if (setting_level == AAM_QUIET)
      XKHDD::SetAAMLevel(0x80);
    else if (setting_level == AAM_FAST)
      XKHDD::SetAAMLevel(0xFE);
  }
  else if (strSetting.Equals("harddisk.apmlevel"))
  {
    CSettingInt * pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int setting_level = pSetting->GetData();

    switch(setting_level)
    {
    case APM_LOPOWER:
      XKHDD::SetAPMLevel(0x80);
      break;
    case APM_HIPOWER:
      XKHDD::SetAPMLevel(0xFE);
      break;
    case APM_LOPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x01);
      break;
    case APM_HIPOWER_STANDBY:
      XKHDD::SetAPMLevel(0x7F);
      break;
    }
  }
  else if (strSetting.Equals("autodetect.nickname") )
  {
    CStdString strXboxNickNameIn = g_guiSettings.GetString("autodetect.nickname");
    CUtil::SetXBOXNickName(strXboxNickNameIn, strXboxNickNameIn);
  }
#endif
  else if (strSetting.Equals("servers.ftpserver"))
  {
    g_application.StopFtpServer();
    if (g_guiSettings.GetBool("servers.ftpserver"))
      g_application.StartFtpServer();

  }
  else if (strSetting.Equals("servers.ftpserverpassword"))
  {
   SetFTPServerUserPass();
  }
  else if (strSetting.Equals("servers.ftpserveruser"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("servers.ftpserveruser", pControl->GetCurrentLabel());
  }

  else if (strSetting.Equals("servers.webserver") || strSetting.Equals("servers.webserverport") || strSetting.Equals("servers.webserverpassword"))
  {
    if (strSetting.Equals("servers.webserverport"))
    {
      CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
      // check that it's a valid port
      int port = atoi(pSetting->GetData().c_str());
      if (port <= 0 || port > 65535)
        pSetting->SetData("80");
    }
#ifdef HAS_WEB_SERVER
    g_application.StopWebServer();
    if (g_guiSettings.GetBool("servers.webserver"))
    {
      g_application.StartWebServer();
      g_application.m_pWebServer->SetPassword(g_guiSettings.GetString("servers.webserverpassword").c_str());
    }
#endif
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
  else if (strSetting.Equals("Network.httpproxyport"))
  {
    CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
    // check that it's a valid port
    int port = atoi(pSetting->GetData().c_str());
    if (port <= 0 || port > 65535)
      pSetting->SetData("8080");
  }
  else if (strSetting.Equals("videoplayer.calibrate") || strSetting.Equals("videoscreen.guicalibration"))
  { // activate the video calibration screen
    m_gWindowManager.ActivateWindow(WINDOW_SCREEN_CALIBRATION);
  }
  else if (strSetting.Equals("videoplayer.externaldvdplayer"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    // TODO 2.0: Localize this
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".xbe", g_localizeStrings.Get(655), path))
      pSettingString->SetData(path);
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
    FillInSubtitleHeights(g_guiSettings.GetSetting("subtitles.height"));
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
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkinFontSet = pControl->GetCurrentLabel();
    if (strSkinFontSet != ".svn" && strSkinFontSet != g_guiSettings.GetString("lookandfeel.font"))
    {
      m_strNewSkinFontSet = strSkinFontSet;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the language we are already using
      m_strNewSkinFontSet.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skin"))
  { // new skin choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkin = pControl->GetCurrentLabel();
    CStdString strSkinPath = _P("Q:\\skin\\" + strSkin);
    if (g_SkinInfo.Check(strSkinPath))
    {
      m_strErrorMessage.Empty();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().textColor);
      if (strSkin != ".svn" && strSkin != g_guiSettings.GetString("lookandfeel.skin"))
      {
        m_strNewSkin = strSkin;
        g_application.DelayLoadSkin();
      }
      else
      { // Do not reload the skin we are already using
        m_strNewSkin.Empty();
        g_application.CancelDelayLoadSkin();
      }
    }
    else
    {
      m_strErrorMessage.Format("Incompatible skin. We require skins of version %0.2f or higher", g_SkinInfo.GetMinVersion());
      m_strNewSkin.Empty();
      g_application.CancelDelayLoadSkin();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().disabledColor);
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

    g_audioManager.Load();
  }
  else if (strSetting.Equals("lookandfeel.soundsduringplayback"))
  {
    if (g_guiSettings.GetBool("lookandfeel.soundsduringplayback"))
      g_audioManager.Enable(true);
    else
      g_audioManager.Enable(!g_application.IsPlaying() || g_application.IsPaused());
  }
  else if (strSetting.Equals("videoscreen.resolution"))
  { // new resolution choosen... - update if necessary
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    m_NewResolution = (RESOLUTION)msg.GetParam1();
    // reset our skin if necessary
    // delay change of resolution
    if (m_NewResolution != g_guiSettings.m_LookAndFeelResolution)
    {
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the resolution we are using
      m_NewResolution = INVALID;
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("videoscreen.vsync"))
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    g_videoConfig.SetVSyncMode((VSYNC)msg.GetParam1());
  }
  else if (strSetting.Equals("system.ledcolour"))
  { 
#ifdef HAS_XBOX_HARDWARE
    // Alter LED Colour immediately
    int iData =  ((CSettingInt *)pSettingControl->GetSetting())->GetData();
    if (iData == LED_COLOUR_NO_CHANGE)
      // LED_COLOUR_NO_CHANGE: to prevent "led off" on colour immediately change, set to default green! 
      //                       (we have no previos reference LED COLOUR, to set the LED colour back)
      //                       on next boot the colour will not changed and the default BIOS led colour will used
      ILED::CLEDControl(LED_COLOUR_GREEN); 
    else
      ILED::CLEDControl(iData);
#endif
  }
  else if (strSetting.Equals("locale.language"))
  { // new language chosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strLanguage = pControl->GetCurrentLabel();
    if (strLanguage != ".svn" && strLanguage != pSettingString->GetData())
    {
      m_strNewLanguage = strLanguage;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the language we are already using
      m_strNewLanguage.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("lookandfeel.skintheme"))
  { //a new Theme was chosen
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    CStdString strSkinTheme;

    if (pControl->GetValue() == 0) // Use default theme
      strSkinTheme = "SKINDEFAULT";
    else
      strSkinTheme = pControl->GetCurrentLabel() + ".xpr";

    if (strSkinTheme != pSettingString->GetData())
    {
      m_strNewSkinTheme = strSkinTheme;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin theme we are using
      m_strNewSkinTheme.Empty();
      g_application.CancelDelayLoadSkin();
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
      m_strNewSkinColors = strSkinColor;
      g_application.DelayLoadSkin();
    }
    else
    { // Do not reload the skin colors we are using
      m_strNewSkinColors.Empty();
      g_application.CancelDelayLoadSkin();
    }
  }
  else if (strSetting.Equals("videoplayer.displayresolution"))
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    pSettingInt->SetData(msg.GetParam1());
  }
  else if (strSetting.Equals("videoscreen.flickerfilter") || strSetting.Equals("videoscreen.soften"))
  { // reset display
    g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);
  }
  else if (strSetting.Equals("screensaver.mode"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    int iValue = pControl->GetValue();
    CStdString strScreenSaver;
    if (iValue == 0)
      strScreenSaver = "None";
    else if (iValue == 1)
      strScreenSaver = "Dim";
    else if (iValue == 2)
      strScreenSaver = "Black";
    else if (iValue == 3)
      strScreenSaver = "SlideShow"; // PictureSlideShow
    else
      strScreenSaver = pControl->GetCurrentLabel() + ".xbs";
    pSettingString->SetData(strScreenSaver);
  }
  else if (strSetting.Equals("screensaver.preview"))
  {
    g_application.ActivateScreenSaver(true);
  }
  else if (strSetting.Equals("screensaver.slideshowpath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyPictureShares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("myprograms.dashboard"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".xbe", g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("myprograms.trainerpath") || strSetting.Equals("pictures.screenshotpath") || strSetting.Equals("mymusic.recordingpath") || strSetting.Equals("cddaripper.path") || strSetting.Equals("subtitles.custompath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = g_guiSettings.GetString(strSetting,false);
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    UpdateSettings();
    bool bWriteOnly = true;
#ifdef HAS_TRAINER
    if (strSetting.Equals("myprograms.trainerpath"))
      bWriteOnly = false;
#endif

    if (strSetting.Equals("subtitles.custompath"))
    {
      bWriteOnly = false;
      shares = g_settings.m_vecMyVideoShares;
    }
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path, bWriteOnly))
    {
      pSettingString->SetData(path);

#ifdef HAS_TRAINER
      if (strSetting.Equals("myprograms.trainerpath"))
      {
        if (CGUIDialogYesNo::ShowAndGetInput(12012,20135,20022,20022))
        {
          CGUIWindowPrograms* pWindow = (CGUIWindowPrograms*)m_gWindowManager.GetWindow(WINDOW_PROGRAMS);
          if (pWindow)
            pWindow->PopulateTrainersList();
        }
      }
#endif
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
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    const CStdString& strRegion=pControl->GetCurrentLabel();
    g_langInfo.SetCurrentRegion(strRegion);
    g_guiSettings.SetString("locale.country", strRegion);
  }
  else if (strSetting.Equals("locale.timeserver") || strSetting.Equals("locale.timeaddress"))
  {
    g_application.StopTimeServer();
    if (g_guiSettings.GetBool("locale.timeserver"))
      g_application.StartTimeServer();
  }
  else if (strSetting.Equals("locale.time"))
  {
    SYSTEMTIME curTime;
    GetLocalTime(&curTime);
    if (CGUIDialogNumeric::ShowAndGetTime(curTime, g_localizeStrings.Get(14066)))
    { // yay!
      SYSTEMTIME curDate;
      GetLocalTime(&curDate);
      CUtil::SetSysDateTimeYear(curDate.wYear, curDate.wMonth, curDate.wDay, curTime.wHour, curTime.wMinute);
    }
  }
  else if (strSetting.Equals("locale.date"))
  {
    SYSTEMTIME curDate;
    GetLocalTime(&curDate);
    if (CGUIDialogNumeric::ShowAndGetDate(curDate, g_localizeStrings.Get(14067)))
    { // yay!
      SYSTEMTIME curTime;
      GetLocalTime(&curTime);
      CUtil::SetSysDateTimeYear(curDate.wYear, curDate.wMonth, curDate.wDay, curTime.wHour, curTime.wMinute);
    }
  }
  else if (strSetting.Equals("smb.winsserver") || strSetting.Equals("smb.workgroup") )
  {
    if (g_guiSettings.GetString("smb.winsserver") == "0.0.0.0")
      g_guiSettings.SetString("smb.winsserver", "");

    /* okey we really don't need to restarat, only deinit samba, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!dlg) return ;
    dlg->SetHeading( g_localizeStrings.Get(14038) );
    dlg->SetLine( 0, g_localizeStrings.Get(14039) );
    dlg->SetLine( 1, g_localizeStrings.Get(14040));
    dlg->SetLine( 2, "");
    dlg->DoModal();

    if (dlg->IsConfirmed())
    {
      g_applicationMessenger.RestartApp();
    }
  }
  else if (strSetting.Equals("upnp.client"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("upnp.client"))
      g_application.StartUPnPClient();
    else
      g_application.StopUPnPClient();
#endif
  }
  else if (strSetting.Equals("upnp.server"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("upnp.server"))
      g_application.StartUPnPServer();
    else
      g_application.StopUPnPServer();
#endif
  }
  else if (strSetting.Equals("upnp.renderer"))
  {
#ifdef HAS_UPNP
    if (g_guiSettings.GetBool("upnp.renderer"))
      g_application.StartUPnPRenderer();
    else
      g_application.StopUPnPRenderer();
#endif
  }
  else if (strSetting.Equals("upnp.musicshares"))
  {
    CStdString strDummy;
    g_settings.LoadUPnPXml(_P("q:\\system\\upnpserver.xml"));
    if (CGUIDialogFileBrowser::ShowAndGetShare(strDummy,false,&g_settings.m_vecUPnPMusicShares,"upnpmusic"))
      g_settings.SaveUPnPXml(_P("q:\\system\\upnpserver.xml"));
    else
      g_settings.LoadUPnPXml(_P("q:\\system\\upnpserver.xml"));
  }
  else if (strSetting.Equals("upnp.videoshares"))
  {
    CStdString strDummy;
    g_settings.LoadUPnPXml(_P("q:\\system\\upnpserver.xml"));
    if (CGUIDialogFileBrowser::ShowAndGetShare(strDummy,false,&g_settings.m_vecUPnPVideoShares,"upnpvideo"))
      g_settings.SaveUPnPXml(_P("q:\\system\\upnpserver.xml"));
    else
      g_settings.LoadUPnPXml(_P("q:\\system\\upnpserver.xml"));
  }
  else if (strSetting.Equals("upnp.pictureshares"))
  {
    CStdString strDummy;
    g_settings.LoadUPnPXml(_P("q:\\system\\upnpserver.xml"));
    if (CGUIDialogFileBrowser::ShowAndGetShare(strDummy,false,&g_settings.m_vecUPnPPictureShares,"upnppictures"))
      g_settings.SaveUPnPXml(_P("q:\\system\\upnpserver.xml"));
    else
      g_settings.LoadUPnPXml(_P("q:\\system\\upnpserver.xml"));
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
  else if (strSetting.Equals("musicfiles.savefolderviews"))
  {
    ClearFolderViews(pSettingControl->GetSetting(), WINDOW_MUSIC_FILES);
  }
  else if (strSetting.Equals("myvideos.savefolderviews"))
  {
    ClearFolderViews(pSettingControl->GetSetting(), WINDOW_VIDEO_FILES);
  }
  else if (strSetting.Equals("programfiles.savefolderviews"))
  {
    ClearFolderViews(pSettingControl->GetSetting(), WINDOW_PROGRAMS);
  }
  else if (strSetting.Equals("pictures.savefolderviews"))
  {
    ClearFolderViews(pSettingControl->GetSetting(), WINDOW_PICTURES);
  }
  UpdateSettings();
}

void CGUIWindowSettingsCategory::FreeControls()
{
  // clear the category group
  CGUIControlGroupList *control = (CGUIControlGroupList *)GetControl(CATEGORY_GROUP_ID);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_GROUPLIST)
#else
  if (control)
#endif
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
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (control && control->GetControlType() == CGUIControl::GUICONTROL_GROUPLIST)
#else
  if (control)
#endif
  {
    control->FreeResources();
    control->ClearAll();
  }
  m_vecSettings.clear();
}

void CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, float width, int &iControlID)
{
  CBaseSettingControl *pSettingControl = NULL;
  CGUIControl *pControl = NULL;
  CGUIControl *baseControl = NULL;
  if (pSetting->GetControlType() == CHECKMARK_CONTROL)
  {
    baseControl = m_pOriginalRadioButton;
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    baseControl = m_pOriginalSpin;
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetWidth(width);
    ((CGUISpinControlEx *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
    pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() == SEPARATOR_CONTROL && m_pOriginalImage)
  {
    baseControl = m_pOriginalImage;
    pControl = new CGUIImage(*m_pOriginalImage);
    if (!pControl) return;
    pControl->SetWidth(width);
    pSettingControl = new CSeparatorSettingControl((CGUIImage *)pControl, iControlID, pSetting);
  }
  else if (pSetting->GetControlType() != SEPARATOR_CONTROL) // button control
  {
    baseControl = m_pOriginalButton;
    pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl) return ;
    ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(width);
    pSettingControl = new CButtonSettingControl((CGUIButtonControl *)pControl, iControlID, pSetting);
  }
  if (!pControl) return;
  pControl->SetID(iControlID++);
  pControl->SetVisible(true);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(SETTINGS_GROUP_ID);
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
    m_vecSettings.push_back(pSettingControl);
  }
}

void CGUIWindowSettingsCategory::Render()
{
  // update realtime changeable stuff
  UpdateRealTimeSettings();
  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIControl *control = GetFirstFocusableControl(CONTROL_START_BUTTONS + m_iSection);
  if (control && !control->HasFocus() && control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
  {
    control->SetFocus(true);
    ((CGUIButtonControl *)control)->SetAlpha(0x80);
    bAlphaFaded = true;
  }
  CGUIWindow::Render();
  if (bAlphaFaded)
  {
    control->SetFocus(false);
    ((CGUIButtonControl *)control)->SetAlpha(0xFF);
  }
  // render the error message if necessary
  if (m_strErrorMessage.size())
  {
    CGUIFont *pFont = g_fontManager.GetFont("font13");
    if (pFont)
    {
      // our error message is standard ASCII - no need for charset conversions.
      CStdStringW strLabelUnicode = m_strErrorMessage;
      float fPosY = g_graphicsContext.GetHeight() * 0.8f;
      float fPosX = g_graphicsContext.GetWidth() * 0.5f;
      pFont->DrawText(fPosX, fPosY, 0xFFFFFFFF, 0, strLabelUnicode.c_str(), XBFONT_CENTER_X);
    }
  }
}

void CGUIWindowSettingsCategory::CheckNetworkSettings()
{
  // check if our network needs restarting (requires a reset, so check well!)
  if (m_iNetworkAssignment == -1)
  {
    // nothing to do here, folks - move along.
    return ;
  }
  // we need a reset if:
  // 1.  The Network Assignment has changed OR
  // 2.  The Network Assignment is STATIC and one of the network fields have changed
  if (m_iNetworkAssignment != g_guiSettings.GetInt("network.assignment") ||
      (m_iNetworkAssignment == NETWORK_STATIC && (
         m_strNetworkIPAddress != g_guiSettings.GetString("network.ipaddress") ||
         m_strNetworkSubnet != g_guiSettings.GetString("network.subnet") ||
         m_strNetworkGateway != g_guiSettings.GetString("network.gateway") ||
         m_strNetworkDNS != g_guiSettings.GetString("network.dns"))))
  {
/*    // our network settings have changed - we should prompt the user to reset XBMC
    if (CGUIDialogYesNo::ShowAndGetInput(14038, 14039, 14040, 0))
    {
      // reset settings
      g_applicationMessenger.RestartApp();
      // Todo: aquire new network settings without restart app!
    }
    else*/
    {
      g_network.NetworkMessage(CNetwork::SERVICES_DOWN,1);
      g_network.Deinitialize();
      g_network.Initialize(g_guiSettings.GetInt("network.assignment"),
        g_guiSettings.GetString("network.ipaddress").c_str(),
        g_guiSettings.GetString("network.subnet").c_str(),
        g_guiSettings.GetString("network.gateway").c_str(),
        g_guiSettings.GetString("network.dns").c_str());
    }


    // update our settings variables
    m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
    m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
    m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
    m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
    m_strNetworkDNS = g_guiSettings.GetString("network.dns");
    // replace settings
    /*   g_guiSettings.SetInt("network.assignment", m_iNetworkAssignment);
       g_guiSettings.SetString("network.ipaddress", m_strNetworkIPAddress);
       g_guiSettings.SetString("network.subnet", m_strNetworkSubnet);
       g_guiSettings.SetString("network.gateway", m_strNetworkGateway);
       g_guiSettings.SetString("network.dns", m_strNetworkDNS);*/
  }
}

void CGUIWindowSettingsCategory::FillInSubtitleHeights(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
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
  else
  {
    if (g_guiSettings.GetString("subtitles.font").size())
    {
      //find font sizes...
      CHDDirectory directory;
      CFileItemList items;
      CStdString strPath = "Q:\\system\\players\\mplayer\\font\\";
      strPath += g_guiSettings.GetString("subtitles.font");
      strPath += "\\";
      directory.GetDirectory(_P(strPath), items);
      int iCurrentSize = 0;
      int iSize = 0;
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItem* pItem = items[i];
        if (pItem->m_bIsFolder)
        {
          if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
          int iSizeTmp = atoi(pItem->GetLabel().c_str());
          if (iSizeTmp == pSettingInt->GetData())
            iCurrentSize = iSize;
          pControl->AddLabel(pItem->GetLabel(), iSize++);
        }
      }
      pControl->SetValue(iCurrentSize);
    }
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

  // Find mplayer fonts...
  {
    CHDDirectory directory;
    CFileItemList items;
    CStdString strPath = _P("Q:\\system\\players\\mplayer\\font\\");
    directory.GetDirectory(strPath, items);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItem* pItem = items[i];
      if (pItem->m_bIsFolder)
      {
        if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
        if (strcmpi(pItem->GetLabel().c_str(), pSettingString->GetData().c_str()) == 0)
          iCurrentFont = iFont;
        pControl->AddLabel(pItem->GetLabel(), iFont++);
      }
    }
  }

  // find TTF fonts
  {
    CHDDirectory directory;
    CFileItemList items;
    CStdString strPath = _P("Q:\\media\\Fonts\\");
    if (directory.GetDirectory(strPath, items))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItem* pItem = items[i];

        if (!pItem->m_bIsFolder)
        {

          if ( !CUtil::GetExtension(pItem->GetLabel()).Equals(".ttf") ) continue;
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
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();

  int iSkinFontSet = 0;

  m_strNewSkinFontSet.Empty();

  RESOLUTION res;
  CStdString strPath = g_SkinInfo.GetSkinPath("Font.xml", &res);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath.c_str()))
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
        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        bool isUnicode=(unicodeAttr && stricmp(unicodeAttr, "true") == 0);

        bool isAllowed=true;
        if (g_langInfo.ForceUnicodeFont() && !isUnicode)
          isAllowed=false;

        if (idAttr != NULL && isAllowed)
        {
          pControl->AddLabel(idAttr, iSkinFontSet);
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

void CGUIWindowSettingsCategory::FillInSkins(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CHDDirectory directory;
  CFileItemList items;
  CStdString strPath = _P("Q:\\skin\\");
  directory.GetDirectory(strPath, items);

  int iCurrentSkin = 0;
  int iSkin = 0;
  vector<CStdString> vecSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      //   if (g_SkinInfo.Check(pItem->m_strPath))
      //   {
      vecSkins.push_back(pItem->GetLabel());
      //   }
    }
  }

  sort(vecSkins.begin(), vecSkins.end(), sortstringbyname());
  for (int i = 0; i < (int) vecSkins.size(); ++i)
  {
    CStdString strSkin = vecSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("lookandfeel.skin").c_str()) == 0)
    {
      iCurrentSkin = iSkin;
    }
    pControl->AddLabel(strSkin, iSkin++);
  }
  pControl->SetValue(iCurrentSkin);
  return ;
}

void CGUIWindowSettingsCategory::FillInSoundSkins(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);

  m_strNewSkin.Empty();

  //find skins...
  CFileItemList items;
  CStdString strPath = _P("Q:\\sounds\\");
  CDirectory::GetDirectory(strPath, items);

  int iCurrentSoundSkin = 0;
  int iSoundSkin = 0;
  vector<CStdString> vecSoundSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
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
  for (int i = 0; i < (int) vecSoundSkins.size(); ++i)
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

void CGUIWindowSettingsCategory::FillInVisualisations(CSetting *pSetting, int iControlID)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  if (!pSetting) return;
  int iWinID = m_gWindowManager.GetActiveWindow();
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, iWinID, iControlID);
    g_graphicsContext.SendMessage(msg);
  }
  vector<CStdString> vecVis;
  //find visz....
  CHDDirectory directory;
  CFileItemList items;
  CStdString strPath = _P("Q:\\visualisations\\");
  directory.GetDirectory(strPath, items);
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".vis")
      {
#ifdef _LINUX
	void *handle = dlopen((const char*)pItem->m_strPath, RTLD_LAZY);
	if (!handle)
	  continue;
	dlclose(handle);
#endif
        CStdString strLabel = pItem->GetLabel();
        vecVis.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }

  CStdString strDefaultVis = pSettingString->GetData();
  if (!strDefaultVis.Equals("None"))
    strDefaultVis.Delete(strDefaultVis.size() - 4, 4);

  sort(vecVis.begin(), vecVis.end(), sortstringbyname());

  // add the "disabled" setting first
  int iVis = 0;
  int iCurrentVis = 0;
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
    msg.SetLabel(231);
    g_graphicsContext.SendMessage(msg);
  }
  for (int i = 0; i < (int) vecVis.size(); ++i)
  {
    CStdString strVis = vecVis[i];

    if (strcmpi(strVis.c_str(), strDefaultVis.c_str()) == 0)
      iCurrentVis = iVis;

    {
      CGUIMessage msg(GUI_MSG_LABEL_ADD, iWinID, iControlID, iVis++);
      msg.SetLabel(strVis);
      g_graphicsContext.SendMessage(msg);
    }
  }
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, iWinID, iControlID, iCurrentVis);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIWindowSettingsCategory::FillInVoiceMasks(DWORD dwPort, CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetShowRange(true); // show the range
  int iCurrentMask = 0;
  int iMask = 0;
  vector<CStdString> vecMask;

  //find masks in xml...
  TiXmlDocument xmlDoc;
  CStdString fileName = _P("Q:\\system\\voicemasks.xml");
  if ( !xmlDoc.LoadFile(fileName) ) return ;
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "VoiceMasks") return ;
  if (pRootElement)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild("Name");
    while (pChild)
    {
      if (pChild->FirstChild())
      {
        CStdString strName = pChild->FirstChild()->Value();
        vecMask.push_back(strName);
      }
      pChild = pChild->NextSibling("Name");
    }
  }
  xmlDoc.Clear();


  CStdString strDefaultMask = pSettingString->GetData();

  sort(vecMask.begin(), vecMask.end(), sortstringbyname());
//  CStdString strCustom = "Custom";
  CStdString strNone = "None";
//  vecMask.insert(vecMask.begin(), strCustom);
  vecMask.insert(vecMask.begin(), strNone);
  for (int i = 0; i < (int) vecMask.size(); ++i)
  {
    CStdString strMask = vecMask[i];

    if (strcmpi(strMask.c_str(), strDefaultMask.c_str()) == 0)
      iCurrentMask = iMask;

    pControl->AddLabel(strMask, iMask++);
  }

  pControl->SetValue(iCurrentMask);
}

void CGUIWindowSettingsCategory::FillInVoiceMaskValues(DWORD dwPort, CSetting *pSetting)
{
  CStdString strCurMask = g_guiSettings.GetString(pSetting->GetSetting());
  if (strCurMask.CompareNoCase("None") == 0 || strCurMask.CompareNoCase("Custom") == 0 )
  {
#ifndef HAS_XBOX_AUDIO
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)
#endif
    g_stSettings.m_karaokeVoiceMask[dwPort].energy = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].pitch = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].whisper = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].robotic = XVOICE_MASK_PARAM_DISABLED;
    return;
  }

  //find mask values in xml...
  TiXmlDocument xmlDoc;
  CStdString fileName = _P("Q:\\system\\voicemasks.xml");
  if ( !xmlDoc.LoadFile( fileName ) ) return ;
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "VoiceMasks") return ;
  if (pRootElement)
  {
    const TiXmlNode *pChild = pRootElement->FirstChild("Name");
    while (pChild)
    {
      CStdString strMask = pChild->FirstChild()->Value();
      if (strMask.CompareNoCase(strCurMask) == 0)
      {
        for (int i = 0; i < 4;i++)
        {
          pChild = pChild->NextSibling();
          if (pChild)
          {
            CStdString strValue = pChild->Value();
            if (strValue.CompareNoCase("fSpecEnergyWeight") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].energy = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fPitchScale") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].pitch = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fWhisperValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].whisper = (float) atof(strName.c_str());
              }
            }
            else if (strValue.CompareNoCase("fRoboticValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                g_stSettings.m_karaokeVoiceMask[dwPort].robotic = (float) atof(strName.c_str());
              }
            }
          }
        }
        break;
      }
      pChild = pChild->NextSibling("Name");
    }
  }
  xmlDoc.Clear();
}

void CGUIWindowSettingsCategory::FillInResolutions(CSetting *pSetting, bool playbackSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  // Find the valid resolutions and add them as necessary
  vector<RESOLUTION> res;
  g_graphicsContext.GetAllowedResolutions(res, false);

  /* add the virtual resolutions */
  res.push_back(AUTORES);

  for (vector<RESOLUTION>::iterator it = res.begin(); it != res.end();it++)
  {
    RESOLUTION res = *it;
    if (res == AUTORES)
    {
      if (playbackSetting)
      {
        //  TODO: localize 2.0
        if (g_videoConfig.Has1080i() || g_videoConfig.Has720p())
          pControl->AddLabel(g_localizeStrings.Get(20049) , res); // Best Available
        else if (g_videoConfig.HasWidescreen())
          pControl->AddLabel(g_localizeStrings.Get(20050) , res); // Autoswitch between 16x9 and 4x3
        else
          continue;   // don't have a choice of resolution (other than 480p vs NTSC, which isn't a choice)
      }
      else  // "Auto"
        pControl->AddLabel(g_localizeStrings.Get(14061), res);
    }
    else
    {
      pControl->AddLabel(g_settings.m_ResInfo[res].strMode, res);
    }
  }
  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInVSyncs(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(13106) , VSYNC_DISABLED);
  pControl->AddLabel(g_localizeStrings.Get(13107) , VSYNC_VIDEO);
  pControl->AddLabel(g_localizeStrings.Get(13108) , VSYNC_ALWAYS);

  pControl->SetValue(pSettingInt->GetData());
}

void CGUIWindowSettingsCategory::FillInLanguages(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  m_strNewLanguage.Empty();
  //find languages...
  CHDDirectory directory;
  CFileItemList items;

  CStdString strPath = _P("Q:\\language\\");
  directory.GetDirectory(strPath, items);

  int iCurrentLang = 0;
  int iLanguage = 0;
  vector<CStdString> vecLanguage;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), ".svn") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecLanguage.push_back(pItem->GetLabel());
    }
  }

  sort(vecLanguage.begin(), vecLanguage.end(), sortstringbyname());
  for (int i = 0; i < (int) vecLanguage.size(); ++i)
  {
    CStdString strLanguage = vecLanguage[i];
    if (strcmpi(strLanguage.c_str(), pSettingString->GetData().c_str()) == 0)
      iCurrentLang = iLanguage;
    pControl->AddLabel(strLanguage, iLanguage++);
  }

  pControl->SetValue(iCurrentLang);
}

void CGUIWindowSettingsCategory::FillInScreenSavers(CSetting *pSetting)
{ // Screensaver mode
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();

  pControl->AddLabel(g_localizeStrings.Get(351), 0); // Off
  pControl->AddLabel(g_localizeStrings.Get(352), 1); // Dim
  pControl->AddLabel(g_localizeStrings.Get(353), 2); // Black
  pControl->AddLabel(g_localizeStrings.Get(108), 3); // PictureSlideShow

  //find screensavers ....
  CHDDirectory directory;
  CFileItemList items;
  CStdString strPath = _P("Q:\\screensavers\\");
  directory.GetDirectory(strPath, items);

  int iCurrentScr = -1;
  int iScr = 0;
  vector<CStdString> vecScr;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".xbs")
      {
#ifdef _LINUX
	void *handle = dlopen((const char*)pItem->m_strPath, RTLD_LAZY);
	if (!handle)
	  continue;
	dlclose(handle);
#endif
        CStdString strLabel = pItem->GetLabel();
        vecScr.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }

  CStdString strDefaultScr = pSettingString->GetData();
  CStdString strExtension;
  CUtil::GetExtension(strDefaultScr, strExtension);
  if (strExtension == ".xbs")
    strDefaultScr.Delete(strDefaultScr.size() - 4, 4);

  sort(vecScr.begin(), vecScr.end(), sortstringbyname());
  for (int i = 0; i < (int) vecScr.size(); ++i)
  {
    CStdString strScr = vecScr[i];

    if (strcmpi(strScr.c_str(), strDefaultScr.c_str()) == 0)
      iCurrentScr = i + 4;  // 4: is the number of the predefined Screensavers!

    pControl->AddLabel(strScr, i + 4); // // 4: is the number of the predefined Screensavers!
  }

  // if we can't find the screensaver previously configured
  // then fallback to turning the screensaver off.
  if (iCurrentScr < 0)
  {
    if (strDefaultScr == "Dim")
      iCurrentScr = 1;
    else if (strDefaultScr == "Black")
      iCurrentScr = 2;
    else if (strDefaultScr == "SlideShow") // PictureSlideShow
      iCurrentScr = 3;
    else
    {
      iCurrentScr = 0;
      pSettingString->SetData("None");
    }
  }
  pControl->SetValue(iCurrentScr);
}

void CGUIWindowSettingsCategory::FillInFTPServerUser(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);
  int iDefaultFtpUser = 0;

#ifdef HAS_FTP_SERVER
  CStdString strFtpUser1; int iUserMax;
  // Get FTP XBOX Users and list them !
  if (CUtil::GetFTPServerUserName(0, strFtpUser1, iUserMax))
  {
    for (int i = 0; i < iUserMax; i++)
    {
      if (CUtil::GetFTPServerUserName(i, strFtpUser1, iUserMax))
        pControl->AddLabel(strFtpUser1.c_str(), i);
      if (strFtpUser1.ToLower() == "xbox") iDefaultFtpUser = i;
    }
    pControl->SetValue(iDefaultFtpUser);
    CUtil::GetFTPServerUserName(iDefaultFtpUser, strFtpUser1, iUserMax);
    g_guiSettings.SetString("servers.ftpserveruser", strFtpUser1.c_str());
    pControl->Update();
  }
  else { //Set "None" if there is no FTP User found!
    pControl->AddLabel(g_localizeStrings.Get(231).c_str(), 0);
    pControl->SetValue(0);
    pControl->Update();
  }
#endif
}
bool CGUIWindowSettingsCategory::SetFTPServerUserPass()
{
#ifdef HAS_FTP_SERVER
  // TODO: Read the FileZilla Server XML and Set it here!
  // Get GUI USER and pass and set pass to FTP Server
  CStdString strFtpUserName, strFtpUserPassword;
  strFtpUserName      = g_guiSettings.GetString("servers.ftpserveruser");
  strFtpUserPassword  = g_guiSettings.GetString("servers.ftpserverpassword");
  if(strFtpUserPassword.size()!=0)
  {
    if (CUtil::SetFTPServerUserPassword(strFtpUserName, strFtpUserPassword))
    {
      // todo! ERROR check! if something goes wrong on SetPW!
      // PopUp OK and Display: FTP Server Password was set succesfull!
      CGUIDialogOK::ShowAndGetInput(728, 0, 1247, 0);
    }
    return true;
  }
  else
  {
    // PopUp OK and Display: FTP Server Password is empty! Try Again!
    CGUIDialogOK::ShowAndGetInput(728, 0, 12358, 0);
  }
#endif
  return true;
}

void CGUIWindowSettingsCategory::FillInRegions(CSetting *pSetting)
{
  CSettingString *pSettingString = (CSettingString*)pSetting;
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

void CGUIWindowSettingsCategory::JumpToSection(DWORD dwWindowId, const CStdString &section)
{
  // grab our section
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(dwWindowId - WINDOW_SETTINGS_MYPICTURES);
  if (!pSettingsGroup) return;
  // get the categories we need
  vecSettingsCategory categories;
  pSettingsGroup->GetCategories(categories);
  // iterate through them and check for the required section
  int iSection = -1;
  for (unsigned int i = 0; i < categories.size(); i++)
    if (categories[i]->m_strCategory.Equals(section))
      iSection = i;
  if (iSection == -1) return;

  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSectionBeforeJump=m_iSection;
  m_iControlBeforeJump=m_lastControlID;
  m_iWindowBeforeJump=m_dwWindowId+m_iScreen;

  m_iSection=iSection;
  m_lastControlID=CONTROL_START_CONTROL;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, dwWindowId);
  OnMessage(msg1);
  for (unsigned int i=0; i<m_vecSections.size(); ++i)
  {
    CONTROL_DISABLE(CONTROL_START_BUTTONS+i)
  }
}

void CGUIWindowSettingsCategory::JumpToPreviousSection()
{
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSection=m_iSectionBeforeJump;
  m_lastControlID=m_iControlBeforeJump;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, m_iWindowBeforeJump);
  OnMessage(msg1);

  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
}

void CGUIWindowSettingsCategory::FillInSkinThemes(CSetting *pSetting)
{
  // There is a default theme (just Textures.xpr)
  // any other *.xpr files are additional themes on top of this one.
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skintheme");

  m_strNewSkinTheme.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standart Textures.xpr will be used!

  // find all *.xpr in this path
  CStdString strDefaultTheme = pSettingString->GetData();

  // Search for Themes in the Current skin!
  vector<CStdString> vecTheme;
  CUtil::GetSkinThemes(vecTheme);

  // Remove the .xpr extension from the Themes
  CStdString strExtension;
  CUtil::GetExtension(strSettingString, strExtension);
  if (strExtension == ".xpr") strSettingString.Delete(strSettingString.size() - 4, 4);
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
  // There is a default theme (just default.xml)
  // any other *.xml files are additional color themes on top of this one.
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  CStdString strSettingString = g_guiSettings.GetString("lookandfeel.skincolors");

  m_strNewSkinColors.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standard default.xml will be used!

  // Search for colors in the Current skin!
  vector<CStdString> vecColors;

  CStdString strPath;
  CUtil::AddFileToFolder(g_SkinInfo.GetBaseDir(),"colors",strPath);

  CHDDirectory directory;

  CFileItemList items;
  directory.SetMask(".xml");
  directory.GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (!pItem->m_bIsFolder && pItem->GetLabel().CompareNoCase("defaults.xml") != 0)
    { // not the default one
      CStdString strLabel = pItem->GetLabel();
      vecColors.push_back(strLabel.Mid(0, strLabel.size() - 4));
    }
  }
  sort(vecColors.begin(), vecColors.end(), sortstringbyname());

  // Remove the .xml extension from the Themes
  if (CUtil::GetExtension(strSettingString) == ".xml")
    CUtil::RemoveExtension(strSettingString);

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

  const vector<CSkinInfo::CStartupWindow> &startupWindows = g_SkinInfo.GetStartupWindows();

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
  m_iNetworkAssignment = g_guiSettings.GetInt("network.assignment");
  m_strNetworkIPAddress = g_guiSettings.GetString("network.ipaddress");
  m_strNetworkSubnet = g_guiSettings.GetString("network.subnet");
  m_strNetworkGateway = g_guiSettings.GetString("network.gateway");
  m_strNetworkDNS = g_guiSettings.GetString("network.dns");
  m_strOldTrackFormat = g_guiSettings.GetString("musicfiles.trackformat");
  m_strOldTrackFormatRight = g_guiSettings.GetString("musicfiles.trackformatright");
  m_NewResolution = INVALID;
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
  CGUIWindow *window = m_gWindowManager.GetWindow(windowID);
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

// check and clear our folder views if applicable.
void CGUIWindowSettingsCategory::ClearFolderViews(CSetting *pSetting, int windowID)
{
  CSettingBool *pSettingBool = (CSettingBool*)pSetting;
  if (!pSettingBool->GetData())
  { // clear out our db
    CViewDatabase db;
    if (db.Open())
    {
      db.ClearViewStates(WINDOW_MUSIC_FILES);
      db.Close();
    }
  }
}
