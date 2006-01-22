#include "stdafx.h"
#include "GUIDialogVideoSettings.h"
#include "GUIWindowSettingsCategory.h"
#include "GUIDialogFileBrowser.h"
#include "util.h"
#include "utils/GUIInfoManager.h"
#include "application.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "VideoDatabase.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_NONE_AVAILABLE      3
#define CONTROL_AREA                5
#define CONTROL_GAP                 6
#define CONTROL_DEFAULT_BUTTON      7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN        9
#define CONTROL_DEFAULT_SLIDER     10
#define CONTROL_DEFAULT_SEPARATOR  11
#define CONTROL_START              50
#define CONTROL_PAGE               60

extern void xbox_audio_switch_channel(int iAudioStream, bool bAudioOnAllSpeakers); //lowlevel audio

CGUIDialogVideoSettings::CGUIDialogVideoSettings(void)
    : CGUIDialog(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "VideoOSDSettings.xml")
{
  m_iLastControl = -1;
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalSettingsButton = NULL;
  m_pOriginalSlider = NULL;
  m_pOriginalSeparator = NULL;
  m_iCurrentPage = 0;
  m_iNumPages = 0;
  m_iNumPerPage = 0;
  m_loadOnDemand = false;
  m_dwIDRange = 2;
}

CGUIDialogVideoSettings::~CGUIDialogVideoSettings(void)
{
}

bool CGUIDialogVideoSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_PAGE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
        OnMessage(msg);
        m_iCurrentPage = msg.GetParam1() - 1;
        SetupPage();
      }
      else if (iControl >= CONTROL_START && iControl < CONTROL_PAGE)
        OnClick(iControl);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_iScreen = message.GetParam2();

      CreateSettings();

      m_iCurrentPage = 0;
      m_iNumPages = 0;

      SetupPage();

      SET_CONTROL_FOCUS(CONTROL_START, 0);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      FreeControls();
      m_settings.clear();
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVideoSettings::SetupPage()
{
  // cleanup first, if necessary
  FreeControls();
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalSettingsButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  m_pOriginalSlider = (CGUISettingsSliderControl *)GetControl(CONTROL_DEFAULT_SLIDER);
  m_pOriginalSeparator = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalSettingsButton || !m_pOriginalSlider || !m_pOriginalSeparator)
    return;
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalSettingsButton->SetVisible(false);
  m_pOriginalSlider->SetVisible(false);
  m_pOriginalSeparator->SetVisible(false);

  // update our settings label
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, g_localizeStrings.Get(13395 + m_iScreen - WINDOW_DIALOG_VIDEO_OSD_SETTINGS));

  // our controls for layout...
  const CGUIControl *pControlArea = GetControl(CONTROL_AREA);
  const CGUIControl *pControlGap = GetControl(CONTROL_GAP);
  if (!pControlArea || !pControlGap)
    return;

  if (!m_settings.size())
  { // no settings available
    SET_CONTROL_VISIBLE(CONTROL_NONE_AVAILABLE);
    SET_CONTROL_HIDDEN(CONTROL_PAGE);
    return;
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_NONE_AVAILABLE);
    SET_CONTROL_VISIBLE(CONTROL_PAGE);
  }

  int iPosX = pControlArea->GetXPosition();
  int iWidth = pControlArea->GetWidth();
  int iPosY = pControlArea->GetYPosition();
  int iGapY = pControlGap->GetHeight();
  int numSettings = 0;
  for (unsigned int i=0; i < m_settings.size(); i++)
    if (m_settings[i].type != VideoSetting::SEPARATOR)
      numSettings++;

  int numPerPage = (int)pControlArea->GetHeight() / iGapY;
  if (numPerPage < 1) numPerPage = 1;
  m_iNumPages = (numSettings + numPerPage - 1)/ numPerPage; // round up
  if (m_iCurrentPage >= m_iNumPages - 1)
    m_iCurrentPage = m_iNumPages - 1;
  if (m_iCurrentPage <= 0)
    m_iCurrentPage = 0;
  CGUISpinControl *pPage = (CGUISpinControl *)GetControl(CONTROL_PAGE);
  if (pPage)
  {
    pPage->SetRange(1, m_iNumPages);
    pPage->SetReverse(false);
    pPage->SetShowRange(true);
    pPage->SetValue(m_iCurrentPage + 1);
    pPage->SetVisible(m_iNumPages > 1);
  }

  // find out page offset
  m_iPageOffset = 0;
  for (int i = 0; i < m_iCurrentPage; i++)
  {
    int j = 0;
    while (j < numPerPage)
    {
      if (m_settings[m_iPageOffset].type != VideoSetting::SEPARATOR)
        j++;
      m_iPageOffset++;
    }
  }
  // create our controls
  int numSettingsOnPage = 0;
  int numControlsOnPage = 0;
  for (unsigned int i = m_iPageOffset; i < m_settings.size(); i++)
  {
    VideoSetting &setting = m_settings.at(i);
    AddSetting(setting, iPosX, iPosY, iWidth, CONTROL_START + i - m_iPageOffset);
    if (setting.type == VideoSetting::SEPARATOR)
      iPosY += m_pOriginalSeparator->GetHeight();
    else
    {
      iPosY += iGapY;
      numSettingsOnPage++;
    }
    numControlsOnPage++;
    if (numSettingsOnPage == numPerPage)
      break;
  }
  // fix first and last navigation
  CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START + numControlsOnPage - 1);
  if (pControl) pControl->SetNavigation(pControl->GetControlIdUp(), CONTROL_PAGE,
                                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(CONTROL_START);
  if (pControl) pControl->SetNavigation(CONTROL_PAGE, pControl->GetControlIdDown(),
                                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(CONTROL_PAGE);
  if (pControl) pControl->SetNavigation(CONTROL_START + numControlsOnPage - 1, CONTROL_START,
                                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
}


void CGUIDialogVideoSettings::UpdateSetting(unsigned int num)
{
  unsigned int settingNum = 0;
  for (unsigned int i = 0; i < m_settings.size(); i++)
  {
    if (m_settings[i].id == num)
    {
      settingNum = i;
      break;
    }
  }
  VideoSetting &setting = m_settings.at(settingNum);
  unsigned int controlID = settingNum + CONTROL_START + m_iPageOffset;
  if (setting.type == VideoSetting::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetValue(*(int *)setting.data);
  }
  else if (setting.type == VideoSetting::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetSelected(*(bool *)setting.data);
  }
  else if (setting.type == VideoSetting::SLIDER)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetFloatValue(*(float *)setting.data);
  }
  else if (setting.type == VideoSetting::SLIDER_INT)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetIntValue(*(int *)setting.data);
  }
}

