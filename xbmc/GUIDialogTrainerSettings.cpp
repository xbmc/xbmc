#include "stdafx.h"
#include "GUIDialogTrainerSettings.h"
#include "GUIWindowSettingsCategory.h"
#include "util.h"
#include "utils/GUIInfoManager.h"
#include "utils/trainer.h"
#include "application.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_NONE_AVAILABLE      3
#define CONTROL_AREA                5
#define CONTROL_GAP                 6
#define CONTROL_DEFAULT_BUTTON      7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN        9
#define CONTROL_DEFAULT_SEPARATOR  11
#define CONTROL_START              50
#define CONTROL_PAGE               60

CGUIDialogTrainerSettings::CGUIDialogTrainerSettings(void)
    : CGUIDialog(WINDOW_DIALOG_TRAINER_SETTINGS, "TrainerSettings.xml")
{
  m_iLastControl = -1;
  m_pOriginalRadioButton = NULL;
  m_pOriginalSettingsButton = NULL;
  m_pOriginalSpinButton = NULL;
  m_pOriginalSeparator = NULL;

  m_iCurrentPage = 0;
  m_iNumPages = 0;
  m_iNumPerPage = 0;
  m_loadOnDemand = false;
  m_iTrainer = 0;
  m_iOldTrainer = 0;
  m_bNeedSave = false;
}

CGUIDialogTrainerSettings::~CGUIDialogTrainerSettings(void)
{
}

bool CGUIDialogTrainerSettings::OnMessage(CGUIMessage &message)
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

      m_database->GetTrainers(m_iTitleId,m_vecOptions);
      m_strActive = m_database->GetActiveTrainer(m_iTitleId);
      m_iTrainer = 0;
      m_iOldTrainer = 0;
      m_bNeedSave = false;
      for (unsigned int i=0;i<m_vecOptions.size();++i)
      {
        CTrainer* trainer = new CTrainer;
        if (trainer->Load(m_vecOptions[i]))
        {
          if (m_vecOptions[i] == m_strActive)
            m_iTrainer = i+1;
          m_vecTrainers.push_back(trainer);
        }
        else
          delete trainer;
      }
      if (m_iTrainer)
        m_database->GetTrainerOptions(m_vecTrainers[m_iTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iTrainer-1]->GetOptions());
     
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
      if (m_iTrainer)
      {
        if (m_bNeedSave)
          m_database->SetTrainerOptions(m_vecTrainers[m_iTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iTrainer-1]->GetOptions());

        if (m_strActive != m_vecTrainers[m_iTrainer-1]->GetPath())
          {
            m_database->SetTrainerActive(m_vecTrainers[m_iTrainer-1]->GetPath(),m_iTitleId,true);
            m_bNeedSave = true;
          }
          else
            m_bNeedSave = false;
      }
      else 
      {
        if (m_strActive == "")
          m_bNeedSave = false;
        else
          m_bNeedSave = true;
      }
      for (unsigned int i=0;i<m_vecTrainers.size();++i)
      {
        if (i != m_iTrainer-1 && m_bNeedSave)
          m_database->SetTrainerActive(m_vecTrainers[i]->GetPath(),m_iTitleId,false);
        delete m_vecTrainers[i];
      }
      m_vecTrainers.clear();
      FreeControls();
      m_settings.clear();
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTrainerSettings::SetupPage()
{
  // cleanup first, if necessary
  FreeControls();
  m_pOriginalSpinButton = (CGUISpinControlEx*)GetControl(CONTROL_DEFAULT_SPIN);
  m_pOriginalRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_DEFAULT_RADIOBUTTON);
  m_pOriginalSeparator = (CGUIImage *)GetControl(CONTROL_DEFAULT_SEPARATOR);
  if (!m_pOriginalRadioButton || !m_pOriginalSpinButton || !m_pOriginalSeparator)
    return;
  
  m_pOriginalSpinButton->SetVisible(false);
  m_pOriginalRadioButton->SetVisible(false);
  m_pOriginalSeparator->SetVisible(false);

  // update our settings label
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, g_localizeStrings.Get(12015));

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
    if (m_settings[i].type != TrainerSetting::SEPARATOR)
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
      if (m_settings[m_iPageOffset].type != TrainerSetting::SEPARATOR)
        j++;
      m_iPageOffset++;
    }
  }
  // create our controls
  int numSettingsOnPage = 0;
  int numControlsOnPage = 0;
  for (unsigned int i = m_iPageOffset; i < m_settings.size(); i++)
  {
    TrainerSetting &setting = m_settings.at(i);
    AddSetting(setting, iPosX, iPosY, iWidth, CONTROL_START + i - m_iPageOffset);
    if (setting.type == TrainerSetting::SEPARATOR)
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


void CGUIDialogTrainerSettings::UpdateSetting(unsigned int num)
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
  TrainerSetting &setting = m_settings.at(settingNum);
  unsigned int controlID = settingNum + CONTROL_START + m_iPageOffset;
  if (setting.type == TrainerSetting::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetValue(*(int *)setting.data);
  }
  else if (setting.type == TrainerSetting::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(controlID);
    if (pControl && setting.data) pControl->SetSelected(*(unsigned char*)setting.data?true:false);
  }
}

