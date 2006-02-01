#include "stdafx.h"
#include "GUIDialogSettings.h"

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

CGUIDialogSettings::CGUIDialogSettings(DWORD id, const char *xmlFile)
    : CGUIDialog(id, xmlFile)
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
}

CGUIDialogSettings::~CGUIDialogSettings(void)
{
}

bool CGUIDialogSettings::OnMessage(CGUIMessage &message)
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

void CGUIDialogSettings::SetupPage()
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
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, g_localizeStrings.Get(13395 + GetID() - WINDOW_DIALOG_VIDEO_OSD_SETTINGS));

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
    if (m_settings[i].type != SettingInfo::SEPARATOR)
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
      if (m_settings[m_iPageOffset].type != SettingInfo::SEPARATOR)
        j++;
      m_iPageOffset++;
    }
  }
  // create our controls
  int numSettingsOnPage = 0;
  int numControlsOnPage = 0;
  for (unsigned int i = m_iPageOffset; i < m_settings.size(); i++)
  {
    SettingInfo &setting = m_settings.at(i);
    AddSetting(setting, iPosX, iPosY, iWidth, CONTROL_START + i - m_iPageOffset);
    if (setting.type == SettingInfo::SEPARATOR)
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


void CGUIDialogSettings::UpdateSetting(unsigned int num)
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
  SettingInfo &setting = m_settings.at(settingNum);
  unsigned int controlID = settingNum + CONTROL_START + m_iPageOffset;
  if (setting.type == SettingInfo::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetValue(*(int *)setting.data);
  }
  else if (setting.type == SettingInfo::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetSelected(*(bool *)setting.data);
  }
  else if (setting.type == SettingInfo::SLIDER)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetFloatValue(*(float *)setting.data);
  }
  else if (setting.type == SettingInfo::SLIDER_INT)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetIntValue(*(int *)setting.data);
  }
}

void CGUIDialogSettings::OnClick(int iID)
{
  unsigned int settingNum = iID - CONTROL_START + m_iPageOffset;
  if (settingNum >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(settingNum);
  if (setting.type == SettingInfo::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(iID);
    if (setting.data) *(int *)setting.data = pControl->GetValue();
  }
  else if (setting.type == SettingInfo::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(iID);
    if (setting.data) *(bool *)setting.data = pControl->IsSelected();
  }
  else if (setting.type == SettingInfo::SLIDER)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(iID);
    if (setting.data) *(float *)setting.data = pControl->GetFloatValue();
  }
  else if (setting.type == SettingInfo::SLIDER_INT)
  {
    CGUISettingsSliderControl *pControl = (CGUISettingsSliderControl *)GetControl(iID);
    if (setting.data) *(int *)setting.data = pControl->GetIntValue();
  }
  OnSettingChanged(settingNum);
}

void CGUIDialogSettings::FreeControls()
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

void CGUIDialogSettings::AddSetting(SettingInfo &setting, int iPosX, int iPosY, int iWidth, int iControlID)
{
  CGUIControl *pControl = NULL;
  if (setting.type == SettingInfo::BUTTON)
  {
    pControl = new CGUIButtonControl(*m_pOriginalSettingsButton);
    if (!pControl) return ;
    ((CGUIButtonControl *)pControl)->SetText(setting.name);
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
  }
  else if (setting.type == SettingInfo::SEPARATOR)
  {
    pControl = new CGUIImage(*m_pOriginalSeparator);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
  }
  else if (setting.type == SettingInfo::CHECK)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetText(setting.name);
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    if (setting.data) pControl->SetSelected(*(bool *)setting.data == 1);
  }
  else if (setting.type == SettingInfo::SPIN && setting.entry.size() > 0)
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
  else if (setting.type == SettingInfo::SLIDER || setting.type == SettingInfo::SLIDER_INT)
  {
    pControl = new CGUISettingsSliderControl(*m_pOriginalSlider);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    ((CGUISettingsSliderControl *)pControl)->SetText(setting.name);
    if (setting.type == SettingInfo::SLIDER)
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

void CGUIDialogSettings::AddButton(unsigned int id, int label)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = SettingInfo::BUTTON;
  setting.data = NULL;
  m_settings.push_back(setting);
}

void CGUIDialogSettings::AddBool(unsigned int id, int label, bool *on)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = SettingInfo::CHECK;
  setting.data = on;
  m_settings.push_back(setting);
}

void CGUIDialogSettings::AddSpin(unsigned int id, int label, int *current, unsigned int max, const int *entries)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = SettingInfo::SPIN;
  setting.data = current;
  for (unsigned int i = 0; i < max; i++)
    setting.entry.push_back(g_localizeStrings.Get(entries[i]));
  m_settings.push_back(setting);
}

void CGUIDialogSettings::AddSpin(unsigned int id, int label, int *current, unsigned int min, unsigned int max)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = SettingInfo::SPIN;
  setting.data = current;
  for (unsigned int i = min; i <= max; i++)
  {
    CStdString format;
    format.Format("%i", i);
    setting.entry.push_back(format);
  }
  m_settings.push_back(setting);
}

void CGUIDialogSettings::AddSlider(unsigned int id, int label, float *current, float min, float interval, float max, const char *format /*= NULL*/)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = SettingInfo::SLIDER;
  setting.min = min;
  setting.interval = interval;
  setting.max = max;
  setting.data = current;
  if (format) setting.format = format;
  m_settings.push_back(setting);
}

void CGUIDialogSettings::AddSlider(unsigned int id, int label, int *current, int min, int max)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = g_localizeStrings.Get(label);
  setting.type = SettingInfo::SLIDER_INT;
  setting.min = (float)min;
  setting.max = (float)max;
  setting.data = current;
  m_settings.push_back(setting);
}

void CGUIDialogSettings::AddSeparator(unsigned int id)
{
  SettingInfo setting;
  setting.id = id;
  setting.type = SettingInfo::SEPARATOR;
  setting.data = NULL;
  m_settings.push_back(setting);
}
