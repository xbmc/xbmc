#include "stdafx.h"
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "FileSystem/HDDirectory.h"
#include "Util.h"
#include "GUILabelControl.h"
#include "GUICheckMarkControl.h"
#include "Utils/Weather.h"
#include "MusicDatabase.h"
#include "ProgramDatabase.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include "Utils/LED.h"
#include "Utils/LCDFactory.h"
#include "Utils/FanController.h"
#include "PlayListPlayer.h"
#include "SkinInfo.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "lib/libscrobbler/scrobbler.h"
#include "GUIPassword.h"
#include "utils/GUIInfoManager.h"
#include <xfont.h>
#include "GUIDialogGamepad.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "GUIFontManager.h"
#include "GUIDialogContextMenu.h"
#include "MediaManager.h"
#include "xbox/network.h"

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
    : CGUIWindow(WINDOW_SETTINGS_MYPICTURES, "SettingsCategory.xml")
{
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
  m_strErrorMessage = "";
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
  case GUI_MSG_SETFOCUS:
    {
      unsigned int iControl = message.GetControlId();
      unsigned int iSender = message.GetSenderId();
      // if both the sender and the control are within out category range, or the sender is the window
      // then we have a change of category.
      if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size() &&
          ((iSender >= CONTROL_START_BUTTONS && iSender < CONTROL_START_BUTTONS + m_vecSections.size()) || iSender == GetID()))
      {
        // change the setting...
        if (iControl - CONTROL_START_BUTTONS != m_iSection)
        {
          m_iSection = iControl - CONTROL_START_BUTTONS;
          CheckNetworkSettings();

          if(!g_advancedSettings.bUseMasterLockAdvancedXml)
            g_passwordManager.CheckMasterLock(false);
          CreateSettings();
        }
      }
    }
    break;
  case GUI_MSG_LOAD_SKIN:
    {
      // Do we need to reload the language file
      if (!m_strNewLanguage.IsEmpty())
      {
        g_guiSettings.SetString("LookAndFeel.Language", m_strNewLanguage);
        g_settings.Save();

        CStdString strLangInfoPath;
        strLangInfoPath.Format("Q:\\language\\%s\\langinfo.xml", m_strNewLanguage.c_str());
        g_langInfo.Load(strLangInfoPath);

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
        g_localizeStrings.Load(strLanguagePath);
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

      // Reload a skin theme
      if (!m_strNewSkinTheme.IsEmpty())
      {
        g_guiSettings.SetString("LookAndFeel.SkinTheme", m_strNewSkinTheme);
        g_settings.Save();
      }

      // Reload a resolution
      if (m_NewResolution != INVALID)
      {
        g_guiSettings.SetInt("LookAndFeel.Resolution", m_NewResolution);
        //set the gui resolution, if newRes is AUTORES newRes will be set to the highest available resolution
        g_graphicsContext.SetGUIResolution(m_NewResolution);
        //set our lookandfeelres to the resolution set in graphiccontext
        g_guiSettings.m_LookAndFeelResolution = m_NewResolution;
      }
      // Reload the skin.  Save the current focused control, and refocus it
      // when done.
      unsigned iCtrlID = GetFocusedControl();
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iCtrlID, 0, 0, NULL);
      g_graphicsContext.SendMessage(msg);
      g_application.LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));
      SET_CONTROL_FOCUS(iCtrlID, 0);
      CGUIMessage msgSelect(GUI_MSG_ITEM_SELECT, GetID(), iCtrlID, msg.GetParam1(), msg.GetParam2());
      OnMessage(msgSelect);
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      if (message.GetParam1() != WINDOW_INVALID)
      { // coming to this window first time (ie not returning back from some other window)
        // so we reset our section and control states
        m_iSection = 0;
        ResetControlStates();
      }
      m_iScreen = (int)message.GetParam2() - (int)m_dwWindowId;
      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
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
      if(!g_advancedSettings.bUseMasterLockAdvancedXml)
        g_passwordManager.CheckMasterLock(true);
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
  CControlGroup group(0);
  group.m_lastControl = CONTROL_START_BUTTONS + m_iSection;
  m_vecGroups.push_back(group);
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
    pButton->SetLabel(g_localizeStrings.Get(m_vecSections[i]->m_dwLabelID));
    pButton->SetID(CONTROL_START_BUTTONS + i);
    pButton->SetGroup(CONTROL_GROUP_BUTTONS);
    pButton->SetPosition(pButtonArea->GetXPosition(), pButtonArea->GetYPosition() + i*pControlGap->GetHeight());
    pButton->SetNavigation(CONTROL_START_BUTTONS + (int)i - 1, CONTROL_START_BUTTONS + i + 1, CONTROL_START_CONTROL, CONTROL_START_CONTROL);
    pButton->SetVisible(true);
    pButton->AllocResources();
    Insert(pButton, m_pOriginalSettingsButton);
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
  m_vecGroups.push_back(CControlGroup(1)); // add the control group
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
    if (strSetting.Equals("Pictures.AutoSwitchMethod") || strSetting.Equals("ProgramFiles.AutoSwitchMethod") || strSetting.Equals("MusicFiles.AutoSwitchMethod") || strSetting.Equals("VideoFiles.AutoSwitchMethod"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        pControl->AddLabel(g_localizeStrings.Get(14015 + i), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("MyPrograms.NTSCMode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
      {
        pControl->AddLabel(g_localizeStrings.Get(16106 + i), i);
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("MyMusic.Visualisation"))
    {
      FillInVisualisations(pSetting, GetSetting(pSetting->GetSetting())->GetID());
    }
    else if (strSetting.Equals("Karaoke.Port0VoiceMask"))
    {
      FillInVoiceMasks(0, pSetting);
    }
    else if (strSetting.Equals("Karaoke.Port1VoiceMask"))
    {
      FillInVoiceMasks(1, pSetting);
    }
    else if (strSetting.Equals("Karaoke.Port2VoiceMask"))
    {
      FillInVoiceMasks(2, pSetting);
    }
    else if (strSetting.Equals("Karaoke.Port3VoiceMask"))
    {
      FillInVoiceMasks(3, pSetting);
    }
    else if (strSetting.Equals("AudioOutput.Mode"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(338), AUDIO_ANALOG);
      if (g_audioConfig.HasDigitalOutput())
        pControl->AddLabel(g_localizeStrings.Get(339), AUDIO_DIGITAL);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("CDDARipper.Encoder"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("Lame", CDDARIP_ENCODER_LAME);
      pControl->AddLabel("Vorbis", CDDARIP_ENCODER_VORBIS);
      pControl->AddLabel("Wav", CDDARIP_ENCODER_WAV);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("CDDARipper.Quality"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(604), CDDARIP_QUALITY_CBR);
      pControl->AddLabel(g_localizeStrings.Get(601), CDDARIP_QUALITY_MEDIUM);
      pControl->AddLabel(g_localizeStrings.Get(602), CDDARIP_QUALITY_STANDARD);
      pControl->AddLabel(g_localizeStrings.Get(603), CDDARIP_QUALITY_EXTREME);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("LCD.Type"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), LCD_TYPE_NONE);
      pControl->AddLabel("LCD - HD44780", LCD_TYPE_LCD_HD44780);
      pControl->AddLabel("LCD - KS0073", LCD_TYPE_LCD_KS0073);
      pControl->AddLabel("VFD", LCD_TYPE_VFD);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("LCD.ModChip"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel("SmartXX", MODCHIP_SMARTXX);
      pControl->AddLabel("Xenium", MODCHIP_XENIUM);
      pControl->AddLabel("Xecuter3", MODCHIP_XECUTER3);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("System.TargetTemperature"))
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
    else if (strSetting.Equals("System.FanSpeed"))
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
      pControl->SetValue(int(pSettingInt->GetData()));
    }
    else if (strSetting.Equals("System.RemotePlayHDSpinDown"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(474), SPIN_DOWN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(475), SPIN_DOWN_MUSIC);
      pControl->AddLabel(g_localizeStrings.Get(13002), SPIN_DOWN_VIDEO);
      pControl->AddLabel(g_localizeStrings.Get(476), SPIN_DOWN_BOTH);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("Servers.WebServerPassword"))
    { // get password from the webserver if it's running (and update our settings)
      if (g_application.m_pWebServer)
      {
        ((CSettingString *)GetSetting(strSetting)->GetSetting())->SetData(g_application.m_pWebServer->GetPassword());
        g_settings.Save();
      }
    }
    else if (strSetting.Equals("Network.Assignment"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(718), NETWORK_DASH);
      pControl->AddLabel(g_localizeStrings.Get(716), NETWORK_DHCP);
      pControl->AddLabel(g_localizeStrings.Get(717), NETWORK_STATIC);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("Subtitles.Style"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(738), XFONT_NORMAL);
      pControl->AddLabel(g_localizeStrings.Get(739), XFONT_BOLD);
      pControl->AddLabel(g_localizeStrings.Get(740), XFONT_ITALICS);
      pControl->AddLabel(g_localizeStrings.Get(741), XFONT_BOLDITALICS);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("Subtitles.Color"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      for (int i = SUBTITLE_COLOR_START; i <= SUBTITLE_COLOR_END; i++)
        pControl->AddLabel(g_localizeStrings.Get(760 + i), i);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("Subtitles.Height"))
    {
      FillInSubtitleHeights(pSetting);
    }
    else if (strSetting.Equals("Subtitles.Font"))
    {
      FillInSubtitleFonts(pSetting);
    }
    else if (strSetting.Equals("Subtitles.CharSet") || strSetting.Equals("LookAndFeel.CharSet"))
    {
      FillInCharSets(pSetting);
    }
    else if (strSetting.Equals("LookAndFeel.Font"))
    {
      FillInSkinFonts(pSetting);
    }
    else if (strSetting.Equals("LookAndFeel.Skin"))
    {
      FillInSkins(pSetting);
    }
    else if (strSetting.Equals("LookAndFeel.SoundSkin"))
    {
      FillInSoundSkins(pSetting);
    }
    else if (strSetting.Equals("LookAndFeel.Language"))
    {
      FillInLanguages(pSetting);
    }
    else if (strSetting.Equals("LookAndFeel.Rumble"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351),  0 );  // Off!
      pControl->AddLabel("0.1",     1	); pControl->AddLabel("0.2",     2	);  
      pControl->AddLabel("0.3",     3	); pControl->AddLabel("0.4",     4 );  
      pControl->AddLabel("0.5",     5 ); pControl->AddLabel("0.6",     6 );
      pControl->AddLabel("0.7",     7 ); pControl->AddLabel("0.8",     8 );
      pControl->AddLabel("0.9",     9 ); pControl->AddLabel("1.0",     10 );
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("LookAndFeel.Resolution"))
    {
      FillInResolutions(pSetting);
    }
    else if (strSetting.Equals("LookAndFeel.SkinTheme"))
    {
      // GeminiServer Skin Theme
      FillInSkinThemes(pSetting);
    }
    else if (strSetting.Equals("ScreenSaver.Mode"))
    {
      FillInScreenSavers(pSetting);
    }
    else if (strSetting.Equals("VideoPlayer.DisplayResolution"))
    {
      FillInResolutions(pSetting);
    }
    else if (strSetting.Equals("VideoPlayer.BypassCDSelection"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13170), 0); // "Never" = disabled
      pControl->AddLabel(g_localizeStrings.Get(13171), 1); // "Immediatly" = skip it completely
      for (int i = 2; i <= 37; i++)
      {
        CStdString strText;
        strText.Format((CStdString)g_localizeStrings.Get(13172), (i-1)*5); 
        pControl->AddLabel(strText, i); // "After 5-180 secs"
      }
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("VideoPlayer.FrameRateConversions"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(231), FRAME_RATE_LEAVE_AS_IS); // "None"
      pControl->AddLabel(g_videoConfig.HasPAL() ? g_localizeStrings.Get(12380) : g_localizeStrings.Get(12381), FRAME_RATE_CONVERT); // "Play PAL videos at NTSC rates" or "Play NTSC videos at PAL rates"
      if (g_videoConfig.HasPAL60())
        pControl->AddLabel(g_localizeStrings.Get(12382), FRAME_RATE_USE_PAL60); // "Play NTSC videos in PAL60"
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("LED.Colour"))
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
    else if (strSetting.Equals("LED.DisableOnPlayback"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(106), LED_PLAYBACK_OFF);     // No
      pControl->AddLabel(g_localizeStrings.Get(13002), LED_PLAYBACK_VIDEO);   // Video Only
      pControl->AddLabel(g_localizeStrings.Get(475), LED_PLAYBACK_MUSIC);    // Music Only
      pControl->AddLabel(g_localizeStrings.Get(476), LED_PLAYBACK_VIDEO_MUSIC); // Video & Music
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("VideoPlayer.RenderMethod"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(13355), RENDER_LQ_RGB_SHADER);
      pControl->AddLabel(g_localizeStrings.Get(13356), RENDER_OVERLAYS);
      pControl->AddLabel(g_localizeStrings.Get(13357), RENDER_HQ_RGB_SHADER);
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("MusicPlayer.ReplayGainType"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351), REPLAY_GAIN_NONE);
      pControl->AddLabel(g_localizeStrings.Get(639), REPLAY_GAIN_TRACK);
      pControl->AddLabel(g_localizeStrings.Get(640), REPLAY_GAIN_ALBUM);
      pControl->SetValue(pSettingInt->GetData());
    }
    /*else if (strSetting.Equals("Masterlock.Mastermode"))
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
    else if (strSetting.Equals("Masterlock.Maxretry"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      CStdString cLbl[10]= {"0","1","2","3","4","5","6","7","8","9"};
      pControl->AddLabel(g_localizeStrings.Get(1223), 0);   //Disabled
      for (unsigned int i = 1; i <= 9; i++)  pControl->AddLabel(cLbl[i], i);
      pControl->SetValue(pSettingInt->GetData());
    }
    */
    else if (strSetting.Equals("LookAndFeel.StartUpWindow"))
    {
      FillInStartupWindow(pSetting);
    }
    else if (strSetting.Equals("LookAndFeel.Rumble"))
    {
      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(351),  0 );  // Off!
      pControl->AddLabel("0.1",     1	); pControl->AddLabel("0.2",     2	);  
      pControl->AddLabel("0.3",     3	); pControl->AddLabel("0.4",     4 );  
      pControl->AddLabel("0.5",     5 ); pControl->AddLabel("0.6",     6 );
      pControl->AddLabel("0.7",     7 ); pControl->AddLabel("0.8",     8 );
      pControl->AddLabel("0.9",     9 ); pControl->AddLabel("1.0",     10 );
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("MasterLock.LockSettingsFileManager"))

    {
      g_guiSettings.SetBool("Masterlock.Enableshutdown", g_passwordManager.bMasterLockEnableShutdown);
      g_guiSettings.SetBool("Masterlock.Protectshares", g_passwordManager.bMasterLockProtectShares);
      //g_guiSettings.SetInt("Masterlock.Maxretry", g_passwordManager.iMasterLockMaxRetry);
      g_guiSettings.SetString("Masterlock.Mastercode", g_passwordManager.strMasterLockCode);
    }
    else if (strSetting.Equals("Masterlock.LockSettingsFilemanager"))
    {
      int iTmpState;
      bool bLFState = g_passwordManager.bMasterLockFilemanager;
      bool bLSState = g_passwordManager.bMasterLockSettings;
      
      if(!bLFState && !bLSState) iTmpState = 0;
      else if(!bLFState && bLSState) iTmpState = 1;
      else if(bLFState && !bLSState) iTmpState = 2;
      else if(bLFState && bLSState ) iTmpState = 3;
      else iTmpState = 0;
      
      g_guiSettings.SetInt("Masterlock.LockSettingsFilemanager", iTmpState);

      CSettingInt *pSettingInt = (CSettingInt*)pSetting;
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
      pControl->AddLabel(g_localizeStrings.Get(1223)  , 0 );  // 0 Disabled
      pControl->AddLabel(g_localizeStrings.Get(5)     , 1 );  // 1 Settings
      pControl->AddLabel(g_localizeStrings.Get(7)     , 2 );  // 2 Filemanager
      pControl->AddLabel(g_localizeStrings.Get(12373) , 3 );  // 3 Settings and Filemanager
      pControl->SetValue(pSettingInt->GetData());
    }
    else if (strSetting.Equals("Masterlock.LockHomeMedia"))
    {
      g_guiSettings.SetInt("Masterlock.LockHomeMedia", g_passwordManager.iMasterLockHomeMedia);
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
    else if (strSetting.Equals("Servers.FTPServerUser"))
    {
      //GeminiServer
      FillInFTPServerUser(pSetting);
    }
    else if (strSetting.Equals("Autodetect.NickName"))
    {
      //GeminiServer
      //CStdString strXboxNickNameIn = g_guiSettings.GetString("Autodetect.NickName");
      CStdString strXboxNickNameOut;
      //if (CUtil::SetXBOXNickName(strXboxNickNameIn, strXboxNickNameOut))
      if (CUtil::GetXBOXNickName(strXboxNickNameOut))
        g_guiSettings.SetString("Autodetect.NickName", strXboxNickNameOut.c_str());
    }
    else if (strSetting.Equals("MyVideos.ExternalDVDPlayer"))
    {
      CSettingString *pSettingString = (CSettingString *)pSetting;
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      if (pSettingString->GetData().IsEmpty())
        pControl->SetLabel2(g_localizeStrings.Get(20009));  // TODO: localize 2.0
    }
    else if (strSetting.Equals("XBDateTime.Region"))
    {
      FillInRegions(pSetting);
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
  // check and update our control group, in case the first item(s) are disabled (otherwise
  // we can't navigate to the controls from the buttons on the left)
  for (int firstControl = CONTROL_START_CONTROL; firstControl < CONTROL_START_CONTROL + (int)m_vecSettings.size(); firstControl++)
  {
    const CGUIControl *control = GetControl(firstControl);
    if (!control->IsDisabled())
    {
      m_vecGroups[1].m_lastControl = firstControl;
      break;
    }
  }
}

void CGUIWindowSettingsCategory::UpdateSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    pSettingControl->Update();
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
    if (strSetting.Equals("Pictures.AutoSwitchUseLargeThumbs"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Pictures.UseAutoSwitching"));
    }
    else if (strSetting.Equals("ProgramFiles.AutoSwitchUseLargeThumbs") || strSetting.Equals("ProgramFiles.AutoSwitchMethod"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("ProgramFiles.UseAutoSwitching"));
    }
    else if (strSetting.Equals("ProgramFiles.AutoSwitchPercentage"))
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("ProgramFiles.UseAutoSwitching") && g_guiSettings.GetInt("ProgramFiles.AutoSwitchMethod") == 2);
    }
    else if (strSetting.Equals("MyPrograms.NTSCMode"))
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MyPrograms.GameAutoRegion"));
    }
    else if (strSetting.Equals("XLinkKai.EnableNotifications") || strSetting.Equals("XLinkKai.UserName") || strSetting.Equals("XLinkKai.Password") || strSetting.Equals("XLinkKai.Server"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("XLinkKai.Enabled"));
    }
    else if (strSetting.Equals("MusicFiles.AutoSwitchUseLargeThumbs") || strSetting.Equals("MusicFiles.AutoSwitchMethod"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MusicFiles.UseAutoSwitching"));
    }
    else if (strSetting.Equals("MusicFiles.AutoSwitchPercentage"))
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MusicFiles.UseAutoSwitching") && g_guiSettings.GetInt("MusicFiles.AutoSwitchMethod") == 2);
    }
    else if (strSetting.Equals("VideoFiles.AutoSwitchUseLargeThumbs") || strSetting.Equals("VideoFiles.AutoSwitchMethod"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VideoFiles.UseAutoSwitching"));
    }
    else if (strSetting.Equals("VideoFiles.AutoSwitchPercentage"))
    { // set visibility based on our other setting...
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VideoFiles.UseAutoSwitching") && g_guiSettings.GetInt("VideoFiles.AutoSwitchMethod") == 2);
    }
    else if (strSetting.Equals("MusicPlaylist.ClearPlaylistsOnEnd"))
    { // disable repeat and repeat one if clear playlists is enabled
      if (g_guiSettings.GetBool("MusicPlaylist.ClearPlaylistsOnEnd"))
      {
        g_playlistPlayer.Repeat(PLAYLIST_MUSIC, false);
        g_playlistPlayer.RepeatOne(PLAYLIST_MUSIC, false);
        g_stSettings.m_bMyMusicPlaylistRepeat = false;
        g_settings.Save();
      }
    } 
    else if (strSetting.Equals("CDDARipper.Quality"))
    { // only visible if we are doing non-WAV ripping
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("CDDARipper.Encoder") != CDDARIP_ENCODER_WAV);
    }
    else if (strSetting.Equals("CDDARipper.Bitrate"))
    { // only visible if we are ripping to CBR
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled((g_guiSettings.GetInt("CDDARipper.Encoder") != CDDARIP_ENCODER_WAV) &&
                                           (g_guiSettings.GetInt("CDDARipper.Quality") == CDDARIP_QUALITY_CBR));
    }
    else if (strSetting.Equals("MusicPlayer.OutputToAllSpeakers") || strSetting.Equals("AudioOutput.AC3PassThrough") || strSetting.Equals("AudioOutput.DTSPassThrough"))
    { // only visible if we are in digital mode
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL);
    }
    else if (strSetting.Equals("MusicPlayer.CrossFadeAlbumTracks"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("MusicPlayer.CrossFade") > 0);
    }
    else if (strSetting.Left(12).Equals("Karaoke.Port") || strSetting.Equals("Karaoke.Volume"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Karaoke.VoiceEnabled"));
    }
    else if (strSetting.Equals("System.FanSpeed"))
    { // only visible if we have fancontrolspeed enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("System.FanSpeedControl"));
    }
    else if (strSetting.Equals("System.TargetTemperature"))
    { // only visible if we have autotemperature enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("System.AutoTemperature"));
    }
    else if (strSetting.Equals("System.RemotePlayHDSpinDownDelay"))
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.RemotePlayHDSpinDown") != SPIN_DOWN_NONE);
    }
    else if (strSetting.Equals("System.RemotePlayHDSpinDownMinDuration"))
    { // only visible if we have spin down enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.RemotePlayHDSpinDown") != SPIN_DOWN_NONE);
    }
    else if (strSetting.Equals("System.ShutDownWhilePlaying"))
    { // only visible if we have shutdown enabled
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.ShutDownTime") != 0);
    }
    else if (strSetting.Equals("Servers.FTPServerUser") || strSetting.Equals("Servers.FTPServerPassword") || strSetting.Equals("Servers.FTPAutoFatX"))
    {
      //GeminiServer
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("Servers.FTPServer"));
    }
    else if (strSetting.Equals("Servers.WebServerPassword"))
    { // Fill in a blank pass if we don't have it
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (((CSettingString *)pSettingControl->GetSetting())->GetData().size() == 0 && pControl)
      {
        pControl->SetLabel2(g_localizeStrings.Get(734));
        pControl->SetEnabled(g_guiSettings.GetBool("Servers.WebServer"));
      }
    }
    else if (strSetting.Equals("Servers.WebServerPort"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Servers.WebServer"));
    }
    else if (strSetting.Equals("Network.IPAddress") || strSetting.Equals("Network.Subnet") || strSetting.Equals("Network.Gateway") || strSetting.Equals("Network.DNS"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl)
      {
        if (g_guiSettings.GetInt("Network.Assignment") != NETWORK_STATIC) 
        {
          //We are in non Static Mode! Setting the Received IP Information
          if(strSetting.Equals("Network.IPAddress"))pControl->SetLabel2(g_network.m_networkinfo.ip);
          else if(strSetting.Equals("Network.Subnet"))pControl->SetLabel2(g_network.m_networkinfo.subnet);
          else if(strSetting.Equals("Network.Gateway"))pControl->SetLabel2(g_network.m_networkinfo.gateway);
          else if(strSetting.Equals("Network.DNS"))pControl->SetLabel2(g_network.m_networkinfo.DNS1);
        }
        pControl->SetEnabled(g_guiSettings.GetInt("Network.Assignment") == NETWORK_STATIC);
      }
    }
    else if (strSetting.Equals("Network.HTTPProxyServer") || strSetting.Equals("Network.HTTPProxyPort"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Network.UseHTTPProxy"));
    }
    else if (strSetting.Equals("PostProcessing.VerticalDeBlockLevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.VerticalDeBlocking") &&
                           g_guiSettings.GetBool("PostProcessing.Enable") &&
                           !g_guiSettings.GetBool("PostProcessing.Auto"));
    }
    else if (strSetting.Equals("PostProcessing.HorizontalDeBlockLevel"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.HorizontalDeBlocking") &&
                           g_guiSettings.GetBool("PostProcessing.Enable") &&
                           !g_guiSettings.GetBool("PostProcessing.Auto"));
    }
    else if (strSetting.Equals("PostProcessing.VerticalDeBlocking") || strSetting.Equals("PostProcessing.HorizontalDeBlocking") || strSetting.Equals("PostProcessing.AutoBrightnessContrastLevels") || strSetting.Equals("PostProcessing.DeRing"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.Enable") &&
                           !g_guiSettings.GetBool("PostProcessing.Auto"));
    }
    else if (strSetting.Equals("PostProcessing.Auto"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("PostProcessing.Enable"));
    }
    else if (strSetting.Equals("VideoPlayer.InvertFieldSync"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetBool("VideoPlayer.FieldSync"));
    }
    else if (strSetting.Equals("Subtitles.Color") || strSetting.Equals("Subtitles.Style") || strSetting.Equals("Subtitles.CharSet"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(CUtil::IsUsingTTFSubtitles());
    }
    else if (strSetting.Equals("Subtitles.FlipBiDiCharSet"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      CStdString strCharset=g_langInfo.GetSubtitleCharSet();
      pControl->SetEnabled( /*CUtil::IsUsingTTFSubtitles() &&*/ g_charsetConverter.isBidiCharset(strCharset) > 0);
    }
    else if (strSetting.Equals("LookAndFeel.CharSet"))
    { // TODO: Determine whether we are using a TTF font or not.
      //   CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      //   if (pControl) pControl->SetEnabled(g_guiSettings.GetString("LookAndFeel.Font").Right(4) == ".ttf");
    }
    else if (strSetting.Equals("ScreenSaver.DimLevel"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode") == "Dim");
    }
    else if (strSetting.Equals("ScreenSaver.SlideShowPath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode") == "SlideShow");
    }
    else if (strSetting.Equals("ScreenSaver.SlideShowShuffle"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode") == "SlideShow");
    }
    else if (strSetting.Equals("ScreenSaver.Preview"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode") != "None");
    }
    else if (strSetting.Left(16).Equals("Weather.AreaCode"))
    {
      CSettingString *pSetting = (CSettingString *)GetSetting(strSetting)->GetSetting();
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(GetSetting(strSetting)->GetID());
      pControl->SetLabel2(pSetting->GetData());
    }
    else if (strSetting.Equals("LED.DisableOnPlayback"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
      int iColour = g_guiSettings.GetInt("LED.Colour");
      pControl->SetEnabled(iColour != LED_COLOUR_NO_CHANGE && iColour != LED_COLOUR_OFF);
    }
    else if (strSetting.Equals("MyMusic.TrackFormat"))
    {
      if (m_strOldTrackFormat != g_guiSettings.GetString("MyMusic.TrackFormat"))
      {
        CUtil::DeleteDatabaseDirectoryCache();
        m_strOldTrackFormat = g_guiSettings.GetString("MyMusic.TrackFormat");
      }
    }
    else if (strSetting.Equals("MyMusic.TrackFormatRight"))
    {
      if (m_strOldTrackFormatRight != g_guiSettings.GetString("MyMusic.TrackFormatRight"))
      {
        CUtil::DeleteDatabaseDirectoryCache();
        m_strOldTrackFormatRight = g_guiSettings.GetString("MyMusic.TrackFormatRight");
      }
    }
	  else if (strSetting.Equals("XBDateTime.TimeAddress"))
    {
		  CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("XBDateTime.TimeServer"));
    }
	  else if (strSetting.Equals("XBDateTime.Time") || strSetting.Equals("XBDateTime.Date"))
	  {
		  CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("XBDateTime.TimeServer")); 
		  SYSTEMTIME curTime;
		  GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("XBDateTime.Time"))
        time = g_infoManager.GetTime(false);  // false for no seconds
      else
        time = g_infoManager.GetDate();
      CSettingString *pSettingString = (CSettingString*)pSettingControl->GetSetting();
      pSettingString->SetData(time);
      pSettingControl->Update();
    }
    else if (strSetting.Equals("Masterlock.MasterUser") || strSetting.Equals("Masterlock.Enableshutdown") || strSetting.Equals("Masterlock.Protectshares") || strSetting.Equals("Masterlock.StartupLock") || strSetting.Equals("Masterlock.LockSettingsFilemanager") || strSetting.Equals("Masterlock.LockHomeMedia"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if(g_advancedSettings.bUseMasterLockAdvancedXml)
      {
        pControl->SetEnabled(false);
      }
      else
      {
        bool bState = g_guiSettings.GetString("Masterlock.UserMode").Equals("1"); // advanced user mode
        if (g_guiSettings.GetString("Masterlock.UserMode").Equals("0") && (strSetting =="Masterlock.LockSettingsFilemanager" || strSetting =="Masterlock.LockHomeMedia"))
          bState = true;
        if(g_passwordManager.iMasterLockMode == LOCK_MODE_EVERYONE)
          bState = false;
        if (pControl) 
          pControl->SetEnabled(bState);
        if (strSetting.Equals("Masterlock.MasterUser"))
        {
          if (!g_passwordManager.bMasterUser && g_application.m_bMasterLockOverridesLocalPasswords)
          {
            g_application.m_bMasterLockOverridesLocalPasswords = false;
            g_application.m_MasterUserModeCounter = 2;  // reset the count "to Overwrite the local Pass stuff!"
          }
        }
      }
    }
    else if (strSetting.Equals("Masterlock.Mastercode"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if(g_advancedSettings.bUseMasterLockAdvancedXml)
      {
        pControl->SetEnabled(false);
      }
      else
      {
        CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
        if (pControl)
        {
          switch(g_passwordManager.iMasterLockMode)
          {
          case LOCK_MODE_EVERYONE:
            pControl->SetLabel2(g_localizeStrings.Get(1223).c_str());
            break;
          case LOCK_MODE_NUMERIC:
            pControl->SetLabel2(g_localizeStrings.Get(12337).c_str());
            break;
          case LOCK_MODE_GAMEPAD:
            pControl->SetLabel2(g_localizeStrings.Get(12338).c_str());
            break;
          case LOCK_MODE_QWERTY:
            pControl->SetLabel2(g_localizeStrings.Get(12339).c_str());
            break;
          }
        }
      }
    }
    else if (strSetting.Equals("Masterlock.UserMode"))
    {
      //Enable/Disable Item
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
      if(g_advancedSettings.bUseMasterLockAdvancedXml)
      {
        pControl->SetEnabled(false);
      }
      else
      {
        if(g_passwordManager.iMasterLockMode > LOCK_MODE_EVERYONE && pControl)
            pControl->SetEnabled(true);
        else 
          pControl->SetEnabled(false);
        
        // Set Controll Labels
        CGUIButtonControl *pControlLabel = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
        if (pControlLabel)
        {
            CStdString temp= g_guiSettings.GetString("Masterlock.UserMode");
            if (temp.Equals("0"))
            pControlLabel->SetLabel2(g_localizeStrings.Get(1224).c_str());
            else if (temp.Equals("1"))
              pControlLabel->SetLabel2(g_localizeStrings.Get(1225).c_str());
            else if (temp.IsEmpty() || !temp.Equals("1") || !temp.Equals("0"))
            {
              g_guiSettings.SetString("Masterlock.UserMode","0");
              pControlLabel->SetLabel2(g_localizeStrings.Get(1224).c_str());
            }
        }
      }
    }
    else if (strSetting.Equals("Autodetect.NickName") || strSetting.Equals("Autodetect.CreateLink") || strSetting.Equals("Autodetect.PopUpInfo") || strSetting.Equals("Autodetect.SendUserPw"))
    {
      //GeminiServer
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Autodetect.OnOff"));
    }
    else if (strSetting.Equals("MyVideos.ExternalDVDPlayer"))
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MyVideos.UseExternalDVDPlayer"));
    }
    else if (strSetting.Equals("MyPrograms.TrainerPath"))
    {
      CGUIButtonControl *pControl = (CGUIButtonControl *)GetControl(pSettingControl->GetID());
      if (pControl && g_guiSettings.GetString(strSetting, false).IsEmpty())
        pControl->SetLabel2("");
    }
  }
}

void CGUIWindowSettingsCategory::UpdateRealTimeSettings()
{
  for (unsigned int i = 0; i < m_vecSettings.size(); i++)
  {
    CBaseSettingControl *pSettingControl = m_vecSettings[i];
    CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
	  if (strSetting.Equals("XBDateTime.Time") || strSetting.Equals("XBDateTime.Date"))
	  {
		  CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
		  if (pControl) pControl->SetEnabled(!g_guiSettings.GetBool("XBDateTime.TimeServer")); 
		  SYSTEMTIME curTime;
		  GetLocalTime(&curTime);
      CStdString time;
      if (strSetting.Equals("XBDateTime.Time"))
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
  if (strSetting.Left(16).Equals("Weather.AreaCode"))
  {
    CStdString strSearch;
    if (CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(14024), false))
    {
      strSearch.Replace(" ", "+");
      CStdString strResult = ((CSettingString *)pSettingControl->GetSetting())->GetData();
      if (g_weatherManager.GetSearchResults(strSearch, strResult))
        ((CSettingString *)pSettingControl->GetSetting())->SetData(strResult);
    }
  }
  pSettingControl->OnClick(); // call the control to do it's thing
  // ok, now check the various special things we need to do
  if (strSetting.Equals("MyPrograms.UseDirectoryName"))
  { // delete the program database.
    // TODO: Should this actually be done here??
    CStdString programDatabase = g_settings.GetDatabaseFolder();
    programDatabase += PROGRAM_DATABASE_NAME;
    if (CFile::Exists(programDatabase))
      ::DeleteFile(programDatabase.c_str());
  }
  else if (strSetting.Equals("MyMusic.Visualisation"))
  { // new visualisation choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    if (pControl->GetValue() == 0)
      pSettingString->SetData("None");
    else
      pSettingString->SetData(pControl->GetCurrentLabel() + ".vis");
  }
  else if (strSetting.Equals("MusicFiles.Repeat"))
  {
    g_playlistPlayer.Repeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("MusicFiles.Repeat"));
  }
  else if (strSetting.Equals("Karaoke.Port0VoiceMask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Karaoke.Port0VoiceMask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(0, g_guiSettings.GetSetting("Karaoke.Port0VoiceMask"));
  }
  else if (strSetting.Equals("Karaoke.Port1VoiceMask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Karaoke.Port1VoiceMask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(1, g_guiSettings.GetSetting("Karaoke.Port1VoiceMask"));
  }
  else if (strSetting.Equals("Karaoke.Port2VoiceMask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Karaoke.Port2VoiceMask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(2, g_guiSettings.GetSetting("Karaoke.Port2VoiceMask"));
  }
  else if (strSetting.Equals("Karaoke.Port2VoiceMask"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Karaoke.Port3VoiceMask", pControl->GetCurrentLabel());
    FillInVoiceMaskValues(3, g_guiSettings.GetSetting("Karaoke.Port3VoiceMask"));
  }
  else if (strSetting.Equals("MyMusic.CleanupMusicLibrary"))
  {
    CMusicDatabase musicdatabase;
    musicdatabase.Clean();
    CUtil::DeleteDatabaseDirectoryCache();
  }
  else if (strSetting.Equals("MusicPlayer.JumpToAudioHardware") || strSetting.Equals("VideoPlayer.JumpToAudioHardware"))
  {
    JumpToSection(WINDOW_SETTINGS_SYSTEM, 5);
  }
  else if (strSetting.Equals("MyMusic.UseAudioScrobbler") || strSetting.Equals("MyMusic.AudioScrobblerUserName") || strSetting.Equals("MyMusic.AudioScrobblerPassword"))
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
  else if (strSetting.Equals("MusicPlayer.OutputToAllSpeakers"))
  {
    CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();

    if (!g_application.IsPlaying())
    {
      g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
    }
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("MusicPlayer.ReplayGainType");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("MusicPlayer.ReplayGainPreAmp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("MusicPlayer.ReplayGainNoGainPreAmp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("MusicPlayer.ReplayGainAvoidClipping");
  }
  else if (strSetting.Equals("CDDARipper.Path"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString strPath = pSettingString->GetData();
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyFilesShares,g_localizeStrings.Get(607),strPath,true))
      pSettingString->SetData(strPath);
  }
  else if (strSetting.Equals("XLinkKai.Enabled"))
  {
    if (g_guiSettings.GetBool("XLinkKai.Enabled"))
      g_application.StartKai();
    else
      g_application.StopKai();
  }
  else if (strSetting.Equals("LCD.Type"))
  {
    g_lcd->Initialize();
  }
  else if (strSetting.Equals("LCD.BackLight"))
  {
    g_lcd->SetBackLight(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Equals("LCD.ModChip"))
  {
    g_lcd->Stop();
    CLCDFactory factory;
    delete g_lcd;
    g_lcd = factory.Create();
    g_lcd->Initialize();
  }
  else if (strSetting.Equals("LCD.Contrast"))
  {
    g_lcd->SetContrast(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Equals("System.TargetTemperature"))
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    CFanController::Instance()->SetTargetTemperature(pSetting->GetData());
  }
  else if (strSetting.Equals("System.FanSpeed"))
  {
    CSettingInt *pSetting = (CSettingInt*)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    int iSpeed = (RESOLUTION)msg.GetParam1();
    g_guiSettings.SetInt("System.FanSpeed", iSpeed);
    CFanController::Instance()->SetFanSpeed(iSpeed);
  }
  else if (strSetting.Equals("System.AutoTemperature"))
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
  else if (strSetting.Equals("System.FanSpeedControl"))
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
  else if (strSetting.Equals("Autodetect.NickName") )
  {
    CStdString strXboxNickNameIn = g_guiSettings.GetString("Autodetect.NickName");
    CUtil::SetXBOXNickName(strXboxNickNameIn, strXboxNickNameIn);
  }
  else if (strSetting.Equals("Servers.FTPServer"))
  {
    g_application.StopFtpServer();
    if (g_guiSettings.GetBool("Servers.FTPServer"))
      g_application.StartFtpServer();
    
  }
  else if (strSetting.Equals("Servers.FTPServerPassword"))
  {
   SetFTPServerUserPass(); 
  }
  else if (strSetting.Equals("Servers.FTPServerUser"))
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    g_guiSettings.SetString("Servers.FTPServerUser", pControl->GetCurrentLabel());
  }

  else if (strSetting.Equals("Servers.WebServer") || strSetting.Equals("Servers.WebServerPort") || strSetting.Equals("Servers.WebServerPassword"))
  {
    if (strSetting.Equals("Servers.WebServerPort"))
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
  else if (strSetting.Equals("Network.HTTPProxyPort"))
  {
    CSettingString *pSetting = (CSettingString *)pSettingControl->GetSetting();
    // check that it's a valid port
    int port = atoi(pSetting->GetData().c_str());
    if (port <= 0 || port > 65535)
      pSetting->SetData("8080");
  }
  else if (strSetting.Equals("MyVideos.Calibrate"))
  { // activate the video calibration screen
    m_gWindowManager.ActivateWindow(WINDOW_MOVIE_CALIBRATION);
  }
  else if (strSetting.Equals("MyVideos.ExternalDVDPlayer"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    // TODO 2.0: Localize this
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".xbe", "DVD Player", path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("Subtitles.Height"))
  {
    if (!CUtil::IsUsingTTFSubtitles())
    {
      CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
      ((CSettingInt *)pSettingControl->GetSetting())->FromString(pControl->GetCurrentLabel());
    }
  }
  else if (strSetting.Equals("Subtitles.Font"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    pSettingString->SetData(pControl->GetCurrentLabel());
    FillInSubtitleHeights(g_guiSettings.GetSetting("Subtitles.Height"));
  }
  else if (strSetting.Equals("Subtitles.CharSet"))
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
  else if (strSetting.Equals("LookAndFeel.CharSet"))
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
  else if (strSetting.Equals("LookAndFeel.Font"))
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
  else if (strSetting.Equals("LookAndFeel.Skin"))
  { // new skin choosen...
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
    CStdString strSkin = pControl->GetCurrentLabel();
    CStdString strSkinPath = "Q:\\skin\\" + strSkin;
    if (g_SkinInfo.Check(strSkinPath))
    {
      m_strErrorMessage.Empty();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().textColor);
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
      m_strErrorMessage.Format("Incompatible skin. We require skins of version %0.2f or higher", g_SkinInfo.GetMinVersion());
      m_strNewSkin.Empty();
      g_application.CancelDelayLoadSkin();
      pControl->SettingsCategorySetSpinTextColor(pControl->GetButtonLabelInfo().disabledColor);
    }
  }
  else if (strSetting.Equals("LookAndFeel.SoundSkin"))
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
  else if (strSetting.Equals("LookAndFeel.GUICentering"))
  { // activate the video calibration screen
    m_gWindowManager.ActivateWindow(WINDOW_UI_CALIBRATION);
  }
  else if (strSetting.Equals("LookAndFeel.Resolution"))
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
  else if (strSetting.Equals("LED.Colour"))
  { // Alter LED Colour immediately
    ILED::CLEDControl(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Equals("LookAndFeel.Language"))
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
  else if (strSetting.Equals("LookAndFeel.SkinTheme"))
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
  else if (strSetting.Equals("VideoPlayer.DisplayResolution"))
  {
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg);
    pSettingInt->SetData(msg.GetParam1());
  }
  else if (strSetting.Equals("UIFilters.Flicker") || strSetting.Equals("UIFilters.Soften"))
  { // reset display
    g_graphicsContext.SetGUIResolution(g_guiSettings.m_LookAndFeelResolution);
  }
  else if (strSetting.Equals("ScreenSaver.Mode"))
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
  else if (strSetting.Equals("ScreenSaver.Preview"))
  {
    g_application.ActivateScreenSaver();
  }
  else if (strSetting.Equals("ScreenSaver.SlideShowPath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyPictureShares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("MyPrograms.Dashboard"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    VECSHARES shares;
    g_mediaManager.GetLocalDrives(shares);
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, ".xbe", g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
      pSettingString->SetData(path);
  }
  else if (strSetting.Equals("MyPrograms.TrainerPath"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CStdString path = pSettingString->GetData();
    if (CGUIDialogFileBrowser::ShowAndGetDirectory(g_settings.m_vecMyFilesShares, g_localizeStrings.Get(pSettingString->m_iHeadingString), path))
    {
      pSettingString->SetData(path);
      // TODO: Should we ask to rescan the trainers here?
    }
  }
  else if (strSetting.Equals("LED.Colour"))
  { // Alter LED Colour immediately
    ILED::CLEDControl(((CSettingInt *)pSettingControl->GetSetting())->GetData());
  }
  else if (strSetting.Left(22).Equals("MusicPlayer.ReplayGain"))
  { // Update our replaygain settings
    g_guiSettings.m_replayGain.iType = g_guiSettings.GetInt("MusicPlayer.ReplayGainType");
    g_guiSettings.m_replayGain.iPreAmp = g_guiSettings.GetInt("MusicPlayer.ReplayGainPreAmp");
    g_guiSettings.m_replayGain.iNoGainPreAmp = g_guiSettings.GetInt("MusicPlayer.ReplayGainNoGainPreAmp");
    g_guiSettings.m_replayGain.bAvoidClipping = g_guiSettings.GetBool("MusicPlayer.ReplayGainAvoidClipping");
  }
  else if (strSetting.Equals("XBDateTime.Region"))
  {
    CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());

    const CStdString& strRegion=pControl->GetCurrentLabel();
    g_langInfo.SetCurrentRegion(strRegion);
    g_guiSettings.SetString("XBDateTime.Region", strRegion);
  }
  else if (strSetting.Equals("XBDateTime.TimeServer") || strSetting.Equals("XBDateTime.TimeAddress"))
  {
    g_application.StopTimeServer();
    if (g_guiSettings.GetBool("XBDateTime.TimeServer"))
      g_application.StartTimeServer();
  }
  else if (strSetting.Equals("XBDateTime.Time"))
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
  else if (strSetting.Equals("XBDateTime.Date"))
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
  else if (strSetting.Equals("Smb.Winsserver") || strSetting.Equals("Smb.Workgroup") )
  {
    if (g_guiSettings.GetString("Smb.Winsserver") == "0.0.0.0") 
      g_guiSettings.SetString("Smb.Winsserver", "");

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
  else if (strSetting.Equals("Masterlock.Mastercode"))
  {
    // Prompt user for mastercode if the mastercode was set before or by xml
    if (g_passwordManager.CheckMasterLockCode()) 
    {
      // Now Prompt User to enter the old and then the new MasterCode!
      if(SetMasterLockMode())
      {
        // We asked for the master password and saved the new one!
        // Nothing todo here
      }
    }
  }
  else if (strSetting.Equals("Masterlock.UserMode"))
  {
    if (g_passwordManager.CheckMasterLockCode()) 
    {
      if(SetUserMode())
      {
        // We asked for the master password, so we don't need to check the changes and ask again
        g_passwordManager.bMasterNormalUserMode = g_guiSettings.GetString("Masterlock.UserMode").Equals("0"); //true 0:Normal false 1:Advanced 
      }
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
  CGUIControl *baseControl = NULL;
  if (pSetting->GetControlType() == CHECKMARK_CONTROL)
  {
    baseControl = m_pOriginalRadioButton;
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
    iPosY += iGap;
  }
  else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
  {
    baseControl = m_pOriginalSpin;
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    ((CGUISpinControlEx *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
    pControl->SetWidth(iWidth);
    pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
    iPosY += iGap;
  }
  else if (pSetting->GetControlType() == SEPARATOR_CONTROL && m_pOriginalImage)
  {
    baseControl = m_pOriginalImage;
    pControl = new CGUIImage(*m_pOriginalImage);
    if (!pControl) return;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    pSettingControl = new CSeparatorSettingControl((CGUIImage *)pControl, iControlID, pSetting);
    iPosY += pControl->GetHeight();
  }
  else if (pSetting->GetControlType() != SEPARATOR_CONTROL) // button control
  {
    baseControl = m_pOriginalButton;
    pControl = new CGUIButtonControl(*m_pOriginalButton);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    ((CGUIButtonControl *)pControl)->SettingsCategorySetTextAlign(XBFONT_CENTER_Y);
    ((CGUIButtonControl *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
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
  Insert(pControl, baseControl);
  pControl->AllocResources();
  m_vecSettings.push_back(pSettingControl);
}

void CGUIWindowSettingsCategory::Render()
{
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
  if (m_iNetworkAssignment != g_guiSettings.GetInt("Network.Assignment") ||
      (m_iNetworkAssignment == NETWORK_STATIC && (
         m_strNetworkIPAddress != g_guiSettings.GetString("Network.IPAddress") ||
         m_strNetworkSubnet != g_guiSettings.GetString("Network.Subnet") ||
         m_strNetworkGateway != g_guiSettings.GetString("Network.Gateway") ||
         m_strNetworkDNS != g_guiSettings.GetString("Network.DNS"))))
  { 
    // our network settings have changed - we should prompt the user to reset XBMC
    if (CGUIDialogYesNo::ShowAndGetInput(14038, 14039, 14040, 0))
    { 
      // reset settings
      g_applicationMessenger.RestartApp();
      // Todo: aquire new network settings without restart app!
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
      pControl->AddLabel(strLabel, i);
    }
    pControl->SetValue(pSettingInt->GetData());
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
        const char* unicodeAttr = ((TiXmlElement*) pChild)->Attribute("unicode");

        bool isUnicode=(unicodeAttr && stricmp(unicodeAttr, "true") == 0);

        bool isAllowed=true;
        if (g_langInfo.ForceUnicodeFont() && !isUnicode)
          isAllowed=false;

        if (idAttr != NULL && isAllowed)
        {
          pControl->AddLabel(idAttr, iSkinFontSet);
          if (strcmpi(idAttr, g_guiSettings.GetString("LookAndFeel.Font").c_str()) == 0)
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
  if ( !xmlDoc.LoadFile( "Q:\\system\\voicemasks.xml" ) ) return ;
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
    g_stSettings.m_karaokeVoiceMask[dwPort].energy = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].pitch = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].whisper = XVOICE_MASK_PARAM_DISABLED;
    g_stSettings.m_karaokeVoiceMask[dwPort].robotic = XVOICE_MASK_PARAM_DISABLED;
    return;
  }

  //find mask values in xml...
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( "Q:\\system\\voicemasks.xml" ) ) return ;
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

void CGUIWindowSettingsCategory::FillInResolutions(CSetting *pSetting)
{
  CSettingInt *pSettingInt = (CSettingInt*)pSetting;
  CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
  pControl->Clear();
  // Find the valid resolutions and add them as necessary
  vector<RESOLUTION> res;
  g_graphicsContext.GetAllowedResolutions(res);
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
  }
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
  pControl->AddLabel(g_localizeStrings.Get(108), 3); // PictureSlideShow

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
    else if (strDefaultScr == "SlideShow") // GeminiServer: PictureSlideShow
      iCurrentScr = 3;
    else
    {
      iCurrentScr = 0;
      pSettingString->SetData("None");
    }
  }
  pControl->SetValue(iCurrentScr);
}
bool CGUIWindowSettingsCategory::SetMasterLockMode()
{
  // load a context menu with the options for the mastercode...
  CGUIDialogContextMenu *menu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (menu)
  {
    menu->Initialize();
    menu->AddButton(1223);
    menu->AddButton(12337);
    menu->AddButton(12338);
    menu->AddButton(12339);
    menu->SetPosition((g_graphicsContext.GetWidth() - menu->GetWidth()) / 2, (g_graphicsContext.GetHeight() - menu->GetHeight()) / 2);
    menu->DoModal();

    CStdString newPassword;
    int iLockMode = -1;
    switch(menu->GetButton())
    {
    case 1:
      iLockMode = LOCK_MODE_EVERYONE; //Disabled! Need check routine!!!
      break;
    case 2:
      iLockMode = LOCK_MODE_NUMERIC;
      CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword);
      break;
    case 3:
      iLockMode = LOCK_MODE_GAMEPAD;
      CGUIDialogGamepad::ShowAndVerifyNewPassword(newPassword);
      break;
    case 4:
      iLockMode = LOCK_MODE_QWERTY;
      CGUIDialogKeyboard::ShowAndVerifyNewPassword(newPassword);
      break;
    }
    if (!newPassword.IsEmpty() && newPassword.c_str() != "-" )
    {
      g_passwordManager.iMasterLockMode = iLockMode;
      g_passwordManager.strMasterLockCode = newPassword;
      g_guiSettings.SetInt("Masterlock.Mastermode",g_passwordManager.iMasterLockMode );
      g_guiSettings.SetString("Masterlock.Mastercode", g_passwordManager.strMasterLockCode.c_str());
    }
    else if (iLockMode != LOCK_MODE_EVERYONE)
    {
        // PopUp OK and Display: Master Code is not Valid or is empty or not set!
        CGUIDialogOK::ShowAndGetInput(12360, 12367, 12368, 0);
    }
    if (iLockMode == LOCK_MODE_EVERYONE || iLockMode == -1)
    {
      /*if(g_passwordManager.CheckMasterLockCode())
      {}*/
      g_guiSettings.SetInt("Masterlock.Mastermode",LOCK_MODE_EVERYONE);
      g_guiSettings.SetBool("Masterlock.Enableshutdown",false);
      g_guiSettings.SetBool("Masterlock.Protectshares",false);
      g_guiSettings.SetBool("Masterlock.MasterUser",false);
      g_guiSettings.SetString("Masterlock.Mastercode","-");
      g_guiSettings.SetBool("Masterlock.StartupLock",false);
      g_guiSettings.SetInt("Masterlock.LockHomeMedia",0);
      g_guiSettings.SetInt("Masterlock.LockSettingsFilemanager",0);
      g_passwordManager.CheckMasterLock(false);
      
    }
    else{
      //iLockMode =  g_passwordManager.iMasterLockMode;
      return false;  // Nothing Changed!
    }
  }else return false;
  return true;
}
bool CGUIWindowSettingsCategory::SetUserMode()
{
  // load a context menu with the options for the mastercode...
  CGUIDialogContextMenu *menu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (menu)
  {
    menu->Initialize();
    menu->AddButton(1224); // Normal User   ->False
    menu->AddButton(1225); // Advanced User ->TRUE
    menu->SetPosition((g_graphicsContext.GetWidth() - menu->GetWidth()) / 2, (g_graphicsContext.GetHeight() - menu->GetHeight()) / 2);
    menu->DoModal();

    CStdString newUserMode;
    int iLockMode = -1;
    switch(menu->GetButton())
    {
    case 1:
      g_guiSettings.SetString("Masterlock.UserMode","0");
      break;
    case 2:
      g_guiSettings.SetString("Masterlock.UserMode","1");
      break;
    }
  }else return false;
  return true;
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
  else { //Set "None" if there is no FTP User found!
    pControl->AddLabel(g_localizeStrings.Get(231).c_str(), 0);
    pControl->SetValue(0);
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
        CGUIDialogOK::ShowAndGetInput(728, 0, 1247, 0);
      }
      return true;
    }
    else
    {
      // PopUp OK and Display: FTP Server Password is empty! Try Again!
      CGUIDialogOK::ShowAndGetInput(728, 0, 12358, 0);
    }
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

void CGUIWindowSettingsCategory::JumpToSection(DWORD dwWindowId, int iSection)
{
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
  CStdString strSettingString = g_guiSettings.GetString("LookAndFeel.SkinTheme");

  m_strNewSkinTheme.Empty();

  // Clear and add. the Default Label
  pControl->Clear();
  pControl->SetShowRange(true);
  pControl->AddLabel(g_localizeStrings.Get(15109), 0); // "SKINDEFAULT"! The standart Textures.xpr will be used!
  
  // Get the Current Skin Path 
  CStdString strPath = "Q:\\skin\\"; 
  strPath += g_guiSettings.GetString("LookAndFeel.Skin"); 
  strPath += "\\media\\";

  // find all *.xpr in this path
  CHDDirectory directory;
  CFileItemList items;
  directory.GetDirectory(strPath, items);
  CStdString strDefaultTheme = pSettingString->GetData();
  
  // Search for Themes in the Current skin!
  vector<CStdString> vecTheme;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".xpr" && pItem->GetLabel().CompareNoCase("Textures.xpr"))
      {
        CStdString strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }

  // Remove the .xpr extension from the Themes
  CStdString strExtension;
  CUtil::GetExtension(strSettingString, strExtension);
  if (strExtension == ".xpr") strSettingString.Delete(strSettingString.size() - 4, 4);
  // Sort the Themes for GUI and list them
  int iCurrentTheme = 0;
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
  for (i = 0; i < (int) vecTheme.size(); ++i)
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
  m_iNetworkAssignment = g_guiSettings.GetInt("Network.Assignment");
  m_strNetworkIPAddress = g_guiSettings.GetString("Network.IPAddress");
  m_strNetworkSubnet = g_guiSettings.GetString("Network.Subnet");
  m_strNetworkGateway = g_guiSettings.GetString("Network.Gateway");
  m_strNetworkDNS = g_guiSettings.GetString("Network.DNS");
  m_strOldTrackFormat = g_guiSettings.GetString("MyMusic.TrackFormat");
  m_strOldTrackFormatRight = g_guiSettings.GetString("MyMusic.TrackFormatRight");
  m_NewResolution = INVALID;
  SetupControls();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowSettingsCategory::RestoreControlStates()
{ // we just restore the focused control - nothing else
  int focusControl = m_lastControlID ? m_lastControlID : m_dwDefaultFocusControlID;
  SET_CONTROL_FOCUS(focusControl, 0);
}