void CGUIDialogTrainerSettings::OnClick(int iID)
{
  unsigned int settingNum = iID - CONTROL_START + m_iPageOffset;
  if (settingNum >= m_settings.size()) return;
  TrainerSetting &setting = m_settings.at(settingNum);
  if (setting.type == TrainerSetting::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(iID);
    if (setting.data) 
    {
      m_iOldTrainer = m_iTrainer;
      *(int *)setting.data = pControl->GetValue();
    }
  }
  else if (setting.type == TrainerSetting::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(iID);
    if (setting.data) 
    {
      *(unsigned char*)setting.data = pControl->IsSelected()?1:0;
    }
  }
  OnSettingChanged(settingNum);
}

void CGUIDialogTrainerSettings::FreeControls()
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

void CGUIDialogTrainerSettings::AddSetting(TrainerSetting &setting, int iPosX, int iPosY, int iWidth, int iControlID)
{
  CGUIControl *pControl = NULL;
  if (setting.type == TrainerSetting::SEPARATOR)
  {
    pControl = new CGUIImage(*m_pOriginalSeparator);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
  }
  else if (setting.type == TrainerSetting::CHECK)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetText(setting.name);
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    if (setting.data) 
    {
      pControl->SetSelected(*(unsigned char*)setting.data==1);
    }
  }
  else if (setting.type == TrainerSetting::SPIN && setting.entry.size() > 0)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpinButton);
    if (!pControl) return ;
    pControl->SetPosition(iPosX, iPosY);
    pControl->SetWidth(iWidth);
    ((CGUISpinControlEx *)pControl)->SetText(setting.name);
    pControl->SetWidth(iWidth);
    for (unsigned int i = 0; i < setting.entry.size(); i++)
      ((CGUISpinControlEx *)pControl)->AddLabel(setting.entry[i], i);
    if (setting.data) ((CGUISpinControlEx *)pControl)->SetValue(*(int *)setting.data);
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

void CGUIDialogTrainerSettings::AddSpin(unsigned int id, int label, int *current)
{
  TrainerSetting setting;
  setting.id = id;
  setting.name = "";
  setting.type = TrainerSetting::SPIN;
  setting.data = current;
  setting.entry.push_back("None");
  for (unsigned int i = 0; i < m_vecTrainers.size(); i++)
    setting.entry.push_back(m_vecTrainers[i]->GetName());
  
  m_settings.push_back(setting);
}

void CGUIDialogTrainerSettings::AddBool(unsigned int id, const CStdString& strLabel, unsigned char* on)
{
  TrainerSetting setting;
  setting.id = id;
  setting.name = strLabel;
  setting.type = TrainerSetting::CHECK;
  setting.data = on;
  m_settings.push_back(setting);
}

void CGUIDialogTrainerSettings::AddSeparator(unsigned int id)
{
  TrainerSetting setting;
  setting.id = id;
  setting.type = TrainerSetting::SEPARATOR;
  setting.data = NULL;
  m_settings.push_back(setting);
}

void CGUIDialogTrainerSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  if (m_vecTrainers.size())
  {
    AddSpin(1,12013,&m_iTrainer);
    AddSeparator(2);
    if (m_iTrainer && m_iTrainer < (int)m_vecTrainers.size()+1)
    {
      m_vecTrainers[m_iTrainer-1]->GetOptionLabels(m_vecOptions);
      for (unsigned int i=0;i<m_vecOptions.size();++i)
        AddBool(i+2,m_vecOptions[i],m_vecTrainers[m_iTrainer-1]->GetOptions()+i);
    }
  }    
}

void CGUIDialogTrainerSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  TrainerSetting &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == 1)
  {
    if (m_iOldTrainer && m_bNeedSave)
      m_database->SetTrainerOptions(m_vecTrainers[m_iOldTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iOldTrainer-1]->GetOptions());

    if (m_iTrainer)
      m_database->GetTrainerOptions(m_vecTrainers[m_iTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iTrainer-1]->GetOptions());
  
    CreateSettings();
    SetupPage();
    SET_CONTROL_FOCUS(CONTROL_START, 0);
    m_bNeedSave = false;
  }
  else
    m_bNeedSave = true;
}

bool CGUIDialogTrainerSettings::HasID(DWORD dwID)
{
  return (dwID == WINDOW_DIALOG_TRAINER_SETTINGS);
}

void CGUIDialogTrainerSettings::Render()
{
  CGUIDialog::Render();
}

void CGUIDialogTrainerSettings::ShowForTitle(unsigned int iTitleId, CProgramDatabase* database)
{
  CGUIDialogTrainerSettings *dialog = (CGUIDialogTrainerSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_TRAINER_SETTINGS);
  if (!dialog) return;
  dialog->m_iTitleId = iTitleId;
  dialog->m_database = database;
  dialog->DoModal(m_gWindowManager.GetActiveWindow());
  return ;
}