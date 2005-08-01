#include "stdafx.h"
#include "GUIDialogVideoSettings.h"
#include "GUIWindowSettingsCategory.h"
#include "util.h"
#include "utils/GUIInfoManager.h"
#include "application.h"
#include "cores/VideoRenderers/RenderManager.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_NONE_AVAILABLE      3
#define CONTROL_AREA                5
#define CONTROL_GAP                 6
#define CONTROL_DEFAULT_BUTTON      7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN        9
#define CONTROL_DEFAULT_SLIDER     10
#define CONTROL_START              50
#define CONTROL_PAGE               60

CGUIDialogVideoSettings::CGUIDialogVideoSettings(void)
    : CGUIDialog(WINDOW_DIALOG_VIDEO_OSD_SETTINGS, "VideoOSDSettings.xml")
{
  m_iLastControl = -1;
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalSettingsButton = NULL;
  m_iCurrentPage = 0;
  m_iNumPages = 0;
  m_iNumPerPage = 0;
//  LoadOnDemand(false);    // we are loaded by the osd window.
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
  if (!m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalSettingsButton || !m_pOriginalSlider)
    return;
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalSettingsButton->SetVisible(false);
  m_pOriginalSlider->SetVisible(false);

  // update our settings label
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, g_localizeStrings.Get(13395));

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
  int numSettings = m_settings.size();
  m_iNumPerPage = (int)pControlArea->GetHeight() / iGapY;
  if (m_iNumPerPage < 1) m_iNumPerPage = 1;
  m_iNumPages = (numSettings + m_iNumPerPage - 1)/ m_iNumPerPage; // round up
  if (m_iCurrentPage >= m_iNumPages - 1)
    m_iCurrentPage = m_iNumPages - 1;
  if (m_iCurrentPage <= 0)
    m_iCurrentPage = 0;
  CGUISpinControl *pPage = (CGUISpinControl *)GetControl(CONTROL_PAGE);
  if (pPage)
  {
    pPage->SetRange(1, m_iNumPages);
    pPage->SetReverse(true);
    pPage->SetShowRange(true);
    pPage->SetValue(m_iCurrentPage + 1);
    pPage->SetVisible(m_iNumPages > 1);
  }

  // run through and create our controls
  int numOnPage = 0;
  for (int i = 0; i < m_iNumPerPage && i + m_iCurrentPage * m_iNumPerPage < numSettings; i++)
  {
    VideoSetting &setting = m_settings.at(i + m_iCurrentPage * m_iNumPerPage);
    AddSetting(setting, iPosX, iPosY, iWidth, CONTROL_START + i);
	  iPosY += iGapY;
    numOnPage++;
  }
  // fix first and last navigation
  CGUIControl *pControl = (CGUIControl *)GetControl(CONTROL_START + numOnPage - 1);
  if (pControl) pControl->SetNavigation(pControl->GetControlIdUp(), CONTROL_PAGE,
                                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(CONTROL_START);
  if (pControl) pControl->SetNavigation(CONTROL_PAGE, pControl->GetControlIdDown(),
                                          pControl->GetControlIdLeft(), pControl->GetControlIdRight());
  pControl = (CGUIControl *)GetControl(CONTROL_PAGE);
  if (pControl) pControl->SetNavigation(CONTROL_START + numOnPage - 1, CONTROL_START,
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
  unsigned int controlID = settingNum + CONTROL_START + m_iCurrentPage * m_iNumPerPage;
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
  unsigned int settingNum = iID - CONTROL_START + m_iCurrentPage * m_iNumPerPage;
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
    ((CGUISpinControlEx *)pControl)->SetLabel(setting.name);
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
    ((CGUISettingsSliderControl *)pControl)->SetLabel(setting.name);
    if (setting.type == VideoSetting::SLIDER)
    {
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

void CGUIDialogVideoSettings::Render()
{
  CGUIDialog::Render();
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

void CGUIDialogVideoSettings::AddSlider(unsigned int id, int label, float *current, float min, float interval, float max)
{
  VideoSetting setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = VideoSetting::SLIDER;
  setting.min = min;
  setting.interval = interval;
  setting.max = max;
  setting.data = current;
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

void CGUIDialogVideoSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  AddBool(1, 306, &g_stSettings.m_currentVideoSettings.m_NonInterleaved);
  AddBool(2, 343, &g_stSettings.m_currentVideoSettings.m_AdjustFrameRate);
  AddBool(3, 431, &g_stSettings.m_currentVideoSettings.m_NoCache);
  AddBool(4, 644, &g_stSettings.m_currentVideoSettings.m_Crop);
  if (g_guiSettings.m_LookAndFeelResolution <= HDTV_480p_16x9)
    AddBool(5, 285, &g_stSettings.m_currentVideoSettings.m_Deinterlace);
  else
  {
    const int entries[] = {351, 16008, 16007}; 
    AddSpin(6, 16006, &g_stSettings.m_currentVideoSettings.m_FieldSync, 3, entries);
  }
  {
    const int entries[] = {630, 631, 632, 633, 634, 635, 636 };
    AddSpin(7, 629, &g_stSettings.m_currentVideoSettings.m_ViewMode, 7, entries);
  }
  AddSlider(8, 216, &g_stSettings.m_currentVideoSettings.m_CustomZoomAmount, 0.5f, 0.01f, 2.0f);
  AddSlider(9, 217, &g_stSettings.m_currentVideoSettings.m_CustomPixelRatio, 0.5f, 0.01f, 2.0f);
  AddSlider(10, 464, &g_stSettings.m_currentVideoSettings.m_Brightness, 0, 100);
  AddSlider(11, 465, &g_stSettings.m_currentVideoSettings.m_Contrast, 0, 100);
  AddSlider(12, 466, &g_stSettings.m_currentVideoSettings.m_Gamma, 0, 100);
  m_flickerFilter = g_guiSettings.GetInt("Filters.Flicker");
  AddSpin(13, 13100, &m_flickerFilter, 0, 5);
  m_soften = g_guiSettings.GetBool("Filters.Soften");
  AddBool(14, 215, &m_soften);
  AddButton(15, 214);
}

void CGUIDialogVideoSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  VideoSetting &setting = m_settings.at(num);
  // check and update anything that needs it
  if ((setting.id >= 1 && setting.id <= 3) || setting.id == 5)
    g_application.Restart(true);
  else if (setting.id == 4)
    g_renderManager.AutoCrop(g_stSettings.m_currentVideoSettings.m_Crop);
  else if (setting.id == 7)
  {
    g_renderManager.SetViewMode(g_stSettings.m_currentVideoSettings.m_ViewMode);
    g_stSettings.m_currentVideoSettings.m_CustomZoomAmount = g_stSettings.m_fZoomAmount;
    g_stSettings.m_currentVideoSettings.m_CustomPixelRatio = g_stSettings.m_fPixelRatio;
    UpdateSetting(8);
    UpdateSetting(9);
  }
  else if (setting.id == 8 || setting.id == 9)
  {
    g_stSettings.m_currentVideoSettings.m_ViewMode = VIEW_MODE_CUSTOM;
    g_renderManager.SetViewMode(VIEW_MODE_CUSTOM);
    UpdateSetting(7);
  }
  else if (setting.id >= 10 && setting.id <= 12)
    CUtil::SetBrightnessContrastGammaPercent(g_stSettings.m_currentVideoSettings.m_Brightness, g_stSettings.m_currentVideoSettings.m_Contrast, g_stSettings.m_currentVideoSettings.m_Gamma, true);
  else if (setting.id >= 13 && setting.id <= 14)
  {
    RESOLUTION res = g_graphicsContext.GetVideoResolution();
    g_guiSettings.SetInt("Filters.Flicker", m_flickerFilter);
    g_guiSettings.SetBool("Filters.Soften", m_soften);
    g_graphicsContext.SetVideoResolution(res);
  }
  else if (setting.id == 15)
  {
    // close the OSD
    CGUIDialog *pDialog = (CGUIDialog *)m_gWindowManager.GetWindow(WINDOW_OSD);
    if (pDialog) pDialog->Close();
    // launch calibration window
    m_gWindowManager.ActivateWindow(WINDOW_MOVIE_CALIBRATION);

  }
}