void CGUIDialogVideoSettings::OnClick(int iID)
{
  unsigned int settingNum = iID - CONTROL_START + m_iPageOffset;
  if (settingNum >= m_settings.size()) return;
  VideoSetting &setting = m_settings.at(settingNum);
  if (setting.type == VideoSetting::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(iID);
    if (setting.data) *(int *)setting.data = pControl->GetValue();
  }
  else if (setting.type == VideoSetting::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(iID);
    if (setting.data) *(bool *)setting.data = pControl->IsSelected();
  }
  else if (setting.type == VideoSetting::SLIDER)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(iID);
    if (setting.data) *(float *)setting.data = pControl->GetFloatValue();
  }
  else if (setting.type == VideoSetting::SLIDER_INT)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(iID);
    if (setting.data) *(int *)setting.data = pControl->GetIntValue();
  }
  OnSettingChanged(settingNum);
}

void CGUIDialogVideoSettings::FreeControls()
{
  // free any created controls
  for (unsigned int i = CONTROL_START; i < CONTROL_PAGE; i++)
  {
    CGUIControl *pControl = (CGUIControl *)GetControl(i);
    if (pControl)
    {
      Remove(i);
      pControl->FreeResources();
      delete pControl;
    }
  }
}

void CGUIDialogVideoSettings::AddSetting(VideoSetting &setting, int iPosX, int iPosY, int iWidth, int iControlID)
{
  CGUIControl *pControl = NULL;
  if (setting.type == VideoSetting::BUTTON)
  {
    pControl = new CGUIButtonControl(*m_pOriginalSettingsButton);
    if (!pControl) return ;
    ((CGUIButtonControl *)pControl)->SetText(setting.name);
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
  }
  else if (setting.type == VideoSetting::SEPARATOR)
  {
    pControl = new CGUIImage(*m_pOriginalSeparator);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
  }
  else if (setting.type == VideoSetting::CHECK)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetText(setting.name);
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    if (setting.data) pControl->SetSelected(*(bool *)setting.data == 1);
  }
  else if (setting.type == VideoSetting::SPIN && setting.entry.size() > 0)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    ((CGUISpinControlEx *)pControl)->SetText(setting.name);
    pControl->SetWidth(iWidth);
    for (unsigned int i = 0; i < setting.entry.size(); i++)
      ((CGUISpinControlEx *)pControl)->AddLabel(setting.entry[i], i);
    if (setting.data) ((CGUISpinControlEx *)pControl)->SetValue(*(int *)setting.data);
  }
  else if (setting.type == VideoSetting::SLIDER || setting.type == VideoSetting::SLIDER_INT)
  {
    pControl = new CGUISettingsSliderControl(*m_pOriginalSlider);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    ((CGUISettingsSliderControl *)pControl)->SetText(setting.name);
    if (setting.type == VideoSetting::SLIDER)
    {
      ((CGUISettingsSliderControl *)pControl)->SetFormatString(setting.format);
      ((CGUISettingsSliderControl *)pControl)->SetType(SPIN_CONTROL_TYPE_FLOAT);
      ((CGUISettingsSliderControl *)pControl)->SetFloatRange(setting.min, setting.max);
      ((CGUISettingsSliderControl *)pControl)->SetFloatInterval(setting.interval);
      if (setting.data) ((CGUISettingsSliderControl *)pControl)->SetFloatValue(*(float *)setting.data);
    }
    else
    {
      ((CGUISettingsSliderControl *)pControl)->SetType(SPIN_CONTROL_TYPE_INT);
      ((CGUISettingsSliderControl *)pControl)->SetRange((int)setting.min, (int)setting.max);
      if (setting.data) ((CGUISettingsSliderControl *)pControl)->SetIntValue(*(int *)setting.data);
    }
  }
  if (!pControl) return;
  pControl->SetNavigation(iControlID - 1,
                          iControlID + 1,
                          iControlID,
                          iControlID);
  pControl->SetID(iControlID);
  pControl->SetVisible(true);
  Add(pControl);
  pControl->AllocResources();
}

