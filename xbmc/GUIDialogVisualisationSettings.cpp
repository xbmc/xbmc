#include "stdafx.h"
#include "GUIDialogVisualisationSettings.h"
#include "GUIWindowSettingsCategory.h"
#include "util.h"
#include "utils/GUIInfoManager.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_NONE_AVAILABLE      3
#define CONTROL_AREA                5
#define CONTROL_GAP                 6
#define CONTROL_DEFAULT_BUTTON      7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN        9
#define CONTROL_START              50
#define CONTROL_PAGE               60

CGUIDialogVisualisationSettings::CGUIDialogVisualisationSettings(void)
    : CGUIDialog(0)
{
  m_iLastControl = -1;
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalSettingsButton = NULL;
  m_pVisualisation = NULL;
  m_pSettings = NULL;
  m_iCurrentPage = 0;
  m_iNumPages = 0;
  m_iNumPerPage = 0;
}

CGUIDialogVisualisationSettings::~CGUIDialogVisualisationSettings(void)
{
}

bool CGUIDialogVisualisationSettings::OnMessage(CGUIMessage &message)
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

      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_graphicsContext.SendMessage(msg);
      SetVisualisation((CVisualisation *)msg.GetLPVOID());

      m_iCurrentPage = 0;
      m_iNumPages = 0;

      SetupPage();

      SET_CONTROL_FOCUS(CONTROL_START, 0);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      FreeControls();
      m_pVisualisation = NULL;
      m_pSettings = NULL;
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
      SetVisualisation((CVisualisation *)message.GetLPVOID());

      m_iCurrentPage = 0;
      m_iNumPages = 0;
      SetupPage();
      SET_CONTROL_FOCUS(CONTROL_START, 0);
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVisualisationSettings::SetupPage()
{
  // cleanup first, if necessary
  FreeControls();
  m_pOriginalSpin = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalSettingsButton = (CGUIButtonControl *)GetControl(CONTROL_DEFAULT_BUTTON);
  if (!m_pOriginalSpin || !m_pOriginalRadioButton || !m_pOriginalSettingsButton)
    return;
  m_pOriginalSpin->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalSettingsButton->SetVisible(false);

  // update our settings label
  CStdStringW strSettings;
  strSettings.Format(L"%s %s", g_infoManager.GetLabel(402).c_str(), g_localizeStrings.Get(5));
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, strSettings);

  // our controls for layout...
  const CGUIControl *pControlArea = GetControl(CONTROL_AREA);
  const CGUIControl *pControlGap = GetControl(CONTROL_GAP);
  if (!pControlArea || !pControlGap)
    return;

  if (!m_pSettings || !m_pSettings->size())
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
  int numSettings = m_pSettings->size();
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
    VisSetting &setting = m_pSettings->at(i + m_iCurrentPage * m_iNumPerPage);
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
  UpdateSettings();
}


void CGUIDialogVisualisationSettings::UpdateSettings()
{
}

void CGUIDialogVisualisationSettings::OnClick(int iID)
{
  if (!m_pSettings || !m_pVisualisation) return;
  unsigned int settingNum = iID - CONTROL_START + m_iCurrentPage * m_iNumPerPage;
  if (settingNum >= m_pSettings->size()) return;
  VisSetting &setting = m_pSettings->at(settingNum);
  if (setting.type == VisSetting::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(iID);
    setting.current = pControl->GetValue();
  }
  else if (setting.type == VisSetting::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(iID);
    setting.current = pControl->IsSelected() ? 1 : 0;
  }
  m_pVisualisation->UpdateSetting(settingNum);
  UpdateSettings();
}

void CGUIDialogVisualisationSettings::FreeControls()
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

void CGUIDialogVisualisationSettings::AddSetting(VisSetting &setting, int iPosX, int iPosY, int iWidth, int iControlID)
{
  CGUIControl *pControl = NULL;
  if (setting.type == VisSetting::CHECK)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetText(setting.name);
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    pControl->SetSelected(setting.current == 1);
  }
  else if (setting.type == VisSetting::SPIN && setting.entry.size() > 0)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    ((CGUISpinControlEx *)pControl)->SetLabel(setting.name);
    pControl->SetWidth(iWidth);
    for (unsigned int i = 0; i < setting.entry.size(); i++)
      ((CGUISpinControlEx *)pControl)->AddLabel(setting.entry[i], i);
    ((CGUISpinControlEx *)pControl)->SetValue(setting.current);
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

void CGUIDialogVisualisationSettings::Render()
{
  CGUIDialog::Render();
}

void CGUIDialogVisualisationSettings::SetVisualisation(CVisualisation *pVisualisation)
{
  m_pVisualisation = pVisualisation;
  if (m_pVisualisation)
  {
    m_pVisualisation->GetSettings(&m_pSettings);
  }
}

