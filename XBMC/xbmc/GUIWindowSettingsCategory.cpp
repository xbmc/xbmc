#include "stdafx.h"
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "FileSystem/HDDirectory.h"
#include "Util.h"
#include "GUILabelControl.h"
#include "GUICheckMarkControl.h"
#include "Utils/Weather.h"
#include "MusicDatabase.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include "Utils/LED.h"
#include "Utils/LCDFactory.h"
#include "Utils/FanController.h"
#include "PlayListPlayer.h"
#include "SkinInfo.h"
#include "GUIFontManager.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "lib/libscrobbler/scrobbler.h"
#include "GUIPassword.h"
#include "utils/GUIInfoManager.h"


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
#define CONTROL_DEFAULT_SETTINGS_BUTTON 10
#define CONTROL_DEFAULT_SEPARATOR       11
#define CONTROL_START_BUTTONS           30
#define CONTROL_START_CONTROL           50

struct sortstringbyname
{
  bool operator()(const CStdString& strItem1, const CStdString& strItem2)
  {
    CStdString strLine1 = strItem1;
    CStdString strLine2 = strItem2;
    strLine1 = strLine1.ToLower();
    strLine2 = strLine2.ToLower();
    return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
  }
};

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
    : CGUIWindow(0)
{
  m_iLastControl = -1;
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalButton = NULL;
  m_pOriginalSettingsButton = NULL;
  m_pOriginalImage = NULL;
  // set the correct ID range...
  m_dwIDRange = 8;
  m_iScreen = 0;
  // set the network settings so that we don't reset them unnecessarily
  m_iNetworkAssignment = -1;
  m_strErrorMessage = L"";
  m_strOldTrackFormat = "";
  m_strOldTrackFormatRight = "";
  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
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

    m_iLastControl = -1;

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
  case GUI_MSG_SETFOCUS:
    {
      m_iLastControl = message.GetControlId();
      CBaseSettingControl *pSkinControl = GetSetting("LookAndFeel.Skin");
      CBaseSettingControl *pLanguageControl = GetSetting("LookAndFeel.Language");
      if (g_application.m_dwSkinTime && ((pSkinControl && pSkinControl->GetID() == message.GetControlId()) || (pLanguageControl && pLanguageControl->GetID() == message.GetControlId())))
      {
        // Cancel skin and language load if one of the spin controls lose focus
        g_application.CancelDelayLoadSkin();
        // Reset spin controls to the current selected skin & language
        if (pSkinControl) FillInSkins(pSkinControl->GetSetting());
        if (pLanguageControl) FillInLanguages(pLanguageControl->GetSetting());
      }
      unsigned int iControl = message.GetControlId();
      unsigned int iSender = message.GetSenderId();
      // if both the sender and the control are within out category range, then we have a change of
      // category.
      if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size() &&
          iSender >= CONTROL_START_BUTTONS && iSender < CONTROL_START_BUTTONS + m_vecSections.size())
      {
        // change the setting...
        if (iControl - CONTROL_START_BUTTONS != m_iSection)
        {
          m_iSection = iControl - CONTROL_START_BUTTONS;
          CheckNetworkSettings();
          CreateSettings();
        }
      }
    }
    break;
  case GUI_MSG_LOAD_SKIN:
    {
      unsigned iCtrlID = GetFocusedControl();

      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iCtrlID, 0, 0, NULL);
      g_graphicsContext.SendMessage(msg);

      // Do we need to reload the language file
      if (!m_strNewLanguage.IsEmpty())
      {
        g_guiSettings.SetString("LookAndFeel.Language", m_strNewLanguage);
        g_settings.Save();

        CStdString strLanguage = m_strNewLanguage;
        CStdString strPath = "Q:\\language\\";
        strPath += strLanguage;
        strPath += "\\strings.xml";
        g_localizeStrings.Load(strPath);
      }

      // Do we need to reload the skin font set
      if (!m_strNewSkinFontSet.IsEmpty())
      {
        g_guiSettings.SetString("LookAndFeel.Font", m_strNewSkinFontSet);
        g_settings.Save();
      }

      // Reload another skin
      if (!m_strNewSkin.IsEmpty())
      {
        g_guiSettings.SetString("LookAndFeel.Skin", m_strNewSkin);
        g_settings.Save();
      }

      // Reload the skin
      int iWindowID = GetID();
      g_application.LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));

      m_gWindowManager.ActivateWindow(iWindowID);
      SET_CONTROL_FOCUS(iCtrlID, msg.GetParam2());
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      if (message.GetParam1() != WINDOW_INVALID)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section
        m_iSection = 0;
      }
      m_iScreen = (int)message.GetParam2() - (int)m_dwWindowId;
      m_iNetworkAssignment = g_guiSettings.GetInt("Network.Assignment");
      m_strNetworkIPAddress = g_guiSettings.GetString("Network.IPAddress");
      m_strNetworkSubnet = g_guiSettings.GetString("Network.Subnet");
      m_strNetworkGateway = g_guiSettings.GetString("Network.Gateway");
      m_strNetworkDNS = g_guiSettings.GetString("Network.DNS");
      m_strOldTrackFormat = g_guiSettings.GetString("MusicLists.TrackFormat");
      m_strOldTrackFormatRight = g_guiSettings.GetString("MusicLists.TrackFormatRight");
      m_dwResTime = 0;
      m_OldResolution = (RESOLUTION)g_guiSettings.GetInt("LookAndFeel.Resolution");
      int iFocusControl = m_iLastControl;

      CGUIWindow::OnMessage(message);

      SetupControls();

      if (iFocusControl > -1)
      {
        SET_CONTROL_FOCUS(iFocusControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      //restore resolution setting to original if we were in the middle of changing
      if (m_dwResTime) g_guiSettings.SetInt("LookAndFeel.Resolution", m_OldResolution);
      m_OldResolution = INVALID;

      // Hardware based stuff
      // TODO: This should be done in a completely separate screen
      // to give warning to the user that it writes to the EEPROM.
      if ((g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL))
      {
        g_audioConfig.SetAC3Enabled(g_guiSettings.GetBool("AudioOutput.AC3PassThrough"));
        g_audioConfig.SetDTSEnabled(g_guiSettings.GetBool("AudioOutput.DTSPassThrough"));
        if (g_audioConfig.NeedsSave())
        { // should we perhaps show a dialog here?
          g_audioConfig.Save();
        }
      }
      CheckNetworkSettings();
      FreeControls();
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsCategory::SetupControls()
{
  // cleanup first, if necessary
  FreeControls();
  // get the area to use...
  const CGUIControl *pButtonArea = GetControl(CONTROL_BUTTON_AREA);
  const CGUIControl *pControlGap = GetControl(CONTROL_BUTTON_GAP);
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalSettingsButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_SETTINGS_BUTTON);
  m_pOriginalButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  m_pOriginalImage = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalButton || !pButtonArea || !pControlGap)
    return ;
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalButton->SetVisible(false);
  m_pOriginalSettingsButton->SetVisible(false);
  if (m_pOriginalImage) m_pOriginalImage->SetVisible(false);
  // setup our control groups...
  m_vecGroups.clear();
  m_vecGroups.push_back( -1);
  // get a list of different sections
  CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(m_iScreen);
  if (!pSettingsGroup) return ;
  // update the screen string
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, pSettingsGroup->GetLabelID());
  // get the categories we need
  pSettingsGroup->GetCategories(m_vecSections);
  // run through and create our buttons...
  for (unsigned int i = 0; i < m_vecSections.size(); i++)
  {
    CGUIButtonControl *pButton = new CGUIButtonControl(*m_pOriginalSettingsButton);
    pButton->SetText(g_localizeStrings.Get(m_vecSections[i]->m_dwLabelID));
    pButton->SetID(CONTROL_START_BUTTONS + i);
    pButton->SetGroup(CONTROL_GROUP_BUTTONS);
    pButton->SetPosition(pButtonArea->GetXPosition(), pButtonArea->GetYPosition() + i*pControlGap->GetHeight());
    pButton->SetNavigation(CONTROL_START_BUTTONS + (int)i - 1, CONTROL_START_BUTTONS + i + 1, CONTROL_START_CONTROL, CONTROL_START_CONTROL);
    pButton->SetVisible(true);
    pButton->AllocResources();
    Add(pButton);
  }
  // update the first and last buttons...
  CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START_BUTTONS);
  pControl->SetNavigation(CONTROL_START_BUTTONS + (int)m_vecSections.size() - 1, pControl->GetControlIdDown(),
                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(CONTROL_START_BUTTONS + (int)m_vecSections.size() - 1);
  pControl->SetNavigation(pControl->GetControlIdUp(), CONTROL_START_BUTTONS,
                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  if (m_iSection < 0 || m_iSection >= (int)m_vecSections.size())
    m_iSection = 0;
  CreateSettings();
  // set focus correctly
  m_dwDefaultFocusControlID = CONTROL_START_BUTTONS;
}

void CGUIWindowSettingsCategory::CreateSettings()
{
  FreeSettingsControls();
  m_vecGroups.push_back( -1); // add the control group
  const CGUIControl *pControlArea = GetControl(CONTROL_AREA);
  const CGUIControl *pControlGap = GetControl(CONTROL_GAP);
  if (!pControlArea || !pControlGap)
    return ;
  int iPosX = pControlArea->GetXPosition();
  int iWidth = pControlArea->GetWidth();
  int iPosY = pControlArea->GetYPosition();
  int iGapY = pControlGap->GetHeight();
  vecSettings settings;
  g_guiSettings.GetSettingsGroup(m_vecSections[m_iSection]->m_strCategory, settings);
  int iControlID = CONTROL_START_CONTROL;
  for (unsigned int i = 0; i < settings.size(); i++)
  {
    CSetting *pSetting = settings[i];
    AddSetting(pSetting, iPosX, iPosY, iGapY, iWidth, iControlID);
    CStdString strSetting = pSetting->GetSetting();
    if (strSetting == "Pictures.AutoSwitchMethod" || strSetting == "ProgramsLists.AutoSwitchMethod" || strSetting == "MusicLists.AutoSwitchMethod" || strSetting == "VideoLists.AutoSwitchMethod")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        pControl->AddLabel(g_localizeStrings.Get(14015 + i), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Weather.TemperatureUnits")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("°C", 0);
      pControl->AddLabel("°F", 1);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Weather.SpeedUnits")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("km/h", 0);
      pControl->AddLabel("mph", 1);
      pControl->AddLabel("m/s", 2);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "MyMusic.Visualisation")
    {
      FillInVisualisations(pSetting, GetSetting(pSetting->GetSetting())->GetID());
    }
    else if (strSetting == "Karaoke.Port0VoiceMask" /*"VoiceOnPort0.VoiceMask"*/)
    {
      FillInVoiceMasks(0, pSetting);
    }
    else if (strSetting == "Karaoke.Port1VoiceMask" /*"VoiceOnPort1.VoiceMask"*/)
    {
      FillInVoiceMasks(1, pSetting);
    }
    else if (strSetting == "Karaoke.Port2VoiceMask" /*"VoiceOnPort2.VoiceMask"*/)
    {
      FillInVoiceMasks(2, pSetting);
    }
    else if (strSetting == "Karaoke.Port3VoiceMask" /*"VoiceOnPort3.VoiceMask"*/)
    {
      FillInVoiceMasks(3, pSetting);
    }
    else if (strSetting == "AudioOutput.Mode")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(338), AUDIO_ANALOG);
      if (g_audioConfig.HasDigitalOutput())
        pControl->AddLabel(g_localizeStrings.Get(339), AUDIO_DIGITAL);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "CDDARipper.Encoder")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("Lame", CDDARIP_ENCODER_LAME);
      pControl->AddLabel("Vorbis", CDDARIP_ENCODER_VORBIS);
      pControl->AddLabel("Wav", CDDARIP_ENCODER_WAV);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "CDDARipper.Quality")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(604), CDDARIP_QUALITY_CBR);
      pControl->AddLabel(g_localizeStrings.Get(601), CDDARIP_QUALITY_MEDIUM);
      pControl->AddLabel(g_localizeStrings.Get(602), CDDARIP_QUALITY_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(603), CDDARIP_QUALITY_EXTREME);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "LCD.Mode")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), LCD_MODE_NONE);
      pControl->AddLabel(g_localizeStrings.Get(630), LCD_MODE_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(14023), LCD_MODE_NOTV);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "LCD.Type")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("LCD - HD44780", LCD_TYPE_LCD_HD44780);
      pControl->AddLabel("LCD - KS0073", LCD_TYPE_LCD_KS0073);
      pControl->AddLabel("VFD", LCD_TYPE_VFD);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "LCD.ModChip")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("SmartXX", MODCHIP_SMARTXX);
      pControl->AddLabel("Xenium", MODCHIP_XENIUM);
      pControl->AddLabel("Xecuter3", MODCHIP_XECUTER3);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "System.TargetTemperature")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        CStdString strLabel;
        if (g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /* DEGREES_F */)
          strLabel.Format("%2.0f%cF", ((9.0 / 5.0) * (float)i) + 32.0, 176);
        else
          strLabel.Format("%i%cC", i, 176);
        pControl->AddLabel(strLabel, i - pSettingInt->m_iMin);
      }
      pControl->SetValue(pSettingInt->GetData() - pSettingInt->m_iMin);
    }
    else if (strSetting == "System.FanSpeed")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      CStdString strPercentMask = g_localizeStrings.Get(14047);
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i += 5)
      {
        CStdString strLabel;
        strLabel.Format(strPercentMask.c_str(), i*2);
        pControl->AddLabel(strLabel, i);
      }
      pControl->SetValue(int(pSettingInt->GetData() / 5) - 1);
    }
    else if (strSetting == "System.RemotePlayHDSpinDown")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(474), SPIN_DOWN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(475), SPIN_DOWN_MUSIC);
      pControl->AddLabel(g_localizeStrings.Get(13002), SPIN_DOWN_VIDEO);
      pControl->AddLabel(g_localizeStrings.Get(476), SPIN_DOWN_BOTH);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Servers.WebServerPassword")
    { // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetPassword());
        g_settings.Save();
      }
    }
    else if (strSetting == "Network.Assignment")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(718), NETWORK_DASH);
      pControl->AddLabel(g_localizeStrings.Get(716), NETWORK_DHCP);
      pControl->AddLabel(g_localizeStrings.Get(717), NETWORK_STATIC);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Subtitles.Style")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(738), XFONT_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(739), XFONT_BOLD);
      pControl->AddLabel(g_localizeStrings.Get(740), XFONT_ITALICS);
      pControl->AddLabel(g_localizeStrings.Get(741), XFONT_BOLDITALICS);
      pControl->SetValue(pSettingInt->GetData() - 1);
    }
    else if (strSetting == "Subtitles.Color")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = SUBTITLE_COLOR_START; i <= SUBTITLE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(760 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Subtitles.Height")
    {
      FillInSubtitleHeights(pSetting);
    }
    else if (strSetting == "Subtitles.Font")
    {
      FillInSubtitleFonts(pSetting);
    }
    else if (strSetting == "Subtitles.CharSet" || strSetting == "LookAndFeel.CharSet")
    {
      FillInCharSets(pSetting);
    }
    else if (strSetting == "LookAndFeel.Font")
    {
      FillInSkinFonts(pSetting);
    }
    else if (strSetting == "LookAndFeel.Skin")
    {
      FillInSkins(pSetting);
    }
    else if (strSetting == "LookAndFeel.SoundSkin")
    {
      FillInSoundSkins(pSetting);
    }
    else if (strSetting == "LookAndFeel.Language")
    {
      FillInLanguages(pSetting);
    }
    else if (strSetting == "LookAndFeel.Resolution")
    {
      FillInResolutions(pSetting);
    }
    else if (strSetting == "ScreenSaver.Mode")
    {
      FillInScreenSavers(pSetting);
    }
    else if (strSetting == "LED.Colour")
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
    else if (strSetting == "LED.DisableOnPlayback")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), LED_PLAYBACK_OFF);     // No
      pControl->AddLabel(g_localizeStrings.Get(13002), LED_PLAYBACK_VIDEO);   // Video Only
      pControl->AddLabel(g_localizeStrings.Get(475), LED_PLAYBACK_MUSIC);    // Music Only
      pControl->AddLabel(g_localizeStrings.Get(476), LED_PLAYBACK_VIDEO_MUSIC); // Video & Music
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Filters.RenderMethod")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13355), RENDER_LQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(13356), RENDER_OVERLAYS);
      pControl->AddLabel(g_localizeStrings.Get(13357), RENDER_HQ_RGB_SHADER);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "MyVideos.ViewMode")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = VIEW_MODE_NORMAL; i < VIEW_MODE_CUSTOM; i++)
        pControl->AddLabel(g_localizeStrings.Get(630 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "MyMusic.ReplayGainType")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), REPLAY_GAIN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(639), REPLAY_GAIN_TRACK);
      pControl->AddLabel(g_localizeStrings.Get(640), REPLAY_GAIN_ALBUM);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Smb.Ip")
	  {	// GeminiServer
      g_guiSettings.SetString("Smb.Ip",         g_stSettings.m_strSambaIPAdress);
      g_guiSettings.SetString("Smb.Workgroup",  g_stSettings.m_strSambaWorkgroup);
      g_guiSettings.SetString("Smb.Username",   g_stSettings.m_strSambaDefaultUserName);
      g_guiSettings.SetString("Smb.Password",   g_stSettings.m_strSambaDefaultPassword);
      g_guiSettings.SetString("Smb.ShareName",  g_stSettings.m_strSambaShareName);
      g_guiSettings.SetString("Smb.Winsserver", g_stSettings.m_strSambaWinsServer);
	  }
    else if (strSetting == "Smb.ShareGroup")
	  {	// GeminiServer
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(1211), SMB_SHARE_MUSIC	        );       
      pControl->AddLabel(g_localizeStrings.Get(1212), SMB_SHARE_VIDEO	        );    
      pControl->AddLabel(g_localizeStrings.Get(1213), SMB_SHARE_PICTURES	    );    
      pControl->AddLabel(g_localizeStrings.Get(1214), SMB_SHARE_FILES         );    
      pControl->AddLabel(g_localizeStrings.Get(1215), SMB_SHARE_MU_VI         );    
      pControl->AddLabel(g_localizeStrings.Get(1216), SMB_SHARE_MU_PIC        );    
      pControl->AddLabel(g_localizeStrings.Get(1217), SMB_SHARE_MU_FIL        );    
      pControl->AddLabel(g_localizeStrings.Get(1218), SMB_SHARE_VI_PIC        );    
      pControl->AddLabel(g_localizeStrings.Get(1219), SMB_SHARE_VI_FIL        );    
      pControl->AddLabel(g_localizeStrings.Get(1220), SMB_SHARE_PIC_FIL       );    
      pControl->AddLabel(g_localizeStrings.Get(1221), SMB_SHARE_MU_VI_PIC     );    
      pControl->AddLabel(g_localizeStrings.Get(1226), SMB_SHARE_FIL_VI_MU     );   
      pControl->AddLabel(g_localizeStrings.Get(1227), SMB_SHARE_FIL_PIC_MU    );   
      pControl->AddLabel(g_localizeStrings.Get(1228), SMB_SHARE_FIL_PIC_VI    );   
      pControl->AddLabel(g_localizeStrings.Get(1222), SMB_SHARE_MU_VI_PIC_FIL );
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Smb.SimpAdvance")
	  {	// GeminiServer
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(1223), 0 );    //Disabled User
      pControl->AddLabel(g_localizeStrings.Get(1224), 1 );    //Normal User
      pControl->AddLabel(g_localizeStrings.Get(1225), 2 );    //Advanced User
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Masterlock.Mastermode")
    {
      //GeminiServer
      g_guiSettings.SetInt("Masterlock.Mastermode", g_stSettings.m_iMasterLockMode );
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(1223), LOCK_MODE_EVERYONE );    //Disabled
      pControl->AddLabel(g_localizeStrings.Get(12337), LOCK_MODE_NUMERIC );    //Numeric
      pControl->AddLabel(g_localizeStrings.Get(12338), LOCK_MODE_GAMEPAD );    //Gamepad
      pControl->AddLabel(g_localizeStrings.Get(12339), LOCK_MODE_QWERTY );    //Text
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Masterlock.Maxretry")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      CStdString cLbl[10]= {"0","1","2","3","4","5","6","7","8","9"};
      pControl->AddLabel(g_localizeStrings.Get(1223), 0);   //Disabled
      for (unsigned int i = 1; i <= 9; i++)  pControl->AddLabel(cLbl[i], i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Masterlock.Enableshutdown")
    {
      bool bmcesState, bmcptState;
      if (g_stSettings.m_iMasterLockEnableShutdown == 0) bmcesState = false;  else bmcesState = true;
      if (g_stSettings.m_iMasterLockProtectShares  == 0) bmcptState = false;  else bmcptState = true;
      g_guiSettings.SetBool("Masterlock.Enableshutdown", bmcesState);
      g_guiSettings.SetBool("Masterlock.Protectshares", bmcptState);
      g_guiSettings.SetInt("Masterlock.Maxretry", g_stSettings.m_iMasterLockMaxRetry);
      g_guiSettings.SetString("Masterlock.Mastercode", g_stSettings.m_masterLockCode);
    }
    else if (strSetting == "LookAndFeel.StartUpWindow")
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(514),  0 );  // Manual Settings [XBMC-XML]
      pControl->AddLabel(g_localizeStrings.Get(513),  1	);  // 0    XBMC Home
      pControl->AddLabel(g_localizeStrings.Get(0),    2	);  // 1    My Programs
      pControl->AddLabel(g_localizeStrings.Get(1),    3	);  // 2    My Pictures
      pControl->AddLabel(g_localizeStrings.Get(2),    4 );  // 501  My Musik
      pControl->AddLabel(g_localizeStrings.Get(3),    5 );  // 6    My Video
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Masterlock.LockFilemanager" || strSetting == "Masterlock.LockSettings")
    {
      bool bLFState, bLSState;
      if (g_stSettings.m_iMasterLockFilemanager == 0) bLFState = false;  else bLFState = true;
      if (g_stSettings.m_iMasterLockSettings    == 0) bLSState = false;  else bLSState = true;
      g_guiSettings.SetBool("Masterlock.LockFilemanager", bLFState);
      g_guiSettings.SetBool("Masterlock.LockSettings", bLSState);
    }
    else if (strSetting == "Masterlock.LockHomeMedia")
    {
      g_guiSettings.SetInt("Masterlock.LockHomeMedia", g_stSettings.m_iMasterLockHomeMedia);
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(1223)  , LOCK_DISABLED      );  // 0 Disabled
      pControl->AddLabel(g_localizeStrings.Get(2)     , LOCK_MUSIC         );  // 1 Musik
      pControl->AddLabel(g_localizeStrings.Get(3)     , LOCK_VIDEO         );  // 2 Video
      pControl->AddLabel(g_localizeStrings.Get(1)     , LOCK_PICTURES      );  // 3 Pictures
      pControl->AddLabel(g_localizeStrings.Get(0)     , LOCK_PROGRAMS      );  // 4 Programs
      pControl->AddLabel(g_localizeStrings.Get(1215)  , LOCK_MU_VI         );  // 5 Musik & Video
      pControl->AddLabel(g_localizeStrings.Get(1216)  , LOCK_MU_PIC        );  // 6 Musik & Picture
      pControl->AddLabel(g_localizeStrings.Get(1229)  , LOCK_MU_PROG       );  // 7 Musik & Programs
      pControl->AddLabel(g_localizeStrings.Get(1218)  , LOCK_VI_PIC        );  // 8 Video & Pcitures
      pControl->AddLabel(g_localizeStrings.Get(1230)  , LOCK_VI_PROG       );  // 9 Video & Programs
      pControl->AddLabel(g_localizeStrings.Get(1231)  , LOCK_PIC_PROG      );  // 10 Picture & Programs
      pControl->AddLabel(g_localizeStrings.Get(1221)  , LOCK_MU_VI_PIC     );  // 11 Musik & Video & Pictures
      pControl->AddLabel(g_localizeStrings.Get(1233)  , LOCK_PROG_VI_MU    );  // 12 Programs & Video & Musik
      pControl->AddLabel(g_localizeStrings.Get(1234)  , LOCK_PROG_PIC_MU   );  // 13 Programs & Pictures & Musik
      pControl->AddLabel(g_localizeStrings.Get(1235)  , LOCK_PROG_PIC_VI   );  // 14 Musik & Picture & Video
      pControl->AddLabel(g_localizeStrings.Get(1232)  , LOCK_MU_VI_PIC_PROG);  // 15 Musik & Video & Pictures & Programs
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting == "Servers.FTPServerUser")
    {
      //GeminiServer
      FillInFTPServerUser(pSetting);
    }
    else if (strSetting == "Autodetect.NickName")
    {
      //GeminiServer
      char pszNickName[MAX_NICKNAME];
      CStdString strXboxNickName;
      if (XFindFirstNickname(true,(LPWSTR)pszNickName,MAX_NICKNAME) != INVALID_HANDLE_VALUE)  
        strXboxNickName = pszNickName; 
      else 
      { //Todo: Can be more then One NickName!!
        strXboxNickName = g_guiSettings.GetString("Autodetect.NickName");
        XSetNickname((LPCWSTR)strXboxNickName.c_str(), true);
      }
      g_guiSettings.SetString("Autodetect.NickName", strXboxNickName.c_str());
    }
  }
  // fix first and last navigation
  CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START_CONTROL + (int)m_vecSettings.size() - 1);
  if (pControl) pControl->SetNavigation(pControl->GetControlIdUp(), CONTROL_START_CONTROL,
                                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(CONTROL_START_CONTROL);
  if (pControl) pControl->SetNavigation(CONTROL_START_CONTROL + (int)m_vecSettings.size() - 1, pControl->GetControlIdDown(),
                                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
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
    if (strSetting == "Pictures.AutoSwitchUseLargeThumbs")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Pictures.UseAutoSwitching"));
    }
    else if (strSetting == "ProgramsLists.AutoSwitchUseLargeThumbs" || strSetting == "ProgramsLists.AutoSwitchMethod")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("ProgramsLists.UseAutoSwitching"));
    }
    else if (strSetting == "ProgramsLists.AutoSwitchPercentage")
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("ProgramsLists.UseAutoSwitching") && g_guiSettings.GetInt("ProgramsLists.AutoSwitchMethod") == 2);
    }
    else if (strSetting == "MusicLists.AutoSwitchUseLargeThumbs" || strSetting == "MusicLists.AutoSwitchMethod")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MusicLists.UseAutoSwitching"));
    }
    else if (strSetting == "MusicLists.AutoSwitchPercentage")
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MusicLists.UseAutoSwitching") && g_guiSettings.GetInt("MusicLists.AutoSwitchMethod") == 2);
    }
    else if (strSetting == "VideoLists.AutoSwitchUseLargeThumbs" || strSetting == "VideoLists.AutoSwitchMethod")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VideoLists.UseAutoSwitching"));
    }
    else if (strSetting == "VideoLists.AutoSwitchPercentage")
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VideoLists.UseAutoSwitching") && g_guiSettings.GetInt("VideoLists.AutoSwitchMethod") == 2);
    }
    else if (strSetting == "CDDARipper.Quality")
    { // only visible if we are doing non-WAV ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("CDDARipper.Encoder") != CDDARIP_ENCODER_WAV);
    }
    else if (strSetting == "CDDARipper.Bitrate")
    { // only visible if we are ripping to CBR
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled((g_guiSettings.GetInt("CDDARipper.Encoder") != CDDARIP_ENCODER_WAV) &&
                                           (g_guiSettings.GetInt("CDDARipper.Quality") == CDDARIP_QUALITY_CBR));
    }
    // Karaoke patch (114097) ...
 /*   else if (strSetting == "VoiceOnPort0.VoiceMask")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VoiceOnPort0.EnableDevice"));
    }*/
    else if (strSetting == "VoiceOnPort0.EnablefSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort0.fSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort0.EnablefSpecEnergyWeight")
                                           && (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort0.EnablefPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort0.fPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort0.EnablefPitchScale")
                                           && (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort0.EnablefWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort0.fWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort0.EnablefWhisperValue")
                                           && (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort0.EnablefRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort0.fRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort0.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort0.EnablefRoboticValue")
                                           && (g_guiSettings.GetString("VoiceOnPort0.VoiceMask").compare("Custom") == 0) );
    }
    /*    else if (strSetting == "VoiceOnPort1.VoiceMask")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VoiceOnPort1.EnableDevice"));
    }*/
    else if (strSetting == "VoiceOnPort1.EnablefSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort1.fSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort1.EnablefSpecEnergyWeight")
                                           && (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort1.EnablefPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort1.fPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort1.EnablefPitchScale")
                                           && (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort1.EnablefWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort1.fWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort1.EnablefWhisperValue")
                                           && (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort1.EnablefRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort1.fRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort1.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort1.EnablefRoboticValue")
                                           && (g_guiSettings.GetString("VoiceOnPort1.VoiceMask").compare("Custom") == 0) );
    }
    /*    else if (strSetting == "VoiceOnPort2.VoiceMask")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VoiceOnPort2.EnableDevice"));
    }*/
    else if (strSetting == "VoiceOnPort2.EnablefSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort2.fSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort2.EnablefSpecEnergyWeight")
                                           && (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort2.EnablefPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort2.fPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort2.EnablefPitchScale")
                                           && (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort2.EnablefWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort2.fWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort2.EnablefWhisperValue")
                                           && (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort2.EnablefRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort2.fRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort2.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort2.EnablefRoboticValue")
                                           && (g_guiSettings.GetString("VoiceOnPort2.VoiceMask").compare("Custom") == 0) );
    }
    /*    else if (strSetting == "VoiceOnPort3.VoiceMask")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VoiceOnPort3.EnableDevice"));
    }*/
    else if (strSetting == "VoiceOnPort3.EnablefSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort3.fSpecEnergyWeight")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort3.EnablefSpecEnergyWeight")
                                           && (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort3.EnablefPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort3.fPitchScale")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort3.EnablefPitchScale")
                                           && (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort3.EnablefWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort3.fWhisperValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort3.EnablefWhisperValue")
                                           && (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort3.EnablefRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "VoiceOnPort3.fRoboticValue")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(/*g_guiSettings.GetBool("VoiceOnPort3.EnableDevice")
                                           &&*/ g_guiSettings.GetBool("VoiceOnPort3.EnablefRoboticValue")
                                           && (g_guiSettings.GetString("VoiceOnPort3.VoiceMask").compare("Custom") == 0) );
    }
    else if (strSetting == "MyMusic.OutputToAllSpeakers" || strSetting == "AudioVideo.OutputToAllSpeakers"|| strSetting == "AudioOutput.AC3PassThrough" || strSetting == "AudioOutput.DTSPassThrough")
    { // only visible if we are in digital mode
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL);
    }
    else if (strSetting == "System.FanSpeed")
    { // only visible if we have fancontrolspeed enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("System.FanSpeedControl"));
    }
    else if (strSetting == "System.TargetTemperature")
    { // only visible if we have autotemperature enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("System.AutoTemperature"));
    }
    else if (strSetting == "System.RemotePlayHDSpinDownDelay")
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.RemotePlayHDSpinDown") != SPIN_DOWN_NONE);
    }
    else if (strSetting == "System.RemotePlayHDSpinDownMinDuration")
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.RemotePlayHDSpinDown") != SPIN_DOWN_NONE);
    }
    else if (strSetting == "System.ShutDownWhilePlaying")
    { // only visible if we have shutdown enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.ShutDownTime") != 0);
    }
    else if (strSetting == "Servers.FTPServerUser" || strSetting == "Servers.FTPServerPassword" || strSetting == "Servers.FTPAutoFatX")
    {
      //GeminiServer
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("Servers.FTPServer"));
    }
    else if (strSetting == "Servers.WebServerPassword")
    { // Fill in a blank pass if we don't have it
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (((CSettingString *)pSettingControl->GetSetting())->GetData().size() == 0 && pControl)
      {
        pControl->SetText2(g_localizeStrings.Get(734));
        pControl->SetEnabled(g_guiSettings.GetBool("Servers.WebServer"));
      }
    }
    else if (strSetting == "Servers.WebServerPort")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Servers.WebServer"));
    }
    else if (strSetting == "Network.IPAddress" || strSetting == "Network.Subnet" || strSetting == "Network.Gateway" || strSetting == "Network.DNS")
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if (g_guiSettings.GetInt("Network.Assignment") != NETWORK_STATIC) pControl->SetText2("-");
        pControl->SetEnabled(g_guiSettings.GetInt("Network.Assignment") == NETWORK_STATIC);
      }
    }
    else if (strSetting == "Network.HTTPProxyServer" || strSetting == "Network.HTTPProxyPort")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Network.UseHTTPProxy"));
    }
    else if (strSetting == "PostProcessing.VerticalDeBlockLevel")
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.VerticalDeBlocking") &&
                           g_guiSettings.GetBool("PostProcessing.Enable") &&
                           !g_guiSettings.GetBool("PostProcessing.Auto"));
    }
    else if (strSetting == "PostProcessing.HorizontalDeBlockLevel")
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.HorizontalDeBlocking") &&
                           g_guiSettings.GetBool("PostProcessing.Enable") &&
                           !g_guiSettings.GetBool("PostProcessing.Auto"));
    }
    else if (strSetting == "Filters.NoiseLevel")
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("Filters.Noise"));
    }
    else if (strSetting == "PostProcessing.VerticalDeBlocking" || strSetting == "PostProcessing.HorizontalDeBlocking" || strSetting == "PostProcessing.AutoBrightnessContrastLevels" || strSetting == "PostProcessing.DeRing")
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.Enable") &&
                           !g_guiSettings.GetBool("PostProcessing.Auto"));
    }
    else if (strSetting == "PostProcessing.Auto")
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.Enable"));
    }
    else if (strSetting == "MyVideos.WidescreenSwitching")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_videoConfig.HasWidescreen());
    }
    else if (strSetting == "MyVideos.PAL60Switching")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_videoConfig.HasPAL60());
    }
    else if (strSetting == "MyVideos.UseGUIResolution")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_videoConfig.Has720p() || g_videoConfig.Has1080i());
    }
    else if (strSetting == "Subtitles.Color" || strSetting == "Subtitles.Style" || strSetting == "Subtitles.CharSet")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(CUtil::IsUsingTTFSubtitles());
    }
    else if (strSetting == "Subtitles.FlipBiDiCharSet")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled( /*CUtil::IsUsingTTFSubtitles() &&*/ g_charsetConverter.isBidiCharset(g_guiSettings.GetString("Subtitles.CharSet")) > 0);
    }
    else if (strSetting == "LookAndFeel.CharSet")
    { // TODO: Determine whether we are using a TTF font or not.
      //   CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      //   if (pControl) pControl->SetEnabled(g_guiSettings.GetString("LookAndFeel.Font").Right(4) == ".ttf");
    }
    else if (strSetting == "ScreenSaver.DimLevel")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode") == "Dim");
    }
    else if (strSetting == "ScreenSaver.Preview")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode") != "None");
    }
    else if (strSetting.Left(16) == "Weather.AreaCode")
    {
      CSettingString *pSetting = (CSettingString *)GetSetting(strSetting)->GetSetting();
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetText2(pSetting->GetData());
    }
    else if (strSetting == "LED.DisableOnPlayback")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      int iColour = g_guiSettings.GetInt("LED.Colour");
      pControl->SetEnabled(iColour != LED_COLOUR_NO_CHANGE && iColour != LED_COLOUR_OFF);
    }
    else if (strSetting == "MyMusic.AudioScrobblerUserName" || strSetting == "MyMusic.AudioScrobblerPassword")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MyMusic.UseAudioScrobbler"));
    }
    else if (strSetting == "MusicLists.TrackFormat")
    {
      if (m_strOldTrackFormat != g_guiSettings.GetString("MusicLists.TrackFormat"))
      {
        CUtil::DeleteDatabaseDirectoryCache();
        m_strOldTrackFormat = g_guiSettings.GetString("MusicLists.TrackFormat");
      }
    }
    else if (strSetting == "MusicLists.TrackFormatRight")
    {
      if (m_strOldTrackFormatRight != g_guiSettings.GetString("MusicLists.TrackFormatRight"))
      {
        CUtil::DeleteDatabaseDirectoryCache();
        m_strOldTrackFormatRight = g_guiSettings.GetString("MusicLists.TrackFormatRight");
      }
    }
	  else if (strSetting == "XBDateTime.TimeAddress")
    {
		  CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("XBDateTime.TimeServer"));
    }
	  else if (strSetting == "XBDateTime.Time" || strSetting == "XBDateTime.Date")
	  {
		  CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("XBDateTime.TimeServer")); 
		  SYSTEMTIME curTime;
		  GetLocalTime(&curTime);
      CStdString time;
      if (strSetting == "XBDateTime.Time")
        time = g_infoManager.GetTime(false);  // false for no seconds
      else
        time = g_infoManager.GetDate();  // false as we want numbers
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
    else if (strSetting == "Smb.Ip" || strSetting == "Smb.Workgroup"  || strSetting == "Smb.Username" || strSetting == "Smb.Password" || strSetting == "Smb.SetSmb")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      bool bState;
      if (g_guiSettings.GetInt("Smb.SimpAdvance") == 0) bState = false; //Disable
      if (g_guiSettings.GetInt("Smb.SimpAdvance") == 1) bState = true;  //Normal User
      if (g_guiSettings.GetInt("Smb.SimpAdvance") == 2) bState = true;  //Advanced User
      if (pControl) pControl->SetEnabled(bState);
    }
    else if (strSetting == "Smb.Winsserver" || strSetting == "Smb.ShareName" || strSetting == "Smb.ShareGroup")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      bool bState;
      if (g_guiSettings.GetInt("Smb.SimpAdvance") == 0) bState = false; //Disable
      if (g_guiSettings.GetInt("Smb.SimpAdvance") == 1) bState = false; //Normal User
      if (g_guiSettings.GetInt("Smb.SimpAdvance") == 2) bState = true;  //Advanced User
      if (pControl) pControl->SetEnabled(bState);
    }
    else if (strSetting == "Masterlock.Mastermode")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());  
      if (g_guiSettings.GetInt("Masterlock.Mastermode") == 1) g_guiSettings.GetString("Masterlock.Mastercode") = "-";
      if (g_guiSettings.GetInt("Masterlock.Mastermode") == 2) g_guiSettings.GetString("Masterlock.Mastercode") = "-";
      if (g_guiSettings.GetInt("Masterlock.Mastermode") == 3) g_guiSettings.GetString("Masterlock.Mastercode") = "-";

      if (g_guiSettings.GetInt("Masterlock.Mastermode") == 0) // Disabled !!
      {
        if(CheckMasterLockCode())
        {
          g_stSettings.m_iMasterLockMaxRetry              = 0;
          g_stSettings.m_iMasterLockEnableShutdown        = 0;
          g_stSettings.m_iMasterLockProtectShares         = 0;
          g_stSettings.m_iMasterLockMode                  = 0;
          g_stSettings.m_iMasterLockStartupLock           = 0;
          g_stSettings.m_masterLockCode = "-";
          g_stSettings.m_iMasterLockFilemanager           = 0;
          g_stSettings.m_iMasterLockSettings              = 0;
          g_stSettings.m_iMasterLockHomeMedia             = 0;
          
          g_settings.UpDateXbmcXML("masterlock", "mastermode",      "0");
          g_settings.UpDateXbmcXML("masterlock", "mastercode",      "-");
          g_settings.UpDateXbmcXML("masterlock", "maxretry",        "0");
          g_settings.UpDateXbmcXML("masterlock", "enableshutdown",  "0");
          g_settings.UpDateXbmcXML("masterlock", "protectshares",   "0");
          g_settings.UpDateXbmcXML("masterlock", "startuplock",     "0");

          g_settings.UpDateXbmcXML("masterlock", "LockFilemanager", "0");
          g_settings.UpDateXbmcXML("masterlock", "LockSettings",    "0");
          g_settings.UpDateXbmcXML("masterlock", "LockHomeMedia",   "0");
        }
        else
        {
          // PopUp OK and Display: Master Code is not Valid or is empty or not set!
          CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          if (!dlg) return ;
          dlg->SetHeading( g_localizeStrings.Get(12360));
          dlg->SetLine( 0, g_localizeStrings.Get(12367));
          dlg->SetLine( 1, g_localizeStrings.Get(12368));
          dlg->SetLine( 2, "");
          dlg->DoModal( m_gWindowManager.GetActiveWindow() );
        }
      }
    }
    else if (strSetting == "Masterlock.Enableshutdown" || strSetting == "Masterlock.Protectshares" || strSetting == "Masterlock.Maxretry" || strSetting == "Masterlock.Mastercode" || strSetting == "Masterlock.SetMasterlock" || strSetting == "Masterlock.StartupLock" || strSetting =="Masterlock.LockFilemanager" || strSetting =="Masterlock.LockSettings" || strSetting =="Masterlock.LockHomeMedia")
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      bool bState;
      if (g_guiSettings.GetInt("Masterlock.Mastermode") == 0) bState = false; //Disable
        else bState = true;
      if (pControl) pControl->SetEnabled(bState);
    }
    else if (strSetting == "Autodetect.NickName" || strSetting == "Autodetect.CreateLink" || strSetting == "Autodetect.PopUpInfo" || strSetting == "Autodetect.SendUserPw" || strSetting == "Autodetect.PingTime")
    {
      //GeminiServer
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Autodetect.OnOff"));
    }

  }
}
void CGUIWindowSettingsCategory::UpdateRealTimeSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
	  if (strSetting == "XBDateTime.Time" || strSetting == "XBDateTime.Date")
	  {
		  CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("XBDateTime.TimeServer")); 
		  SYSTEMTIME curTime;
		  GetLocalTime(&curTime);
      CStdString time;
      if (strSetting == "XBDateTime.Time")
        time = g_infoManager.GetTime(false);  // false for no seconds
      else
        time = g_infoManager.GetDate();  // false as we want numbers
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
  }
}