void CGUIDialogVideoSettings::AddButton(unsigned int id, int label)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = VideoSetting::BUTTON;
  setting.data = NULL;
  m_settings.push_back(setting);
}

void CGUIDialogVideoSettings::AddBool(unsigned int id, int label, bool *on)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = VideoSetting::CHECK;
  setting.data = on;
  m_settings.push_back(setting);
}

void CGUIDialogVideoSettings::AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = VideoSetting::SPIN;
  setting.data = current;
  for (unsigned int i = 0; i < max; i++)
    setting.entry.push_back(g_localizeStrings.Get(entries[i]));
  m_settings.push_back(setting);
}

void CGUIDialogVideoSettings::AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = VideoSetting::SPIN;
  setting.data = current;
  for (unsigned int i = min; i <= max; i++)
  {
    CStdString format;
    format.Format("%i", i);
    setting.entry.push_back(format);
  }
  m_settings.push_back(setting);
}

void CGUIDialogVideoSettings::AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, const char *format /*= NULL*/)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = VideoSetting::SLIDER;
  setting.min = min;
  setting.interval = interval;
  setting.max = max;
  setting.data = current;
  if (format) setting.format = format;
  m_settings.push_back(setting);
}

void CGUIDialogVideoSettings::AddSlider(unsigned int id, int label, int *current, int min, int max)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = VideoSetting::SLIDER_INT;
  setting.min = (float)min;
  setting.max = (float)max;
  setting.data = current;
  m_settings.push_back(setting);
}

