#include "stdafx.h"
#include "GUIWindowSettingsCategory.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "application.h"
#include "filesystem/HDDirectory.h"
#include "util.h"
#include "GUILabelControl.h"
#include "GUICheckMarkControl.h"
#include "utils/Weather.h"
#include "MusicDatabase.h"
#include "XBAudioConfig.h"
#include "XBVideoConfig.h"
#include "utils/lcd.h"
#include "utils/led.h"
#include "utils/LCDFactory.h"
#include "utils/FanController.h"
#include "GUIDialogYesNo.h"
#include "utils/CharsetConverter.h"
#include "playlistplayer.h"

#define CONTROL_GROUP_BUTTONS						0
#define CONTROL_GROUP_SETTINGS					1

#define CONTROL_SETTINGS_LABEL					2

#define CONTROL_BUTTON_AREA							3
#define CONTROL_BUTTON_GAP							4
#define CONTROL_AREA										5
#define CONTROL_GAP											6
#define CONTROL_DEFAULT_BUTTON					7
#define CONTROL_DEFAULT_RADIOBUTTON			8
#define CONTROL_DEFAULT_SPIN						9
#define CONTROL_DEFAULT_SETTINGS_BUTTON 10
#define CONTROL_START_BUTTONS						30
#define CONTROL_START_CONTROL						50

struct sortstringbyname
{
	bool operator()(const CStdString& strItem1, const CStdString& strItem2)
	{
    CStdString strLine1=strItem1;
    CStdString strLine2=strItem2;
    strLine1=strLine1.ToLower();
    strLine2=strLine2.ToLower();
    return strcmp(strLine1.c_str(),strLine2.c_str())<0;
  }
};

CGUIWindowSettingsCategory::CGUIWindowSettingsCategory(void)
:CGUIWindow(0)
{
	m_iLastControl=-1;
	m_pOriginalSpin = NULL;
	m_pOriginalRadioButton = NULL;
	m_pOriginalButton = NULL;
	m_pOriginalSettingsButton = NULL;
	// set the correct ID range...
	m_dwIDRange = 8;
	m_iScreen = 0;
	// set the network settings so that we don't reset them unnecessarily
	m_iNetworkAssignment = -1;
	m_strErrorMessage = L"";
}

CGUIWindowSettingsCategory::~CGUIWindowSettingsCategory(void)
{
}