void CGUIWindowSettingsCategory::OnClick(CBaseSettingControl *pSettingControl)
{
  CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
  if (strSetting.Left(16) == "Weather.AreaCode")
  {
    CStdString strSearch;
    if (CGUIDialogKeyboard::ShowAndGetInput(strSearch, (CStdStringW)g_localizeStrings.Get(14024), false))
    {
      strSearch.Replace(" ", "+");
      CStdString strResult = ((CSettingString *)pSettingControl->GetSetting())->GetData();
      if (g_weatherManager.GetSearchResults(strSearch, strResult))
        ((CSettingString *)pSettingControl->GetSetting())->SetData(strResult);
    }
  }
  // call the control to do it's thing
  pSettingControl->OnClick();
  // ok, now check the various special things we need to do
  if (strSetting == "MyPrograms.UseDirectoryName")
  { // delete the program database.
    // TODO: Should this actually be done here??
    CStdString programDatabase = g_stSettings.m_szAlbumDirectory;
    programDatabase += PROGRAM_DATABASE_NAME;
    if (CFile::Exists(programDatabase))
      ::DeleteFile(programDatabase.c_str());
  }
  else if (strSetting == "MyMusic.Visualisation")
  { // new visualisation choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue() == 0)
      pSettingString->SetData("None");
    else
      pSettingString->SetData(pControl->GetCurrentLabel() + ".vis");
  }
  else if (strSetting == "MusicFiles.Repeat")
  {
    g_playlistPlayer.Repeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("MusicFiles.Repeat"));
  }
  else if (strSetting == "Karaoke.Port0VoiceMask" /*"VoiceOnPort0.VoiceMask"*/)
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Karaoke.Port0VoiceMask" /*"VoiceOnPort0.VoiceMask"*/, pControl->GetCurrentLabel());
    FillInVoiceMaskValues(0, g_guiSettings.GetSetting("Karaoke.Port0VoiceMask" /*"VoiceOnPort0.VoiceMask"*/));
  }
  else if (strSetting == "Karaoke.Port1VoiceMask" /*"VoiceOnPort1.VoiceMask"*/)
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Karaoke.Port1VoiceMask" /*"VoiceOnPort1.VoiceMask"*/, pControl->GetCurrentLabel());
    FillInVoiceMaskValues(1, g_guiSettings.GetSetting("Karaoke.Port1VoiceMask" /*"VoiceOnPort1.VoiceMask"*/));
  }
  else if (strSetting == "Karaoke.Port2VoiceMask" /*"VoiceOnPort2.VoiceMask"*/)
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Karaoke.Port2VoiceMask" /*"VoiceOnPort2.VoiceMask"*/, pControl->GetCurrentLabel());
    FillInVoiceMaskValues(2, g_guiSettings.GetSetting("Karaoke.Port2VoiceMask" /*"VoiceOnPort2.VoiceMask"*/));
  }
  else if (strSetting == "Karaoke.Port2VoiceMask" /*"VoiceOnPort3.VoiceMask"*/)
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("VoiceOnPort3.VoiceMask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(3, g_guiSettings.GetSetting("Karaoke.Port2VoiceMask" /*"VoiceOnPort3.VoiceMask"*/));
  }
  else if (strSetting == "MusicLibrary.Cleanup")
  {
    g_musicDatabase.Clean();
    CUtil::DeleteDatabaseDirectoryCache();
  }
  else if (strSetting == "MyMusic.JumpToAudioHardware")
  {
    JumpToSection(WINDOW_SETTINGS_SYSTEM, 5);
  }
  else if (strSetting == "MyMusic.UseAudioScrobbler" || strSetting == "MyMusic.AudioScrobblerUserName" || strSetting == "MyMusic.AudioScrobblerPassword")
  {
    if (g_guiSettings.GetBool("MyMusic.UseAudioScrobbler"))
    {
      CStdString strPassword=g_guiSettings.GetString("MyMusic.AudioScrobblerPassword");
      CStdString strUserName=g_guiSettings.GetString("MyMusic.AudioScrobblerUserName");
      if (!strUserName.IsEmpty() || !strPassword.IsEmpty())
        CScrobbler::GetInstance()->Init();
    }
    else
    {
      CScrobbler::GetInstance()->Term();
    }
  }
  else if (strSetting == ("MyMusic.OutputToAllSpeakers") || (strSetting == "AudioVideo.OutputToAllSpeakers") )
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();

    if (!g_application.IsPlaying())
    {
      g_audioContext.RemoveActiveDevice();
      g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
    }
  }
  else if (strSetting == "LCD.Mode")
  {
    g_lcd->Initialize();
  }
  else if (strSetting == "LCD.BackLight")
  {
    g_lcd->SetBackLight(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting == "LCD.ModChip")
  {
    g_lcd->Stop();
    CLCDFactory factory;
    g_lcd = factory.Create();
    g_lcd->Initialize();
  }
  else if (strSetting == "LCD.Contrast")
  {
    g_lcd->SetContrast(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting == "System.TargetTemperature")
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    CFanController::Instance()->SetTargetTemperature(pSetting->GetData());
  }
  else if (strSetting == "System.FanSpeed")
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    int iSpeed = (RESOLUTION)msg.GetParam1();
    g_guiSettings.SetInt("System.FanSpeed", iSpeed);
    CFanController::Instance()->SetFanSpeed(iSpeed);
  }
  else if (strSetting == "System.AutoTemperature")
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
    if (pSetting->GetData())
    {
      g_guiSettings.SetBool("System.FanSpeedControl", false);
      CFanController::Instance()->Start(g_guiSettings.GetInt("System.TargetTemperature"));
    }
    else
      CFanController::Instance()->Stop();
  }
  else if (strSetting == "System.FanSpeedControl")
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
    if (pSetting->GetData())
    {
      g_guiSettings.SetBool("System.AutoTemperature", false);
      CFanController::Instance()->Stop();
      CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("System.FanSpeed"));
    }
    else
      CFanController::Instance()->RestoreStartupSpeed();
  }
  else if (strSetting == "Autodetect.NickName" )
  {
    CStdString strNickName   = g_guiSettings.GetString("Autodetect.NickName");
    //Todo: MAX_NICKNAME
    XSetNickname((LPCWSTR)strNickName.c_str(), true);
  }
  else if (strSetting == "Servers.FTPServer")
  {
    g_application.StopFtpServer();
    if (g_guiSettings.GetBool("Servers.FTPServer"))
      g_application.StartFtpServer();
    
  }
  else if (strSetting == "Servers.FTPServerPassword")
  {
   SetFTPServerUserPass(); 
  }
  else if (strSetting == "Servers.FTPServerUser")
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Servers.FTPServerUser", pControl->GetCurrentLabel());
  }

  else if (strSetting == "Servers.WebServer" || strSetting == "Servers.WebServerPort" || strSetting == "Servers.WebServerPassword")
  {
    if (strSetting == "Servers.WebServerPort")
    {
      CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
      // check that it's a valid port
      int port = atoi(pSetting->GetData().c_str());
      if (port <= 0 || port > 65535)
        pSetting->SetData("80");
    }
    g_application.StopWebServer();
    if (g_guiSettings.GetBool("Servers.WebServer"))
    {
      g_application.StartWebServer();
      CStdString strPassword = g_guiSettings.GetString("Servers.WebServerPassword");
//      if (strPassword.size() > 0)
      g_application.m_pWebServer->SetPassword((char_t*)strPassword.c_str());
    }
  }
  else if (strSetting == "Network.HTTPProxyPort")
  {
    CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
    // check that it's a valid port
    int port = atoi(pSetting->GetData().c_str());
    if (port <= 0 || port > 65535)
      pSetting->SetData("8080");
  }
  else if (strSetting == "MyVideos.Calibrate")
  { // activate the video calibration screen
    m_gWindowManager.ActivateWindow(WINDOW_MOVIE_CALIBRATION);
  }
  else if (strSetting == "Subtitles.Height")
  {
    if (!CUtil::IsUsingTTFSubtitles())
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
    }
  }
  else if (strSetting == "Subtitles.Font")
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    FillInSubtitleHeights(g_guiSettings.GetSetting("Subtitles.Height"));
  }
  else if (strSetting == "Subtitles.CharSet")
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && newCharset != pSettingString->GetData())
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting == "LookAndFeel.CharSet")
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString newCharset = g_charsetConverter.getCharsetNameByLabel(pControl->GetCurrentLabel());
    if (newCharset != "" && newCharset != pSettingString->GetData())
    {
      pSettingString->SetData(newCharset);
      g_charsetConverter.reset();
    }
  }
  else if (strSetting == "LookAndFeel.Font")
  { // new font choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkinFontSet = pControl->GetCurrentLabel();
    if (strSkinFontSet != "CVS" && strSkinFontSet != g_guiSettings.GetString("LookAndFeel.Font"))
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
  else if (strSetting == "LookAndFeel.Skin")
  { // new skin choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkin = pControl->GetCurrentLabel();
    CStdString strSkinPath = "Q:\\skin\\" + strSkin;
    if (g_SkinInfo.Check(strSkinPath))
    {
      m_strErrorMessage.Empty();
      pControl->SetSpinTextColor(pControl->GetButtonTextColor());
      if (strSkin != "CVS" && strSkin != g_guiSettings.GetString("LookAndFeel.Skin"))
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
      m_strErrorMessage.Format(L"Incompatible skin. We require skins of version %0.2f or higher", g_SkinInfo.GetMinVersion());
      m_strNewSkin.Empty();
      g_application.CancelDelayLoadSkin();
      pControl->SetSpinTextColor(pControl->GetDisabledColor());
    }
  }
  else if (strSetting == "LookAndFeel.SoundSkin")
  { // new sound skin choosen...
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue()==0)
      g_guiSettings.SetString("LookAndFeel.SoundSkin", "OFF");
    else if (pControl->GetValue()==1)
      g_guiSettings.SetString("LookAndFeel.SoundSkin", "SKINDEFAULT");
    else
      g_guiSettings.SetString("LookAndFeel.SoundSkin", pControl->GetCurrentLabel());

    g_audioManager.Load();
  }
  else if (strSetting == "LookAndFeel.GUICentering")
  { // activate the video calibration screen
    m_gWindowManager.ActivateWindow(WINDOW_UI_CALIBRATION);
  }
  else if (strSetting == "LookAndFeel.Resolution")
  { // new resolution choosen... - update if necessary
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    m_NewResolution = (RESOLUTION)msg.GetParam1();
    // delay change of resolution
    if (m_NewResolution != m_OldResolution)
    {
      m_dwResTime = timeGetTime() + 2000;
    }
  }
  else if (strSetting == "LookAndFeel.Language")
  { // new language choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strLanguage = pControl->GetCurrentLabel();
    if (strLanguage != "CVS" && strLanguage != pSettingString->GetData())
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
  else if (strSetting == "UIFilters.Flicker" || strSetting == "UIFilters.Soften")
  { // reset display
    g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
  }
  else if (strSetting == "ScreenSaver.Mode")
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
    else
      strScreenSaver = pControl->GetCurrentLabel() + ".xbs";
    pSettingString->SetData(strScreenSaver);
  }
  else if (strSetting == "ScreenSaver.Preview")
  {
    g_application.ActivateScreenSaver();
  }
  else if (strSetting == "LED.Colour")
  { // Alter LED Colour immediately
    ILED::CLEDControl(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Left(10).Equals("ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("MyMusic.ReplayGainType");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("MyMusic.ReplayGainPreAmp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("MyMusic.ReplayGainNoGainPreAmp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("MyMusic.ReplayGainAvoidClipping");
  }


  else if (strSetting == "XBDateTime.TimeServer" || strSetting == "XBDateTime.TimeAddress")
  {
    g_application.StopTimeServer();
    if (g_guiSettings.GetBool("XBDateTime.TimeServer"))
      g_application.StartTimeServer();
  }
  else if (strSetting == "XBDateTime.Time")
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
  else if (strSetting == "XBDateTime.Date")
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
  else if (strSetting == "Smb.SetSmb" || strSetting == "Smb.SimpAdvance")
  {
    //Let's get the ip and set the correct url string smb://IP !
    CStdString strSmbIp;
    strSmbIp.Format("smb://%s/",g_guiSettings.GetString("Smb.Ip"));
    
    // We need the previos smb share name to remove them
    CStdString strSmbShareName;
    strSmbShareName.Format("%s",g_stSettings.m_strSambaShareName);
    
    //Delete all Previos Share neames before add. new/changed one!
    //!! If there are a share , with the same name but non SMB, this will be alo deleted!
    g_settings.DeleteBookmark("music",    strSmbShareName, strSmbIp);
    g_settings.DeleteBookmark("video",    strSmbShareName, strSmbIp);
    g_settings.DeleteBookmark("pictures", strSmbShareName, strSmbIp);
    g_settings.DeleteBookmark("files",    strSmbShareName, strSmbIp);

    //Set Default if all is Empty
    if (g_guiSettings.GetString("Smb.Ip")        == "") g_guiSettings.SetString("Smb.Ip", "192.168.0.5");
    if (g_guiSettings.GetString("Smb.Workgroup") == "") g_guiSettings.SetString("Smb.Workgroup", "WORKGROUP");
    if (g_guiSettings.GetString("Smb.Username")  == "") g_guiSettings.SetString("Smb.Username", "-");
    if (g_guiSettings.GetString("Smb.Password")  == "") g_guiSettings.SetString("Smb.Password", "-");
    if (g_guiSettings.GetString("Smb.ShareName") == "") g_guiSettings.SetString("Smb.ShareName", "WORKGROUP (SMB) Network");
    if (g_guiSettings.GetString("Smb.Winsserver")== "") g_guiSettings.SetString("Smb.Winsserver", "-");

    //Set/Update
    strcpy(g_stSettings.m_strSambaIPAdress,         g_guiSettings.GetString("Smb.Ip"));
    strcpy(g_stSettings.m_strSambaWorkgroup,        g_guiSettings.GetString("Smb.Workgroup"));
    strcpy(g_stSettings.m_strSambaDefaultUserName,  g_guiSettings.GetString("Smb.Username"));
    strcpy(g_stSettings.m_strSambaDefaultPassword,  g_guiSettings.GetString("Smb.Password"));
    strcpy(g_stSettings.m_strSambaShareName,        g_guiSettings.GetString("Smb.ShareName"));
    strcpy(g_stSettings.m_strSambaWinsServer,       g_guiSettings.GetString("Smb.Winsserver"));

    g_settings.UpDateXbmcXML("samba", "winsserver",       g_guiSettings.GetString("Smb.Winsserver"));
    g_settings.UpDateXbmcXML("samba", "smbip",            g_guiSettings.GetString("Smb.Ip"));
    g_settings.UpDateXbmcXML("samba", "workgroup",        g_guiSettings.GetString("Smb.Workgroup"));
    g_settings.UpDateXbmcXML("samba", "defaultusername",  g_guiSettings.GetString("Smb.Username"));
    g_settings.UpDateXbmcXML("samba", "defaultpassword",  g_guiSettings.GetString("Smb.Password"));
    g_settings.UpDateXbmcXML("samba", "smbsharename",     g_guiSettings.GetString("Smb.ShareName"));
    
    //g_settings.UpDateXbmcXML("CDDBIpAddress", "194.97.4.18"); //This is a Test ;

    // if the SMB settings is disabled set also the Share group to 0
    if(g_guiSettings.GetInt("Smb.SimpAdvance")!= 0)
    {
      //if the user mode is //Normal User, Set the SMB share to all Modes!
      if (g_guiSettings.GetInt("Smb.SimpAdvance") == 1) g_guiSettings.SetInt("Smb.ShareGroup",SMB_SHARE_MU_VI_PIC_FIL);

      switch (g_guiSettings.GetInt("Smb.ShareGroup"))
      {
        case SMB_SHARE_MUSIC:
          //Musikc
            if (!g_settings.UpdateBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
            { g_settings.AddBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp); }
          break;
        case SMB_SHARE_VIDEO:
          //Video
            if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
            {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          break;
        case SMB_SHARE_PICTURES:
          //Pictures
            if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
            {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}

          break;
        case SMB_SHARE_FILES:
          //Files
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          break;
        case SMB_SHARE_MU_VI:
          //Musik&Video
          if(!g_settings.UpdateBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("music",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          break;
        case SMB_SHARE_MU_PIC:
          //Musik&Pictures
          if(!g_settings.UpdateBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("music",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          break;
        case SMB_SHARE_MU_FIL:
          //Musik&Files
          if(!g_settings.UpdateBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("music",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          break;
        case SMB_SHARE_VI_PIC:
          //Video&Pic
          if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          break;
        case SMB_SHARE_VI_FIL:
          //Video&Files
          if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          break;
        case SMB_SHARE_PIC_FIL:
          //Picture&Files
          if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          break;
        case SMB_SHARE_MU_VI_PIC:
          //Musik&Video&Picture
          if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          break;
        case SMB_SHARE_FIL_VI_MU:
          //Files&Video&Music
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("music",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          break;
        case SMB_SHARE_FIL_PIC_MU:
          //Files&Pictures&Music
          if(!g_settings.UpdateBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("music",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          break;
        case SMB_SHARE_FIL_PIC_VI:
          //Files&Pictures&VIdeo
          if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          break;
        case SMB_SHARE_MU_VI_PIC_FIL:
          //Musik&Video&Picture&File
          if(!g_settings.UpdateBookmark("music",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("music",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("video",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("video",    g_guiSettings.GetString("Smb.ShareName"),strSmbIp);}
          if(!g_settings.UpdateBookmark("pictures", g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("pictures",    g_guiSettings.GetString("Smb.ShareName"), strSmbIp);}
          if(!g_settings.UpdateBookmark("files",    g_guiSettings.GetString("Smb.ShareName"), "path", strSmbIp))
          {g_settings.AddBookmark("files",    g_guiSettings.GetString("Smb.ShareName"),  strSmbIp);}
          break;
      }
    }
    if (strSetting == "Smb.SetSmb")
    {
      CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (!dlg) return ;
      dlg->SetHeading( g_localizeStrings.Get(1200) );
      dlg->SetLine( 0, g_localizeStrings.Get(1209) );
      dlg->SetLine( 1, g_guiSettings.GetString("Smb.ShareName"));
      dlg->SetLine( 2, strSmbIp);
      dlg->DoModal( m_gWindowManager.GetActiveWindow() );
      //if (dlg->IsConfirmed()) //Do nothing!
    }

  }
  else if (strSetting == "Masterlock.Mastercode")
  {
    
   CStdString strTempMasterCode;
    // prompt user for mastercode if the mastercode was set b4 or by xml
   if (CheckMasterLockCode()) // Now Prompt User to enter the old and then the new MasterCode! Choosed GUI LOCK Mode will appear!
    {
      CStdString strNewPassword;
      switch (g_guiSettings.GetInt("Masterlock.Mastermode"))
      {
          case LOCK_MODE_NUMERIC:
            CGUIDialogNumeric::ShowAndGetNewPassword(strNewPassword);
            break;
          case LOCK_MODE_GAMEPAD:
            CGUIDialogGamepad::ShowAndGetNewPassword(strNewPassword);
            break;
          case LOCK_MODE_QWERTY:
            CGUIDialogKeyboard::ShowAndGetNewPassword(strNewPassword);
            break;
      }
      strTempMasterCode = strNewPassword;
    }
   if (strTempMasterCode != "" && strTempMasterCode != "-")
    {
     g_guiSettings.SetString("Masterlock.Mastercode", strTempMasterCode.c_str());
    }
   CStdString strTEST = g_guiSettings.GetString("Masterlock.Mastercode");
  }
  else if (strSetting == "Masterlock.SetMasterlock")
  {
    bool bIsMasterMode = true;
    int iStateSD, iStatePS, iStateSL, iLockModeP, iLockModeN, iLFState, iLSState;
    CStdString csMMode, csMRetry, csStateSD, csStatePS, csStateSL, csMCode, strMLC, csLFState, csLSState;
    CStdString cLbl[16]= {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15"};
    csMCode = g_guiSettings.GetString("Masterlock.Mastercode");
    strMLC  = g_stSettings.m_masterLockCode;
    iLockModeP = g_stSettings.m_iMasterLockMode;
    iLockModeN = g_guiSettings.GetInt("Masterlock.Mastermode");
    
    //Some Converting stuff!
    csMMode   = cLbl[g_guiSettings.GetInt("Masterlock.Mastermode")];
    csMRetry  = cLbl[g_guiSettings.GetInt("Masterlock.Maxretry")];
    
    if (g_guiSettings.GetBool("Masterlock.Enableshutdown")) { iStateSD = 1; csStateSD = "1"; } else { iStateSD = 0; csStateSD = "0"; }
    if (g_guiSettings.GetBool("Masterlock.Protectshares"))  { iStatePS = 1; csStatePS = "1"; } else { iStatePS = 0; csStatePS = "0"; }
    if (g_guiSettings.GetBool("Masterlock.StartupLock"))    { iStateSL = 1; csStateSL = "1"; } else { iStateSL = 0; csStateSL = "0"; }
    if (g_guiSettings.GetBool("Masterlock.LockFilemanager")){ iLFState = 1; csLFState = "1"; } else { iLFState = 0; csLFState = "0"; }
    if (g_guiSettings.GetBool("Masterlock.LockSettings"))   { iLSState = 1; csLSState = "1"; } else { iLSState = 0; csLSState = "0"; }
    
    if((iLockModeP == iLockModeN && csMCode == strMLC)||(iLockModeP != iLockModeN && csMCode != strMLC)||(iLockModeP == iLockModeN && csMCode != strMLC))
    {
      if (g_guiSettings.GetString("Masterlock.Mastercode")!= "" && g_guiSettings.GetString("Masterlock.Mastercode")!= "-")
      {
        //Check if the MasterLockCode is changed or not! If not PopUP MasterLockCode! 
        if (g_stSettings.m_masterLockCode == g_guiSettings.GetString("Masterlock.Mastercode"))
        { if(!CheckMasterLockCode()) bIsMasterMode = false; }
        
        if (bIsMasterMode)
        {
          //Set Master Lock Changes to the Internal Settings!
          g_stSettings.m_iMasterLockMaxRetry              = g_guiSettings.GetInt("Masterlock.Maxretry");
          g_application.m_iMasterLockRetriesRemaining     = g_stSettings.m_iMasterLockMaxRetry;
          g_stSettings.m_iMasterLockEnableShutdown        = iStateSD;
          g_stSettings.m_iMasterLockProtectShares         = iStatePS;
          g_stSettings.m_iMasterLockStartupLock           = iStateSL;
          g_stSettings.m_iMasterLockMode                  = g_guiSettings.GetInt("Masterlock.Mastermode");
          g_stSettings.m_masterLockCode                   = g_guiSettings.GetString("Masterlock.Mastercode");

          g_stSettings.m_iMasterLockFilemanager           = iLFState;
          g_stSettings.m_iMasterLockSettings              = iLSState;
          g_stSettings.m_iMasterLockHomeMedia             = g_guiSettings.GetInt("Masterlock.LockHomeMedia");
          
          // Set Master Lock Changes to the XML
          g_settings.UpDateXbmcXML("masterlock", "mastermode",      csMMode);
          g_settings.UpDateXbmcXML("masterlock", "mastercode",      csMCode);
          g_settings.UpDateXbmcXML("masterlock", "maxretry",        csMRetry);
          g_settings.UpDateXbmcXML("masterlock", "enableshutdown",  csStateSD);
          g_settings.UpDateXbmcXML("masterlock", "protectshares",   csStatePS);
          g_settings.UpDateXbmcXML("masterlock", "startuplock",     csStateSL);
          g_settings.UpDateXbmcXML("masterlock", "LockFilemanager", csLFState);
          g_settings.UpDateXbmcXML("masterlock", "LockSettings",    csLSState);
          g_settings.UpDateXbmcXML("masterlock", "LockHomeMedia",   cLbl[g_stSettings.m_iMasterLockHomeMedia]);
          
          // PopUp OK and Display the Changed MasterCode!
          CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          if (!dlg) return ;
          dlg->SetHeading( g_localizeStrings.Get(12360));
          dlg->SetLine( 0, g_localizeStrings.Get(12366));
          dlg->SetLine( 1, csMCode);
          dlg->SetLine( 2, "");
          dlg->DoModal( m_gWindowManager.GetActiveWindow());
        }
      }
      else
      {
        // PopUp OK and Display: Master Code is not Valid or is empty or not set!
        CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        if (!dlg) return ;
        dlg->SetHeading( g_localizeStrings.Get(12360));
        dlg->SetLine( 0, g_localizeStrings.Get(12367));
        dlg->SetLine( 1, g_localizeStrings.Get(12368));
        dlg->SetLine( 2, "");
        dlg->DoModal( m_gWindowManager.GetActiveWindow() );
      }
    }
    else
    {
      // PopUp OK and Display: MasterLock mode has changed but no no Mastercode has been set!
      CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (!dlg) return ;
      dlg->SetHeading( g_localizeStrings.Get(12360));
      dlg->SetLine( 0, g_localizeStrings.Get(12370));
      dlg->SetLine( 1, g_localizeStrings.Get(12371));
      dlg->SetLine( 2, "");
      dlg->DoModal( m_gWindowManager.GetActiveWindow() );
    }
  }
  else if (strSetting == "LookAndFeel.StartUpWindow")
  {
    // Set the Current XML or Previos StartWindow state!
    CStdString strStWin;
    int iCurState = g_guiSettings.GetInt("LookAndFeel.StartUpWindow");
    if (iCurState !=0)  // 0 if Manual Settings, don't tuch the StartWindow Stuff!
    {
      switch (iCurState)
        {
          case 1:                      
            strStWin = "0";     // 0 XBMC Home           
            break;                     
          case 2:                      
            strStWin = "1";     // 1 My Programs           
            break;
          case 3:                
            strStWin = "2";     // 2 My Pictures     
            break;               
          case 4:                
            strStWin = "501";   // 501 My Musik  
            break;               
          case 5:                
            strStWin = "6";     // 6 My Video    
            break;  
        }
      g_settings.UpDateXbmcXML("startwindow", strStWin);
    }
  }
  UpdateSettings();
}
void CGUIWindowSettingsCategory::FreeControls()
{
  // free any created controls
  for (unsigned int i = 0; i < m_vecSections.size(); i++)
  {
    CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START_BUTTONS + i);
    Remove(CONTROL_START_BUTTONS + i);
    if (pControl)
    {
      pControl->FreeResources();
      delete pControl;
    }
  }
  m_vecSections.clear();
  FreeSettingsControls();
}

void CGUIWindowSettingsCategory::FreeSettingsControls()
{
  // remove the settings group
  if (m_vecGroups.size() > 1)
    m_vecGroups.erase(m_vecGroups.begin() + 1);
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START_CONTROL + i);
    Remove(CONTROL_START_CONTROL + i);
    if (pControl)
    {
      pControl->FreeResources();
      delete pControl;
    }
    delete m_vecSettings[i];
  }
  m_vecSettings.clear();
}

void CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, int iPosX, int &iPosY, int iGap, int iWidth, int &iControlID)
{
  CBaseSettingControl *pSettingControl = NULL;
  CGUIControl *pControl = NULL;
  if (pSetting->GetControlType() == CHECKMARK_CONTROL)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
    iPosY += iGap;
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    ((CGUISpinControlEx *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(iWidth);
    pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
    iPosY += iGap;
  }
  else if (pSetting->GetControlType() == SEPARATOR_CONTROL && m_pOriginalImage)
  {
    pControl = new CGUIImage(*m_pOriginalImage);
    if (!pControl) return;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    pSettingControl = new CSeparatorSettingControl((CGUIImage *)pControl, iControlID, pSetting);
    iPosY += pControl->GetHeight();
  }
  else if (pSetting->GetControlType() != SEPARATOR_CONTROL) // button control
  {
    pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    ((CGUIButtonControl *)pControl)->SetTextAlign(XBFONT_CENTER_Y);
    ((CGUIButtonControl *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(iWidth);
    pSettingControl = new CButtonSettingControl((CGUIButtonControl *)pControl, iControlID, pSetting);
    iPosY += iGap;
  }
  if (!pControl) return;
  pControl->SetNavigation(iControlID - 1,
                          iControlID + 1,
                          CONTROL_START_BUTTONS,
                          CONTROL_START_BUTTONS);
  pControl->SetID(iControlID++);
  pControl->SetGroup(CONTROL_GROUP_SETTINGS);
  pControl->SetVisible(true);
  Add(pControl);
  pControl->AllocResources();
  m_vecSettings.push_back(pSettingControl);
}

void CGUIWindowSettingsCategory::Render()
{
  // check if we need to set a new resolution
  if (m_dwResTime && timeGetTime() >= m_dwResTime)
  {
    m_dwResTime = 0;

    if (m_NewResolution != m_OldResolution)
    {
      unsigned iCtrlID = GetFocusedControl();
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iCtrlID, 0, 0, NULL);
      g_graphicsContext.SendMessage(msg);
      //set our option
      g_guiSettings.SetInt("LookAndFeel.Resolution", m_NewResolution);
      m_OldResolution = m_NewResolution;
      //set the gui resolution, if newRes is AUTORES newRes will be set to the highest available resolution
      g_graphicsContext.SetGUIResolution(m_NewResolution);
      //set our lookandfeelres to the resolution set in graphiccontext
      g_guiSettings.m_LookAndFeelResolution = m_NewResolution;
      g_application.LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));
      m_gWindowManager.ActivateWindow(GetID());
      SET_CONTROL_FOCUS(iCtrlID, g_guiSettings.GetInt("LookAndFeel.Resolution"));
    }
  }
  // update realtime changeable stuff
  UpdateRealTimeSettings();
  // update alpha status of current button
  bool bAlphaFaded = false;
  CGUIButtonControl *pButton = (CGUIButtonControl *)GetControl(CONTROL_START_BUTTONS + m_iSection);
  if (pButton && !pButton->HasFocus())
  {
    pButton->SetFocus(true);
    pButton->SetAlpha(0x80);
    bAlphaFaded = true;
  }
  CGUIWindow::Render();
  if (bAlphaFaded)
  {
    pButton->SetFocus(false);
    pButton->SetAlpha(0xFF);
  }
  // render the error message if necessary
  if (m_strErrorMessage.size())
  {
    CGUIFont *pFont = g_fontManager.GetFont("font13");
    if (pFont)
    {
      float fPosY = g_graphicsContext.GetHeight() * 0.8f;
      float fPosX = g_graphicsContext.GetWidth() * 0.5f;
      pFont->DrawText(fPosX, fPosY, 0xFFFFFFFF, m_strErrorMessage.c_str(), XBFONT_CENTER_X);
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
  if (m_iNetworkAssignment != g_guiSettings.GetInt("Network.Assignment") ||
      (m_iNetworkAssignment == NETWORK_STATIC && (
         m_strNetworkIPAddress != g_guiSettings.GetString("Network.IPAddress") ||
         m_strNetworkSubnet != g_guiSettings.GetString("Network.Subnet") ||
         m_strNetworkGateway != g_guiSettings.GetString("Network.Gateway") ||
         m_strNetworkDNS != g_guiSettings.GetString("Network.DNS"))))
  { // our network settings have changed - we should prompt the user to reset XBMC
    CGUIDialogYesNo *dlg = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!dlg) return ;
    dlg->SetHeading( 14038 );
    dlg->SetLine( 0, 14039 );
    dlg->SetLine( 1, 14040 );
    dlg->SetLine( 2, L"" );
    dlg->DoModal( m_gWindowManager.GetActiveWindow() );
    if (dlg->IsConfirmed())
    { // reset settings
      g_applicationMessenger.RestartApp();
    }
    // update our settings variables
    m_iNetworkAssignment = g_guiSettings.GetInt("Network.Assignment");
    m_strNetworkIPAddress = g_guiSettings.GetString("Network.IPAddress");
    m_strNetworkSubnet = g_guiSettings.GetString("Network.Subnet");
    m_strNetworkGateway = g_guiSettings.GetString("Network.Gateway");
    m_strNetworkDNS = g_guiSettings.GetString("Network.DNS");
    // replace settings
    /*   g_guiSettings.SetInt("Network.Assignment", m_iNetworkAssignment);
       g_guiSettings.SetString("Network.IPAddress", m_strNetworkIPAddress);
       g_guiSettings.SetString("Network.Subnet", m_strNetworkSubnet);
       g_guiSettings.SetString("Network.Gateway", m_strNetworkGateway);
       g_guiSettings.SetString("Network.DNS", m_strNetworkDNS);*/
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
      pControl->AddLabel(strLabel, (i - pSettingInt->m_iMin) / pSettingInt->m_iStep);
    }
    pControl->SetValue((pSettingInt->GetData() - pSettingInt->m_iMin) / pSettingInt->m_iStep);
  }
  else
  {
    if (g_guiSettings.GetString("Subtitles.Font").size())
    {
      //find font sizes...
      CHDDirectory directory;
      CFileItemList items;
      CStdString strPath = "Q:\\system\\players\\mplayer\\font\\";
      /*     if(g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
            {
              strPath = "Q:\\mplayer\\font\\";
            }*/
      strPath += g_guiSettings.GetString("Subtitles.Font");
      strPath += "\\";
      directory.GetDirectory(strPath, items);
      int iCurrentSize = 0;
      int iSize = 0;
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItem* pItem = items[i];
        if (pItem->m_bIsFolder)
        {
          if (strcmpi(pItem->GetLabel().c_str(), "CVS") == 0) continue;
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
    CStdString strPath = "Q:\\system\\players\\mplayer\\font\\";
    /*   if(g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
        {
          strPath = "Q:\\mplayer\\font\\";
        }*/
    directory.GetDirectory(strPath, items);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItem* pItem = items[i];
      if (pItem->m_bIsFolder)
      {
        if (strcmpi(pItem->GetLabel().c_str(), "CVS") == 0) continue;
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
    CStdString strPath = "Q:\\media\\fonts\\";
    if (directory.GetDirectory(strPath, items))
    {
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItem* pItem = items[i];

        if (!pItem->m_bIsFolder)
        {
          char* extension = CUtil::GetExtension(pItem->GetLabel().c_str());
          if (stricmp(extension, ".ttf") != 0) continue;
          if (strcmpi(pItem->GetLabel().c_str(), pSettingString->GetData().c_str()) == 0)
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

  int iCurrentSkinFontSet = 0;
  int iSkinFontSet = 0;

  m_strNewSkinFontSet.Empty();

  RESOLUTION res;
  CStdString strPath = g_SkinInfo.GetSkinPath("font.xml", &res);

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
        if (idAttr != NULL)
        {
          if (strcmpi(idAttr, g_guiSettings.GetString("LookAndFeel.Font").c_str()) == 0)
          {
            iCurrentSkinFontSet = iSkinFontSet;
          }
          pControl->AddLabel(idAttr, iSkinFontSet++);
        }
        pChild = pChild->NextSibling();
      }
    }

    pControl->SetValue(iCurrentSkinFontSet);
  }
  else
  {
    // Since no fontset is defined, there is no selection of a fontset, so disable the component
    pControl->AddLabel("Default", 1);
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
  CStdString strPath = "Q:\\skin\\";
  directory.GetDirectory(strPath, items);

  int iCurrentSkin = 0;
  int iSkin = 0;
  vector<CStdString> vecSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), "CVS") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      //   if (g_SkinInfo.Check(pItem->m_strPath))
      //   {
      vecSkins.push_back(pItem->GetLabel());
      //   }
    }
  }

  sort(vecSkins.begin(), vecSkins.end(), sortstringbyname());
  for (i = 0; i < (int) vecSkins.size(); ++i)
  {
    CStdString strSkin = vecSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("LookAndFeel.Skin").c_str()) == 0)
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
  CStdString strPath = "Q:\\sounds\\";
  CDirectory::GetDirectory(strPath, items);

  int iCurrentSoundSkin = 0;
  int iSoundSkin = 0;
  vector<CStdString> vecSoundSkins;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), "CVS") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecSoundSkins.push_back(pItem->GetLabel());
    }
  }

  pControl->AddLabel(g_localizeStrings.Get(474), iSoundSkin++); // Off
  pControl->AddLabel(g_localizeStrings.Get(15109), iSoundSkin++); // Skin Default

  if (g_guiSettings.GetString("LookAndFeel.SoundSkin")=="SKINDEFAULT")
    iCurrentSoundSkin=1;

  sort(vecSoundSkins.begin(), vecSoundSkins.end(), sortstringbyname());
  for (i = 0; i < (int) vecSoundSkins.size(); ++i)
  {
    CStdString strSkin = vecSoundSkins[i];
    if (strcmpi(strSkin.c_str(), g_guiSettings.GetString("LookAndFeel.SoundSkin").c_str()) == 0)
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
  CStdString& strCurrentCharsetLabel = g_charsetConverter.getCharsetLabelByName(pSettingString->GetData());

  sort(vecCharsets.begin(), vecCharsets.end(), sortstringbyname());

  for (int i = 0; i < (int) vecCharsets.size(); ++i)
  {
    CStdString strCharsetLabel = vecCharsets[i];

    if (strCharsetLabel == strCurrentCharsetLabel)
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
  CStdString strPath = "Q:\\visualisations\\";
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
  if ( !xmlDoc.LoadFile( "Q:\\voicemasks.xml" ) ) return ;
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
  // Get the voice mask spin control values
  CStdString strEnergy, strPitch, strWhisper, strRobotic;
  strEnergy.Format("VoiceOnPort%i.fSpecEnergyWeight", (int) dwPort);
  strPitch.Format("VoiceOnPort%i.fPitchScale", (int) dwPort);
  strWhisper.Format("VoiceOnPort%i.fWhisperValue", (int) dwPort);
  strRobotic.Format("VoiceOnPort%i.fRoboticValue", (int) dwPort);

  float fEnergy = g_guiSettings.GetFloat(strEnergy);
  float fPitch = g_guiSettings.GetFloat(strPitch);
  float fWhisper = g_guiSettings.GetFloat(strWhisper);
  float fRobotic = g_guiSettings.GetFloat(strRobotic);

  //Get the voice mask bool values

  CStdString strBoolEnergy, strBoolPitch, strBoolWhisper, strBoolRobotic;
  strBoolEnergy.Format("VoiceOnPort%i.EnablefSpecEnergyWeight", (int) dwPort);
  strBoolPitch.Format("VoiceOnPort%i.EnablefPitchScale", (int) dwPort);
  strBoolWhisper.Format("VoiceOnPort%i.EnablefWhisperValue", (int) dwPort);
  strBoolRobotic.Format("VoiceOnPort%i.EnablefRoboticValue", (int) dwPort);

  bool bEnergy = g_guiSettings.GetBool(strBoolEnergy);
  bool bPitch = g_guiSettings.GetBool(strBoolPitch);
  bool bWhisper = g_guiSettings.GetBool(strBoolWhisper);
  bool bRobotic = g_guiSettings.GetBool(strBoolRobotic);

  if (strCurMask.CompareNoCase("None") == 0 || strCurMask.CompareNoCase("Custom") == 0 )
  {
    g_guiSettings.SetFloat(strEnergy, 0.0f);
    g_guiSettings.SetFloat(strPitch, 0.0f);
    g_guiSettings.SetFloat(strWhisper, 0.0f);
    g_guiSettings.SetFloat(strRobotic, 0.0f);

    g_guiSettings.SetBool(strBoolEnergy, false);
    g_guiSettings.SetBool(strBoolPitch, false);
    g_guiSettings.SetBool(strBoolWhisper, false);
    g_guiSettings.SetBool(strBoolRobotic, false);
    return ;
  }

  //find mask values in xml...
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( "Q:\\voicemasks.xml" ) ) return ;
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
                fEnergy = (float) atof(strName.c_str());
                if (fEnergy > 0)
                  bEnergy = true;
                else
                {
                  bEnergy = false;
                  fEnergy = 0.0f;
                }
              }
            }
            else if (strValue.CompareNoCase("fPitchScale") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                fPitch = (float) atof(strName.c_str());
                if (fPitch > 0)
                  bPitch = true;
                else
                {
                  bPitch = false;
                  fPitch = 0.0f;
                }
              }
            }
            else if (strValue.CompareNoCase("fWhisperValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                fWhisper = (float) atof(strName.c_str());
                if (fWhisper > 0)
                  bWhisper = true;
                else
                {
                  bWhisper = false;
                  fWhisper = 0.0f;
                }
              }
            }
            else if (strValue.CompareNoCase("fRoboticValue") == 0)
            {
              if (pChild->FirstChild())
              {
                CStdString strName = pChild->FirstChild()->Value();
                fRobotic = (float) atof(strName.c_str());
                if (fRobotic > 0)
                  bRobotic = true;
                else
                {
                  bRobotic = false;
                  fRobotic = 0.0f;
                }
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

  g_guiSettings.SetFloat(strEnergy, fEnergy);
  g_guiSettings.SetFloat(strPitch, fPitch);
  g_guiSettings.SetFloat(strWhisper, fWhisper);
  g_guiSettings.SetFloat(strRobotic, fRobotic);

  g_guiSettings.SetBool(strBoolEnergy, bEnergy);
  g_guiSettings.SetBool(strBoolPitch, bPitch);
  g_guiSettings.SetBool(strBoolWhisper, bWhisper);
  g_guiSettings.SetBool(strBoolRobotic, bRobotic);

}
void CGUIWindowSettingsCategory::FillInResolutions(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  // Find the valid resolutions and add them as necessary
  vector<RESOLUTION> res;
  g_graphicsContext.GetAllowedResolutions(res);
  int iCurrentRes = 0;
  int iLabel = 0;
  for (vector<RESOLUTION>::iterator it = res.begin(); it != res.end();it++)
  {
    RESOLUTION res = *it;
    if (res == AUTORES)
    {
      pControl->AddLabel(g_localizeStrings.Get(14061), res);
    }
    else
    {
      pControl->AddLabel(g_settings.m_ResInfo[res].strMode, res);
    }
    if (pSettingInt->GetData() == res)
      iCurrentRes = iLabel;
    iLabel++;
  }
  pControl->SetValue(iCurrentRes);
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

  CStdString strPath = "Q:\\language\\";
  directory.GetDirectory(strPath, items);

  int iCurrentLang = 0;
  int iLanguage = 0;
  vector<CStdString> vecLanguage;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (strcmpi(pItem->GetLabel().c_str(), "CVS") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "fonts") == 0) continue;
      if (strcmpi(pItem->GetLabel().c_str(), "media") == 0) continue;
      vecLanguage.push_back(pItem->GetLabel());
    }
  }

  sort(vecLanguage.begin(), vecLanguage.end(), sortstringbyname());
  for (i = 0; i < (int) vecLanguage.size(); ++i)
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

  //find screensavers ....
  CHDDirectory directory;
  CFileItemList items;
  CStdString strPath = "Q:\\screensavers\\";
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
  for (i = 0; i < (int) vecScr.size(); ++i)
  {
    CStdString strScr = vecScr[i];

    if (strcmpi(strScr.c_str(), strDefaultScr.c_str()) == 0)
      iCurrentScr = i + 3;

    pControl->AddLabel(strScr, i + 3);
  }

  // if we can't find the screensaver previously configured
  // then fallback to turning the screensaver off.
  if (iCurrentScr < 0)
  {
    if (strDefaultScr == "Dim")
      iCurrentScr = 1;
    else if (strDefaultScr == "Black")
      iCurrentScr = 2;
    else
    {
      iCurrentScr = 0;
      pSettingString->SetData("None");
    }
  }
  pControl->SetValue(iCurrentScr);
}

bool CGUIWindowSettingsCategory::CheckMasterLockCode()
{
  // GeminiServer
  // prompt user for mastercode if the mastercode was set b4 or by xml
  // prompt user for mastercode when changing lock settings
  if (g_passwordManager.IsMasterLockLocked(true))  return true;
  else return false;
}
void CGUIWindowSettingsCategory::FillInFTPServerUser(CSetting *pSetting)
{
  //GeminiServer
  CSettingString *pSettingString = (CSettingString*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->SetType(SPIN_CONTROL_TYPE_TEXT);
  pControl->Clear();
  pControl->SetShowRange(true);
  int iDefaultFtpUser = 0;

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
    g_guiSettings.SetString("Servers.FTPServerUser", strFtpUser1.c_str());
    pControl->Update();
  }
}
bool CGUIWindowSettingsCategory::SetFTPServerUserPass()
{
  // Get GUI USER and pass and set pass to FTP Server
    CStdString strFtpUserName, strFtpUserPassword;
    strFtpUserName      = g_guiSettings.GetString("Servers.FTPServerUser");
    strFtpUserPassword  = g_guiSettings.GetString("Servers.FTPServerPassword");
    if(strFtpUserPassword.size()!=0)
    {
      if (CUtil::SetFTPServerUserPassword(strFtpUserName, strFtpUserPassword))
      {
          // todo! ERROR check! if something goes wrong on SetPW!
          // PopUp OK and Display: FTP Server Password was set succesfull!
          CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          if (!dlg) return false;
          dlg->SetHeading( g_localizeStrings.Get(728));
          dlg->SetLine( 0, "");
          dlg->SetLine( 1, g_localizeStrings.Get(1247));
          dlg->SetLine( 2, "");
          dlg->DoModal( m_gWindowManager.GetActiveWindow() );
      }
      return true;
    }
    else
    {
          // PopUp OK and Display: FTP Server Password is empty! Try Again!
          CGUIDialogOK *dlg = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
          if (!dlg) return false;
          dlg->SetHeading( g_localizeStrings.Get(728));
          dlg->SetLine( 0, "");
          dlg->SetLine( 1, g_localizeStrings.Get(12358));
          dlg->SetLine( 2, "");
          dlg->DoModal( m_gWindowManager.GetActiveWindow() );
    }
    return true;
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

void CGUIWindowSettingsCategory::JumpToSection(DWORD dwWindowId, int iSection)
{
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSectionBeforeJump=m_iSection;
  m_iControlBeforeJump=m_iLastControl;
  m_iWindowBeforeJump=m_dwWindowId+m_iScreen;
  m_iSection=iSection;
  m_iLastControl=CONTROL_START_CONTROL;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, dwWindowId);
  OnMessage(msg1);
  for (unsigned int i=0; i<m_vecSections.size(); ++i)
  {
    CONTROL_DISABLE(CONTROL_START_BUTTONS+i)
  }
}

void CGUIWindowSettingsCategory::JumpToPreviousSection()
{
  for (unsigned int i=0; i<m_vecSections.size(); ++i)
  {
    CONTROL_DISABLE(CONTROL_START_BUTTONS+i)
  }
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT, 0, 0, 0, 0);
  OnMessage(msg);
  m_iSection=m_iSectionBeforeJump;
  m_iLastControl=m_iControlBeforeJump;
  CGUIMessage msg1(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, m_iWindowBeforeJump);
  OnMessage(msg1);

  m_iSectionBeforeJump=-1;
  m_iControlBeforeJump=-1;
  m_iWindowBeforeJump=WINDOW_INVALID;
}