void CGUIDialogVideoSettings::AddSeparator(unsigned int id)
{
  VideoSetting setting;
  setting.id = id;
  setting.type = VideoSetting::SEPARATOR;
  setting.data = NULL;
  m_settings.push_back(setting);
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
#define VIDEO_SETTINGS_NON_INTERLEAVED    14
#define VIDEO_SETTINGS_NO_CACHE           15
#define VIDEO_SETTINGS_FORCE_INDEX        16

#define AUDIO_SETTINGS_VOLUME             1
#define AUDIO_SETTINGS_VOLUME_AMPLIFICATION 2
#define AUDIO_SETTINGS_DELAY              3
#define AUDIO_SETTINGS_STREAM             4
// separator 5
#define SUBTITLE_SETTINGS_ENABLE          6
#define SUBTITLE_SETTINGS_DELAY           7
#define SUBTITLE_SETTINGS_STREAM          8
#define SUBTITLE_SETTINGS_BROWSER         9

void CGUIDialogVideoSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  if (m_iScreen == WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
  { // Video settings
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

    {
      const int entries[] = { 16018, 16019, 16020, 16021, 16022 };
      AddSpin(VIDEO_SETTINGS_INTERLACEMETHOD, 16023, (int*)&g_stSettings.m_currentVideoSettings.m_InterlaceMethod, 5, entries);
    }

    AddSeparator(8);
    AddButton(VIDEO_SETTINGS_MAKE_DEFAULT, 12376);
    m_flickerFilter = g_guiSettings.GetInt("Filters.Flicker");
    AddSpin(VIDEO_SETTINGS_FLICKER, 13100, &m_flickerFilter, 0, 5);
    m_soften = g_guiSettings.GetBool("Filters.Soften");
    AddBool(VIDEO_SETTINGS_SOFTEN, 215, &m_soften);
    AddButton(VIDEO_SETTINGS_CALIBRATION, 214);
    AddBool(VIDEO_SETTINGS_NON_INTERLEAVED, 306, &g_stSettings.m_currentVideoSettings.m_NonInterleaved);
    AddBool(VIDEO_SETTINGS_NO_CACHE, 431, &g_stSettings.m_currentVideoSettings.m_NoCache);
    AddButton(VIDEO_SETTINGS_FORCE_INDEX, 12009);
  }
  else
  { // Audio and Subtitle settings
    m_volume = g_stSettings.m_nVolumeLevel * 0.01f;
    AddSlider(AUDIO_SETTINGS_VOLUME, 13376, &m_volume, VOLUME_MINIMUM * 0.01f, (VOLUME_MAXIMUM - VOLUME_MINIMUM) * 0.0001f, VOLUME_MAXIMUM * 0.01f, "%2.1f dB");
    AddSlider(AUDIO_SETTINGS_VOLUME_AMPLIFICATION, 290, &g_stSettings.m_currentVideoSettings.m_VolumeAmplification, 0, 5, 30, "%2.1f dB");
    AddSlider(AUDIO_SETTINGS_DELAY, 297, &g_stSettings.m_currentVideoSettings.m_AudioDelay, -g_advancedSettings.m_videoAudioDelayRange, 0.1f, g_advancedSettings.m_videoAudioDelayRange, "%2.1fs");
    AddAudioStreams(AUDIO_SETTINGS_STREAM);
    AddSeparator(5);
    AddBool(SUBTITLE_SETTINGS_ENABLE, 13397, &g_stSettings.m_currentVideoSettings.m_SubtitleOn);
    AddSlider(SUBTITLE_SETTINGS_DELAY, 303, &g_stSettings.m_currentVideoSettings.m_SubtitleDelay, -g_advancedSettings.m_videoSubsDelayRange, 0.1f, g_advancedSettings.m_videoSubsDelayRange, "%2.1fs");
    AddSubtitleStreams(SUBTITLE_SETTINGS_STREAM);
    AddButton(SUBTITLE_SETTINGS_BROWSER,13250);
    AddButton(VIDEO_SETTINGS_MAKE_DEFAULT, 12376);
  }
}

void CGUIDialogVideoSettings::AddAudioStreams(unsigned int id)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(460);
  setting.type = VideoSetting::SPIN;
  setting.min = 0;
  setting.data = &m_audioStream;

  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetAudioStreamCount() - 1;
  m_audioStream = g_application.m_pPlayer->GetAudioStream();

  if( m_audioStream < 0 ) m_audioStream = 0;

  // check if we have a single, stereo stream, and if so, allow us to split into
  // left, right or both
  if (!setting.max)
  {
    CStdString strAudioInfo;
    g_application.m_pPlayer->GetAudioInfo(strAudioInfo);
    int iNumChannels = atoi(strAudioInfo.Right(strAudioInfo.size() - strAudioInfo.Find("chns:") - 5).c_str());
    CStdString strAudioCodec = strAudioInfo.Mid(7, strAudioInfo.Find(") VBR") - 5);
    bool bDTS = strstr(strAudioCodec.c_str(), "DTS") != 0;
    bool bAC3 = strstr(strAudioCodec.c_str(), "AC3") != 0;
    if (iNumChannels == 2 && !(bDTS || bAC3))
    { // ok, enable these options
/*      if (g_stSettings.m_currentVideoSettings.m_AudioStream == -1)
      { // default to stereo stream
        g_stSettings.m_currentVideoSettings.m_AudioStream = 0;
      }*/
      setting.max = 2;
      for (int i = 0; i <= setting.max; i++)
        setting.entry.push_back(g_localizeStrings.Get(13320 + i));
      m_audioStream = -g_stSettings.m_currentVideoSettings.m_AudioStream - 1;
      m_settings.push_back(setting);
      return;
    }
  }

  // cycle through each audio stream and add it to our list control
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    g_application.m_pPlayer->GetAudioStreamName(i, strItem);
    if (strItem.length() == 0)
      strItem.Format("%2i", i + 1);
    setting.entry.push_back(strItem);
  }

  if( setting.max < 0 )
  {
    setting.max = 0;
    setting.entry.push_back(g_localizeStrings.Get(231).c_str());
  }

  m_settings.push_back(setting);
}