void CGUIWindowSettingsCategory::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_iLastControl=-1;

    m_gWindowManager.PreviousWindow();
		return;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowSettingsCategory::OnMessage(CGUIMessage &message)
{
	switch (message.GetMessage())
	{
	case GUI_MSG_CLICKED:
		{
			unsigned int iControl=message.GetSenderId();
/*			if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size())
			{
				// change the setting...
				m_iSection = iControl-CONTROL_START_BUTTONS;
				CheckNetworkSettings();
				CreateSettings();
				return true;
			}*/
			for (unsigned int i=0; i<m_vecSettings.size(); i++)
			{
				if (m_vecSettings[i]->GetID() == iControl)
					OnClick(m_vecSettings[i]);
			}
		}
		break;
	case GUI_MSG_SETFOCUS:
		{
			m_iLastControl=message.GetControlId();
			CBaseSettingControl *pSkinControl = GetSetting("LookAndFeel.Skin");
			CBaseSettingControl *pLanguageControl = GetSetting("LookAndFeel.Language");
			if (g_application.m_dwSkinTime && ((pSkinControl && pSkinControl->GetID() == message.GetControlId()) || (pLanguageControl && pLanguageControl->GetID() == message.GetControlId())))
			{
				//	Cancel skin and language load if one of the spin controls lose focus
				g_application.CancelDelayLoadSkin();
				//	Reset spin controls to the current selected skin & language
				if (pSkinControl) FillInSkins(pSkinControl->GetSetting());
				if (pLanguageControl) FillInLanguages(pLanguageControl->GetSetting());
			}
			unsigned int iControl=message.GetControlId();
			unsigned int iSender=message.GetSenderId();
			// if both the sender and the control are within out category range, then we have a change of
			// category.
			if (iControl >= CONTROL_START_BUTTONS && iControl < CONTROL_START_BUTTONS + m_vecSections.size() &&
					iSender >= CONTROL_START_BUTTONS && iSender < CONTROL_START_BUTTONS + m_vecSections.size())
			{
				// change the setting...
				if (iControl-CONTROL_START_BUTTONS != m_iSection)
				{
					m_iSection = iControl-CONTROL_START_BUTTONS;
					CheckNetworkSettings();
					CreateSettings();
				}
			}
		}
		break;
	case GUI_MSG_LOAD_SKIN:
		{
			unsigned iCtrlID = GetFocusedControl();

			CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iCtrlID,0,0,NULL);
			g_graphicsContext.SendMessage(msg);

			//	Do we need to reload the language file
			if (!m_strNewLanguage.IsEmpty())
			{
				g_guiSettings.SetString("LookAndFeel.Language", m_strNewLanguage);
				g_settings.Save();

				CStdString strLanguage=m_strNewLanguage;
				CStdString strPath = "Q:\\language\\";
				strPath+=strLanguage;
				strPath+="\\strings.xml";
				g_localizeStrings.Load(strPath);
			}

			// Do we need to reload the skin font set
			if (!m_strNewSkinFontSet.IsEmpty())
			{
				g_guiSettings.SetString("LookAndFeel.Font", m_strNewSkinFontSet);
				g_settings.Save();
			}
			
			//	Reload another skin
			if (!m_strNewSkin.IsEmpty())
			{
				g_guiSettings.SetString("LookAndFeel.Skin", m_strNewSkin);
				g_settings.Save();
			}

			//	Reload the skin
			int iWindowID = GetID();
			g_application.LoadSkin(g_guiSettings.GetString("LookAndFeel.Skin"));

			m_gWindowManager.ActivateWindow(iWindowID);
			SET_CONTROL_FOCUS(iCtrlID, msg.GetParam2());
		}
		break;
	case GUI_MSG_WINDOW_INIT:
		{
			m_iScreen = (int)message.GetParam2() - (int)m_dwWindowId;
			m_iNetworkAssignment = g_guiSettings.GetInt("Network.Assignment");
			m_strNetworkIPAddress = g_guiSettings.GetString("Network.IPAddress");
			m_strNetworkSubnet = g_guiSettings.GetString("Network.Subnet");
			m_strNetworkGateway = g_guiSettings.GetString("Network.Gateway");
			m_strNetworkDNS = g_guiSettings.GetString("Network.DNS");
			m_dwResTime     = 0;
			m_OldResolution = (RESOLUTION)g_guiSettings.GetInt("LookAndFeel.Resolution");
			int iFocusControl=m_iLastControl;

			CGUIWindow::OnMessage(message);

      SetupControls();

			if (iFocusControl>-1)
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
				{	// should we perhaps show a dialog here?
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
	if (!m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalButton || !pButtonArea || !pControlGap)
		return;
	m_pOriginalSpin->SetVisible(false);
	m_pOriginalRadioButton->SetVisible(false);
	m_pOriginalButton->SetVisible(false);
	m_pOriginalSettingsButton->SetVisible(false);
	// setup our control groups...
	m_vecGroups.clear();
	m_vecGroups.push_back(-1);
	// get a list of different sections
	CSettingsGroup *pSettingsGroup = g_guiSettings.GetGroup(m_iScreen);
	if (!pSettingsGroup) return;
	// update the screen string
	SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, pSettingsGroup->GetLabelID());
	// get the categories we need
	pSettingsGroup->GetCategories(m_vecSections);
	// run through and create our buttons...
	for (unsigned int i=0; i<m_vecSections.size(); i++)
	{
		CGUIButtonControl *pButton = new CGUIButtonControl(*m_pOriginalSettingsButton);
		pButton->SetText(g_localizeStrings.Get(m_vecSections[i]->m_dwLabelID));
		pButton->SetID(CONTROL_START_BUTTONS + i);
		pButton->SetGroup(CONTROL_GROUP_BUTTONS);
		pButton->SetPosition(pButtonArea->GetXPosition(), pButtonArea->GetYPosition() + i*pControlGap->GetHeight());
		pButton->SetNavigation(CONTROL_START_BUTTONS+(int)i-1, CONTROL_START_BUTTONS+i+1, CONTROL_START_CONTROL, CONTROL_START_CONTROL);
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
	// add the control group
	m_vecGroups.push_back(-1);

	const CGUIControl *pControlArea = GetControl(CONTROL_AREA);
	const CGUIControl *pControlGap = GetControl(CONTROL_GAP);
	if (!pControlArea || !pControlGap)
		return;
	int iPosX = pControlArea->GetXPosition();
	int iWidth = pControlArea->GetWidth();
	int iPosY = pControlArea->GetYPosition();
	int iGapY = pControlGap->GetHeight();
	vecSettings settings;
	g_guiSettings.GetSettingsGroup(m_vecSections[m_iSection]->m_strCategory, settings);
	for (unsigned int i=0; i<settings.size(); i++)
	{
		CSetting *pSetting = settings[i];
		AddSetting(pSetting, iPosX, iPosY, iWidth, CONTROL_START_CONTROL+i);
		// special cases...
		CStdString strSetting = pSetting->GetSetting();
		if (strSetting == "Pictures.AutoSwitchMethod" || strSetting == "ProgramsLists.AutoSwitchMethod" || strSetting == "MusicLists.AutoSwitchMethod" || strSetting == "VideoLists.AutoSwitchMethod")
		{
			CSettingInt *pSettingInt = (CSettingInt*)pSetting;
			CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
			for (int i=pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
			{
				pControl->AddLabel(g_localizeStrings.Get(14015+i), i);
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
			FillInVisualisations(pSetting);
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
			for (int i=pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i++)
			{
				CStdString strLabel;
				if(g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /* DEGREES_F */)
					strLabel.Format("%2.0f%cF", ((9.0 / 5.0) * (float)i) + 32.0, 176);
				else
					strLabel.Format("%i%cC", i, 176);
				pControl->AddLabel(strLabel, i-pSettingInt->m_iMin);
			}
			pControl->SetValue(pSettingInt->GetData()-pSettingInt->m_iMin);
		}
		else if (strSetting == "System.FanSpeed")
		{
			CSettingInt *pSettingInt = (CSettingInt*)pSetting;
			CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
			CStdString strPercentMask=g_localizeStrings.Get(14047);
			for (int i=pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i+=5)
			{
				CStdString strLabel;
				strLabel.Format(strPercentMask.c_str(), i*2);
				pControl->AddLabel(strLabel, i);
			}
			pControl->SetValue(int(pSettingInt->GetData()/5)-1);
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
		{	// get password from the webserver if it's running (and update our settings)
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
			pControl->SetValue(pSettingInt->GetData()-1);
		}
		else if (strSetting == "Subtitles.Color")
		{
			CSettingInt *pSettingInt = (CSettingInt*)pSetting;
			CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(strSetting)->GetID());
			pControl->AddLabel(g_localizeStrings.Get(743), SUBTITLE_COLOR_YELLOW);
			pControl->AddLabel(g_localizeStrings.Get(742), SUBTITLE_COLOR_WHITE);
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
			pControl->AddLabel(g_localizeStrings.Get(106), LED_PLAYBACK_OFF);					// No
			pControl->AddLabel(g_localizeStrings.Get(13002), LED_PLAYBACK_VIDEO);			// Video Only
			pControl->AddLabel(g_localizeStrings.Get(475), LED_PLAYBACK_MUSIC);				// Music Only
			pControl->AddLabel(g_localizeStrings.Get(476), LED_PLAYBACK_VIDEO_MUSIC);	// Video & Music
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
		iPosY+=iGapY;
	}
	// fix first and last navigation
	CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START_CONTROL + (int)m_vecSettings.size()-1);
	if (pControl) pControl->SetNavigation(pControl->GetControlIdUp(), CONTROL_START_CONTROL,
																				pControl->GetControlIdLeft(), pControl->GetControlIdRight());
	pControl = (CGUIControl *)GetControl(CONTROL_START_CONTROL);
	if (pControl) pControl->SetNavigation(CONTROL_START_CONTROL + (int)m_vecSettings.size()-1, pControl->GetControlIdDown(),
																				pControl->GetControlIdLeft(), pControl->GetControlIdRight());
	// update our settings (turns controls on/off as appropriate)
	UpdateSettings();
}

void CGUIWindowSettingsCategory::UpdateSettings()
{
	for (unsigned int i=0; i<m_vecSettings.size(); i++)
	{
		CBaseSettingControl *pSettingControl = m_vecSettings[i];
		pSettingControl->Update();
		CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
		if (strSetting == "Pictures.AutoSwitchUseLargeThumbs" || strSetting == "Pictures.AutoSwitchMethod")
		{
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Pictures.UseAutoSwitching"));
		}
		else if (strSetting == "Pictures.AutoSwitchPercentage")
		{	// set visibility based on our other setting...
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Pictures.UseAutoSwitching") && g_guiSettings.GetInt("Pictures.AutoSwitchMethod") == 2);
		}
		else if (strSetting == "ProgramsLists.AutoSwitchUseLargeThumbs" || strSetting == "ProgramsLists.AutoSwitchMethod")
		{
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("ProgramsLists.UseAutoSwitching"));
		}
		else if (strSetting == "ProgramsLists.AutoSwitchPercentage")
		{	// set visibility based on our other setting...
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("ProgramsLists.UseAutoSwitching") && g_guiSettings.GetInt("ProgramsLists.AutoSwitchMethod") == 2);
		}
		else if (strSetting == "MusicLists.AutoSwitchUseLargeThumbs" || strSetting == "MusicLists.AutoSwitchMethod")
		{
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MusicLists.UseAutoSwitching"));
		}
		else if (strSetting == "MusicLists.AutoSwitchPercentage")
		{	// set visibility based on our other setting...
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("MusicLists.UseAutoSwitching") && g_guiSettings.GetInt("MusicLists.AutoSwitchMethod") == 2);
		}
		else if (strSetting == "VideoLists.AutoSwitchUseLargeThumbs" || strSetting == "VideoLists.AutoSwitchMethod")
		{
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VideoLists.UseAutoSwitching"));
		}
		else if (strSetting == "VideoLists.AutoSwitchPercentage")
		{	// set visibility based on our other setting...
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("VideoLists.UseAutoSwitching") && g_guiSettings.GetInt("VideoLists.AutoSwitchMethod") == 2);
		}
		else if (strSetting == "CDDARipper.Quality")
		{	// only visible if we are doing non-WAV ripping
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("CDDARipper.Encoder") != CDDARIP_ENCODER_WAV);
		}
		else if (strSetting == "CDDARipper.Bitrate")
		{	// only visible if we are ripping to CBR
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled((g_guiSettings.GetInt("CDDARipper.Encoder") != CDDARIP_ENCODER_WAV) &&
																					(g_guiSettings.GetInt("CDDARipper.Quality") == CDDARIP_QUALITY_CBR));
		}
		else if (strSetting == "AudioOutput.OutputToAllSpeakers" || strSetting == "AudioOutput.PCMPassThrough"
					|| strSetting == "AudioOutput.AC3PassThrough" || strSetting == "AudioOutput.DTSPassThrough")
		{	// only visible if we are in digital mode
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL);
		}
		else if (strSetting == "System.FanSpeed")
		{	// only visible if we have fancontrolspeed enabled
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("System.FanSpeedControl"));
		}
		else if (strSetting == "System.TargetTemperature")
		{	// only visible if we have autotemperature enabled
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("System.AutoTemperature"));
		}
		else if (strSetting == "System.RemotePlayHDSpinDownDelay")
		{	// only visible if we have spin down enabled
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.RemotePlayHDSpinDown") != SPIN_DOWN_NONE);
		}
		else if (strSetting == "System.RemotePlayHDSpinDownMinDuration")
		{	// only visible if we have spin down enabled
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.RemotePlayHDSpinDown") != SPIN_DOWN_NONE);
		}
		else if (strSetting == "System.ShutDownWhilePlaying")
		{	// only visible if we have shutdown enabled
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetInt("System.ShutDownTime") != 0);
		}
		else if (strSetting == "Servers.WebServerPassword")
		{	// Fill in a blank pass if we don't have it
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
		else if (strSetting == "Servers.TimeAddress")
		{
			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
			if (pControl) pControl->SetEnabled(g_guiSettings.GetBool("Servers.TimeServer"));
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
    else if (strSetting == "PostProcessing.VerticalDeBlocking" || strSetting == "PostProcessing.HorizontalDeBlocking" ||
							strSetting == "PostProcessing.AutoBrightnessContrastLevels" || strSetting == "PostProcessing.DeRing")
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
			pControl->SetEnabled(CUtil::IsUsingTTFSubtitles() && g_charsetConverter.isBidiCharset(g_guiSettings.GetString("Subtitles.CharSet")));
		}
		else if (strSetting == "LookAndFeel.CharSet")
		{	// TODO: Determine whether we are using a TTF font or not.
//			CGUIControl *pControl = (CGUIControl *)GetControl(pSettingControl->GetID());
//			if (pControl) pControl->SetEnabled(g_guiSettings.GetString("LookAndFeel.Font").Right(4) == ".ttf");
		}
		else if (strSetting == "ScreenSaver.DimLevel")
		{
			CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
			pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode")=="Dim");
		}
		else if (strSetting == "ScreenSaver.Preview")
		{
			CGUIControl *pControl = (CGUIControl *)GetControl(GetSetting(strSetting)->GetID());
			pControl->SetEnabled(g_guiSettings.GetString("ScreenSaver.Mode")!="None");
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
	}
}

void CGUIWindowSettingsCategory::OnClick(CBaseSettingControl *pSettingControl)
{
	CStdString strSetting = pSettingControl->GetSetting()->GetSetting();
	if (strSetting.Left(16) == "Weather.AreaCode")
	{
		CStdString strSearch;
		if(CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(14024), false))
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
	{	// delete the program database.
		// TODO: Should this actually be done here??
		CStdString programDatabase=g_stSettings.m_szAlbumDirectory;
		programDatabase+=PROGRAM_DATABASE_NAME;
		if (CUtil::FileExists(programDatabase))
			::DeleteFile(programDatabase.c_str());
	}
	else if (strSetting == "MyMusic.Visualisation")
	{	// new visualisation choosen...
		CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
		CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
		g_guiSettings.SetString("MyMusic.Visualisation", pControl->GetCurrentLabel()+".vis");
	}
	else if (strSetting == "MyMusic.Repeat")
	{
		g_playlistPlayer.Repeat(PLAYLIST_MUSIC_TEMP, g_guiSettings.GetBool("MyMusic.Repeat"));
	}
	else if (strSetting == "MusicLibrary.Cleanup")
	{
		g_musicDatabase.Clean();
	}
	else if (strSetting == "MusicLibrary.DeleteAlbumInfo")
	{
		g_musicDatabase.DeleteAlbumInfo();
		g_musicDatabase.Close();
	}
	else if (strSetting == "MusicLibrary.DeleteCDDBInfo")
	{
		g_musicDatabase.DeleteCDDBInfo();
	}
	else if (strSetting == "AudioOutput.OutputToAllSpeakers")
	{
		CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
		if (pSetting->GetData()) g_guiSettings.SetBool("AudioOutput.PCMPassThrough", false);
	}
	else if (strSetting == "AudioOutput.PCMPassThrough")
	{
		CSettingBool *pSetting = (CSettingBool*)pSettingControl->GetSetting();
		if (pSetting->GetData()) g_guiSettings.SetBool("AudioOutput.OutputToAllSpeakers", false);
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
    g_lcd=factory.Create();
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
		CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControlID,0,0,NULL);
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
	else if (strSetting == "Servers.TimeServer" || strSetting == "Servers.TimeAddress")
	{
		g_application.StopTimeServer();
		if (g_guiSettings.GetBool("Servers.TimeServer"))
			g_application.StartTimeServer();
	}
	else if (strSetting == "Servers.FTPServer")
	{
		g_application.StopFtpServer();
		if (g_guiSettings.GetBool("Servers.FTPServer"))
			g_application.StartFtpServer();
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
			if (strPassword.size()>0)
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
	{	// activate the video calibration screen
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
	{	// new font choosen...
		CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
		CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
		CStdString strSkinFontSet = pControl->GetCurrentLabel();
		if (strSkinFontSet != "CVS" && strSkinFontSet != g_guiSettings.GetString("LookAndFeel.Font"))
		{
			m_strNewSkinFontSet=strSkinFontSet;
			g_application.DelayLoadSkin();
		}
		else
		{	//	Do not reload the language we are already using
			m_strNewSkinFontSet.Empty();
			g_application.CancelDelayLoadSkin();
		}
	}
	else if (strSetting == "LookAndFeel.Skin")
	{	// new skin choosen...
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
				m_strNewSkin=strSkin;
				g_application.DelayLoadSkin();
			}
			else
			{	//	Do not reload the skin we are already using
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
	else if (strSetting == "LookAndFeel.GUICentering")
	{	// activate the video calibration screen
		m_gWindowManager.ActivateWindow(WINDOW_UI_CALIBRATION);
	}
	else if (strSetting == "LookAndFeel.Resolution")
	{	// new resolution choosen... - update if necessary
    CSettingInt *pSettingInt = (CSettingInt *)pSettingControl->GetSetting();
    int iControlID = pSettingControl->GetID();
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControlID,0,0,NULL);
    g_graphicsContext.SendMessage(msg);
    m_NewResolution = (RESOLUTION)msg.GetParam1();
    // delay change of resolution
    if (m_NewResolution != m_OldResolution)
    {
      m_dwResTime = timeGetTime() + 2000;
    }
	}
	else if (strSetting == "LookAndFeel.Language")
	{	// new language choosen...
		CSettingString *pSettingString = (CSettingString *)pSettingControl->GetSetting();
		CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(pSettingControl->GetID());
		CStdString strLanguage = pControl->GetCurrentLabel();
		if (strLanguage != "CVS" && strLanguage != pSettingString->GetData())
		{
			m_strNewLanguage = strLanguage;
			g_application.DelayLoadSkin();
		}
		else
		{	//	Do not reload the language we are already using
			m_strNewLanguage.Empty();
			g_application.CancelDelayLoadSkin();
		}
	}
	else if (strSetting == "UIFilters.Flicker" || strSetting == "UIFilters.Soften")
	{	// reset display
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
	{	// Alter LED Colour immediately
		ILED::CLEDControl(((CSettingInt *)pSettingControl->GetSetting())->GetData());
	}
	g_settings.Save();
	UpdateSettings();
}

void CGUIWindowSettingsCategory::FreeControls()
{
	// free any created controls
	for (unsigned int i=0; i<m_vecSections.size(); i++)
	{
		CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START_BUTTONS+i);
		Remove(CONTROL_START_BUTTONS+i);
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
	if (m_vecGroups.size()>1)
		m_vecGroups.erase(m_vecGroups.begin()+1);
	for (unsigned int i=0; i<m_vecSettings.size(); i++)
	{
		CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START_CONTROL+i);
		Remove(CONTROL_START_CONTROL+i);
		if (pControl)
		{
      pControl->FreeResources();
			delete pControl;
		}
		delete m_vecSettings[i];
	}
	m_vecSettings.clear();
}

void CGUIWindowSettingsCategory::AddSetting(CSetting *pSetting, int iPosX, int iPosY, int iWidth, int iControlID)
{
	CBaseSettingControl *pSettingControl = NULL;
	CGUIControl *pControl = NULL;
	if (pSetting->GetControlType() == CHECKMARK_CONTROL)
	{
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
		if (!pControl) return;
		((CGUIRadioButtonControl *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
		pControl->SetPosition(iPosX, iPosY);
		pControl->SetWidth(iWidth);
		pSettingControl = new CRadioButtonSettingControl((CGUIRadioButtonControl *)pControl, iControlID, pSetting);
	}
	else if (pSetting->GetControlType() == SPIN_CONTROL_FLOAT || pSetting->GetControlType() == SPIN_CONTROL_INT_PLUS || pSetting->GetControlType() == SPIN_CONTROL_TEXT || pSetting->GetControlType() == SPIN_CONTROL_INT)
	{	
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return;
	  pControl->SetPosition(iPosX, iPosY);
		pControl->SetWidth(iWidth);
		((CGUISpinControlEx *)pControl)->SetLabel(g_localizeStrings.Get(pSetting->GetLabel()));
		pControl->SetWidth(iWidth);
		pSettingControl = new CSpinExSettingControl((CGUISpinControlEx *)pControl, iControlID, pSetting);
	}
	else // if (pSetting->GetControlType() == BUTTON_CONTROL)
	{
		pControl = new CGUIButtonControl(*m_pOriginalButton);
		if (!pControl) return;
		pControl->SetPosition(iPosX, iPosY);
		((CGUIButtonControl *)pControl)->SetTextAlign(XBFONT_CENTER_Y);
		((CGUIButtonControl *)pControl)->SetText(g_localizeStrings.Get(pSetting->GetLabel()));
		pControl->SetWidth(iWidth);
		pSettingControl = new CButtonSettingControl((CGUIButtonControl *)pControl, iControlID, pSetting);
	}
	pControl->SetNavigation(iControlID-1,
													iControlID+1,
													CONTROL_START_BUTTONS,
													CONTROL_START_BUTTONS);
	pControl->SetID(iControlID);
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
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iCtrlID,0,0,NULL);
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
			float fPosY = g_graphicsContext.GetHeight()*0.8f;
			float fPosX = g_graphicsContext.GetWidth()*0.5f;
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
		return;
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
	{	// our network settings have changed - we should prompt the user to reset XBMC
		CGUIDialogYesNo *dlg = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
		if (!dlg) return;
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
/*			g_guiSettings.SetInt("Network.Assignment", m_iNetworkAssignment);
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
	{	// easy - just fill as per usual
		CStdString strLabel;
		for (int i=pSettingInt->m_iMin; i <= pSettingInt->m_iMax; i+= pSettingInt->m_iStep)
		{
			if (pSettingInt->m_iFormat>-1)
			{
				CStdString strFormat=g_localizeStrings.Get(pSettingInt->m_iFormat);
				strLabel.Format(strFormat, i);
			}
			else
				strLabel.Format(pSettingInt->m_strFormat, i);
			pControl->AddLabel(strLabel, (i-pSettingInt->m_iMin)/pSettingInt->m_iStep);
		}
		pControl->SetValue((pSettingInt->GetData()-pSettingInt->m_iMin)/pSettingInt->m_iStep);
	}
	else
	{
		if (g_guiSettings.GetString("Subtitles.Font").size())
		{
			//find font sizes...
			CHDDirectory directory;	
			VECFILEITEMS items;
			CStdString strPath = "Q:\\system\\players\\mplayer\\font\\";
	    if(g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
      {
        strPath = "Q:\\mplayer\\font\\";
      }
			strPath+=g_guiSettings.GetString("Subtitles.Font");
			strPath+="\\";
			directory.GetDirectory(strPath,items);
			int iCurrentSize=0;
			int iSize=0;
			for (int i=0; i < (int)items.size(); ++i)
			{
				CFileItem* pItem=items[i];
				if (pItem->m_bIsFolder)
				{
					if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"CVS")==0) continue;
					int iSizeTmp=atoi(pItem->GetLabel().c_str());
					if (iSizeTmp==pSettingInt->GetData())
						iCurrentSize=iSize;
					pControl->AddLabel(pItem->GetLabel(), iSize++);
				}
				delete pItem;
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
	int iCurrentFont=0;
	int iFont=0;

	// Find mplayer fonts...
	{
		CHDDirectory directory;	
		VECFILEITEMS items;
		CStdString strPath = "Q:\\system\\players\\mplayer\\font\\";
	  if(g_guiSettings.GetBool("MyVideos.AlternateMPlayer"))
    {
      strPath = "Q:\\mplayer\\font\\";
    }
		directory.GetDirectory(strPath,items);
		for (int i=0; i < (int)items.size(); ++i)
		{
			CFileItem* pItem=items[i];
			if (pItem->m_bIsFolder)
			{
				if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"CVS")==0) continue;
				if (CUtil::cmpnocase(pItem->GetLabel().c_str(), pSettingString->GetData().c_str())==0)
					iCurrentFont=iFont;
				pControl->AddLabel(pItem->GetLabel(), iFont++);
			}
			delete pItem;
		}
	}

	// find TTF fonts
	{
		CHDDirectory directory;	
		VECFILEITEMS items;
		CStdString strPath = "Q:\\media\\fonts\\";
		if (directory.GetDirectory(strPath,items))
		{
			for (int i=0; i < (int)items.size(); ++i)
			{
				CFileItem* pItem=items[i];

				if (!pItem->m_bIsFolder)
				{
					char* extension = CUtil::GetExtension(pItem->GetLabel().c_str());
					if (stricmp(extension, ".ttf") != 0) continue;
					if (CUtil::cmpnocase(pItem->GetLabel().c_str(), pSettingString->GetData().c_str())==0)
						iCurrentFont=iFont;

					pControl->AddLabel(pItem->GetLabel(), iFont++);
				}

				delete pItem;
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
		CLog::Log(LOGERROR, "Couldn't load %s",strPath.c_str());
		return ;
  }

  TiXmlElement* pRootElement =xmlDoc.RootElement();

  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("fonts")) 
  {
    CLog::Log(LOGERROR, "file %s doesnt start with <fonts>",strPath.c_str());
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
					if (CUtil::cmpnocase(idAttr, g_guiSettings.GetString("LookAndFeel.Font").c_str())==0)
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
	VECFILEITEMS items;
	CStdString strPath = "Q:\\skin\\";
	directory.GetDirectory(strPath,items);

	int iCurrentSkin=0;
	int iSkin=0;
	vector<CStdString> vecSkins;
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		if (pItem->m_bIsFolder)
		{
			if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"CVS")==0) continue;
			if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"fonts")==0) continue;
			if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"media")==0) continue;
//			if (g_SkinInfo.Check(pItem->m_strPath))
//			{
				vecSkins.push_back(pItem->GetLabel());
//			}
		}
		delete pItem;
	}

	sort(vecSkins.begin(),vecSkins.end(),sortstringbyname());
	for (i=0; i < (int) vecSkins.size(); ++i)
	{
		CStdString strSkin=vecSkins[i];
		if (CUtil::cmpnocase(strSkin.c_str(), g_guiSettings.GetString("LookAndFeel.Skin").c_str())==0)
		{
			iCurrentSkin=iSkin;
		}
		pControl->AddLabel(strSkin, iSkin++);
	}
	pControl->SetValue(iCurrentSkin);
	return;
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

	sort(vecCharsets.begin(),vecCharsets.end(),sortstringbyname());

	for (int i = 0; i < (int) vecCharsets.size(); ++i)
	{
		CStdString strCharsetLabel = vecCharsets[i];

		if (strCharsetLabel == strCurrentCharsetLabel)
			iCurrentCharset=i;

		pControl->AddLabel(strCharsetLabel, i);
	}

	pControl->SetValue(iCurrentCharset);
}

void CGUIWindowSettingsCategory::FillInVisualisations(CSetting *pSetting)
{
	CSettingString *pSettingString = (CSettingString*)pSetting;
	CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
	pControl->SetShowRange(true); // show the range
	int iCurrentVis=0;
	int iVis=0;
	vector<CStdString> vecVis;
	//find visz....
	CHDDirectory directory;	
	VECFILEITEMS items;
	CStdString strPath = "Q:\\visualisations\\";
	directory.GetDirectory(strPath,items);
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		if (!pItem->m_bIsFolder)
		{
			CStdString strExtension;
			CUtil::GetExtension(pItem->m_strPath,strExtension);
			if (strExtension==".vis")
			{
				CStdString strLabel=pItem->GetLabel();
				vecVis.push_back(strLabel.Mid(0, strLabel.size()-4));
			}
		}
		delete pItem;
	}

	CStdString strDefaultVis = pSettingString->GetData();
	strDefaultVis.Delete(strDefaultVis.size()-4, 4);

	sort(vecVis.begin(),vecVis.end(),sortstringbyname());
	for (int i=0; i < (int) vecVis.size(); ++i)
	{
		CStdString strVis=vecVis[i];

		if (CUtil::cmpnocase(strVis.c_str(), strDefaultVis.c_str())==0)
			iCurrentVis=iVis;

		pControl->AddLabel(strVis, iVis++);
	}
	
	pControl->SetValue(iCurrentVis);
}

void CGUIWindowSettingsCategory::FillInResolutions(CSetting *pSetting)
{
	CSettingInt *pSettingInt = (CSettingInt*)pSetting;
	CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
	pControl->Clear();
	// Find the valid resolutions and add them as necessary
	vector<RESOLUTION> res;
	g_graphicsContext.GetAllowedResolutions(res);
	int iCurrentRes=0;
	int iLabel=0;
	for (vector<RESOLUTION>::iterator it=res.begin(); it !=res.end();it++)
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
		if (pSettingInt->GetData()==res)
			iCurrentRes=iLabel;
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
	VECFILEITEMS items;

	CStdString strPath = "Q:\\language\\";
	directory.GetDirectory(strPath,items);

	int iCurrentLang=0;
	int iLanguage=0;
	vector<CStdString> vecLanguage;
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		if (pItem->m_bIsFolder)
		{
			if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"CVS")==0) continue;
			if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"fonts")==0) continue;
			if (CUtil::cmpnocase(pItem->GetLabel().c_str(),"media")==0) continue;
			vecLanguage.push_back(pItem->GetLabel());
		}
		delete pItem;
	}

	sort(vecLanguage.begin(),vecLanguage.end(),sortstringbyname());
	for (i=0; i < (int) vecLanguage.size(); ++i)
	{
		CStdString strLanguage=vecLanguage[i];
		if (CUtil::cmpnocase(strLanguage.c_str(), pSettingString->GetData().c_str())==0)
			iCurrentLang=iLanguage;
		pControl->AddLabel(strLanguage, iLanguage++);
	}

	pControl->SetValue(iCurrentLang);
}

void CGUIWindowSettingsCategory::FillInScreenSavers(CSetting *pSetting)
{	// Screensaver mode
	CSettingString *pSettingString = (CSettingString*)pSetting;
	CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(GetSetting(pSetting->GetSetting())->GetID());
	pControl->Clear();

	pControl->AddLabel(g_localizeStrings.Get(351), 0);	// Off
	pControl->AddLabel(g_localizeStrings.Get(352), 1);	// Dim
	pControl->AddLabel(g_localizeStrings.Get(353), 2);	// Black

	//find screensavers ....
	CHDDirectory directory;	
	VECFILEITEMS items;
	CStdString strPath = "Q:\\screensavers\\";
	directory.GetDirectory(strPath,items);

	int iCurrentScr = -1;
	int iScr = 0;
	vector<CStdString> vecScr;
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		if (!pItem->m_bIsFolder)
		{
			CStdString strExtension;
			CUtil::GetExtension(pItem->m_strPath,strExtension);
			if (strExtension==".xbs")
			{
				CStdString strLabel=pItem->GetLabel();
				vecScr.push_back(strLabel.Mid(0, strLabel.size()-4));
			}
		}
		delete pItem;
	}

	CStdString strDefaultScr = pSettingString->GetData();
	CStdString strExtension;
	CUtil::GetExtension(strDefaultScr, strExtension);
	if (strExtension == ".xbs")
		strDefaultScr.Delete(strDefaultScr.size()-4, 4);

	sort(vecScr.begin(),vecScr.end(),sortstringbyname());
	for (i=0; i < (int) vecScr.size(); ++i)
	{
		CStdString strScr=vecScr[i];

		if (CUtil::cmpnocase(strScr.c_str(), strDefaultScr.c_str())==0)
			iCurrentScr=i+3;

		pControl->AddLabel(strScr, i+3);
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

CBaseSettingControl *CGUIWindowSettingsCategory::GetSetting(const CStdString &strSetting)
{
	for (unsigned int i=0; i<m_vecSettings.size(); i++)
	{
		if (m_vecSettings[i]->GetSetting()->GetSetting() == strSetting)
			return m_vecSettings[i];
	}
	return NULL;
}