void CGUIDialogVideoSettings::AddSubtitleStreams(unsigned int id)
{
  VideoSetting setting;

  setting.id = id;
  setting.name = g_localizeStrings.Get(462);
  setting.type = VideoSetting::SPIN;
  setting.min = 0;
  setting.data = &m_subtitleStream;
  m_subtitleStream = g_application.m_pPlayer->GetSubtitle();

  if(m_subtitleStream < 0) m_subtitleStream = 0;

  // get the number of audio strams for the current movie
  setting.max = (float)g_application.m_pPlayer->GetSubtitleCount() - 1;

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i <= setting.max; ++i)
  {
    CStdString strItem;
    g_application.m_pPlayer->GetSubtitleName(i, strItem);
    if (strItem.length() == 0)
      strItem.Format("%2i", i + 1);
    setting.entry.push_back(strItem);
  }

  if (setting.max < 0)
  { // no subtitle streams - just add a "None" entry
    m_subtitleStream = 0;
    setting.max = 0;
    setting.entry.push_back(g_localizeStrings.Get(231).c_str());
  }
  m_settings.push_back(setting);
}


void CGUIDialogVideoSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  VideoSetting &setting = m_settings.at(num);
  // check and update anything that needs it
  if (m_iScreen == WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
  {
    if (setting.id == VIDEO_SETTINGS_NON_INTERLEAVED ||  setting.id == VIDEO_SETTINGS_NO_CACHE )
      g_application.Restart(true);
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
    else if (setting.id == VIDEO_SETTINGS_BRIGHTNESS || setting.id == VIDEO_SETTINGS_CONTRAST || setting.id == VIDEO_SETTINGS_GAMMA)
      CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
    else if (setting.id == VIDEO_SETTINGS_FLICKER || setting.id == VIDEO_SETTINGS_SOFTEN)
    {
      RESOLUTION res = g_graphicsContext.GetVideoResolution();
      g_guiSettings.SetInt("Filters.Flicker", m_flickerFilter);
      g_guiSettings.SetBool("Filters.Soften", m_soften);
      g_graphicsContext.SetVideoResolution(res);
    }
    else if (setting.id == VIDEO_SETTINGS_CALIBRATION)
    {
      // close the OSD
      CGUIDialog *pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_OSD);
      if (pDialog) pDialog->Close();
      // launch calibration window
      m_gWindowManager.ActivateWindow(WINDOW_MOVIE_CALIBRATION);
    }
    else if (setting.id == VIDEO_SETTINGS_FORCE_INDEX)
    {
      g_stSettings.m_currentVideoSettings.m_bForceIndex = true;
      g_application.Restart(true);
    }
  }
  else
  { // audio settings
    if (setting.id == AUDIO_SETTINGS_VOLUME)
    {
      g_stSettings.m_nVolumeLevel = (long)(m_volume * 100.0f);
      g_application.SetVolume(int(((float)(g_stSettings.m_nVolumeLevel - VOLUME_MINIMUM)) / (VOLUME_MAXIMUM - VOLUME_MINIMUM)*100.0f + 0.5f));
    }
    else if (setting.id == AUDIO_SETTINGS_VOLUME_AMPLIFICATION)
    {
      g_application.Restart(true);
    }
    else if (setting.id == AUDIO_SETTINGS_DELAY)
    {
      if (g_application.m_pPlayer)
        g_application.m_pPlayer->SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
    }
    else if (setting.id == AUDIO_SETTINGS_STREAM)
    {
      // first check if it's a stereo track that we can change between stereo, left and right
      if (g_application.m_pPlayer->GetAudioStreamCount() == 1)
      {
        if (setting.max == 2)
        { // we're in the case we want - call the code to switch channels etc.
          // update the screen setting...
          g_stSettings.m_currentVideoSettings.m_AudioStream = -1 - m_audioStream;
          // call monkeyh1's code here...
          bool bAudioOnAllSpeakers = (g_guiSettings.GetInt("AudioOutput.Mode") == AUDIO_DIGITAL) && g_guiSettings.GetBool("VideoPlayer.OutputToAllSpeakers");
          xbox_audio_switch_channel(m_audioStream, bAudioOnAllSpeakers);
          return;
        }
      }
      // only change the audio stream if a different one has been asked for
      if (g_application.m_pPlayer->GetAudioStream() != m_audioStream)
      {
        g_stSettings.m_currentVideoSettings.m_AudioStream = m_audioStream;
        g_application.m_pPlayer->SetAudioStream(m_audioStream);    // Set the audio stream to the one selected
      }
    }
    else if (setting.id == SUBTITLE_SETTINGS_ENABLE)
    {
      g_application.m_pPlayer->SetSubtitleVisible(g_stSettings.m_currentVideoSettings.m_SubtitleOn);
    }
    else if (setting.id == SUBTITLE_SETTINGS_DELAY)
      g_application.m_pPlayer->SetSubTitleDelay(g_stSettings.m_currentVideoSettings.m_SubtitleDelay);
    else if (setting.id == SUBTITLE_SETTINGS_STREAM && setting.max > 0)
    {
      g_stSettings.m_currentVideoSettings.m_SubtitleStream = m_subtitleStream;
      g_application.m_pPlayer->SetSubtitle(m_subtitleStream);
    }
    else if (setting.id == SUBTITLE_SETTINGS_BROWSER)
    {
      CStdString strPath="";
      const CStdString strMask = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar";
      if (CGUIDialogFileBrowser::ShowAndGetFile(g_settings.m_vecMyVideoShares,strMask,g_localizeStrings.Get(293),strPath)) // "subtitles"
      {
        CStdString strExt;
        CUtil::GetExtension(strPath,strExt);
        if (strExt.CompareNoCase(".rar") == 0)
        {
          std::vector<CStdString> vecExtensionsCached;
          CUtil::CacheRarSubtitles(vecExtensionsCached,strPath,"",".keep");
          g_application.Restart(true); // to reread subtitles
          CreateSettings();
          SetupPage();
          SET_CONTROL_FOCUS(CONTROL_START, 0);         
        }
        else
        {
          if (CFile::Cache(strPath,"z:\\subtitle"+strExt+".keep"))
          {
            CStdString strPath2;
            if (strExt.CompareNoCase(".idx") == 0)
            {
              CUtil::ReplaceExtension(strPath,".sub",strPath2);
              if (!CFile::Cache(strPath2,"z:\\subtitle.sub.keep"))
              {
                CUtil::ReplaceExtension(strPath,".rar",strPath2);
                std::vector<CStdString> vecExtensionsCached;
                CUtil::CacheRarSubtitles(vecExtensionsCached,strPath2,CUtil::GetFileName(strPath),".keep");
              }
            }
            if (strExt.CompareNoCase(".sub") == 0)
            {
              CUtil::ReplaceExtension(strPath,".idx",strPath2);
              CFile::Cache(strPath2,"z:\\subtitle.idx.keep");
            }
            g_application.Restart(true); // to reread subtitles
            CreateSettings();
            SetupPage();
            SET_CONTROL_FOCUS(CONTROL_START, 0);
          }
        }
      }
    }
  }
  if (setting.id == VIDEO_SETTINGS_MAKE_DEFAULT)
  {
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

bool CGUIDialogVideoSettings::HasID(DWORD dwID)
{
  return (dwID >= WINDOW_DIALOG_VIDEO_OSD_SETTINGS && dwID <= WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
}

void CGUIDialogVideoSettings::Render()
{
  if (m_iScreen == WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
  {
    m_volume = g_stSettings.m_nVolumeLevel * 0.01f;
    UpdateSetting(AUDIO_SETTINGS_VOLUME);
    if (g_application.m_pPlayer)
    {
      // these settings can change on the fly
      UpdateSetting(AUDIO_SETTINGS_DELAY);
      UpdateSetting(SUBTITLE_SETTINGS_ENABLE);
      UpdateSetting(SUBTITLE_SETTINGS_DELAY);
    }
  }
  CGUIDialog::Render();
}