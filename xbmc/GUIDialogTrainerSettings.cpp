#include "stdafx.h"
#include "GUIDialogTrainerSettings.h"
#include "utils/trainer.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_START              50

CGUIDialogTrainerSettings::CGUIDialogTrainerSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_TRAINER_SETTINGS, "TrainerSettings.xml")
{
  m_iTrainer = 0;
  m_iOldTrainer = 0;
  m_bNeedSave = false;
  m_loadOnDemand = true;
}

CGUIDialogTrainerSettings::~CGUIDialogTrainerSettings(void)
{
}

bool CGUIDialogTrainerSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogSettings::OnMessage(message);
      if (m_iTrainer)
      {
        if (m_bNeedSave)
          m_database->SetTrainerOptions(m_vecTrainers[m_iTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iTrainer-1]->GetOptions(),m_vecTrainers[m_iTrainer-1]->GetNumberOfOptions());

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
    }
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogTrainerSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  // update our settings label
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, g_localizeStrings.Get(12015));
}

void CGUIDialogTrainerSettings::AddBool(unsigned int id, const CStdString& strLabel, unsigned char* on)
{
  SettingInfo setting;
  setting.id = id;
  setting.name = strLabel;
  setting.type = SettingInfo::CHECK_UCHAR;
  setting.data = on;
  m_settings.push_back(setting);
}

#define TRAINER_NAME     1

void CGUIDialogTrainerSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();
  // create our settings
  if (m_vecTrainers.size())
  {
    // trainers spin control
    SettingInfo setting;
    setting.id = TRAINER_NAME;
    setting.name = "";
    setting.type = SettingInfo::SPIN;
    setting.data = &m_iTrainer;
    setting.entry.push_back("None");
    for (unsigned int i = 0; i < m_vecTrainers.size(); i++)
      setting.entry.push_back(m_vecTrainers[i]->GetName());
    m_settings.push_back(setting);

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
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == 1)
  {
    if (m_iOldTrainer && m_bNeedSave)
      m_database->SetTrainerOptions(m_vecTrainers[m_iOldTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iOldTrainer-1]->GetOptions(), m_vecTrainers[m_iOldTrainer-1]->GetNumberOfOptions());

    if (m_iTrainer)
      m_database->GetTrainerOptions(m_vecTrainers[m_iTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iTrainer-1]->GetOptions(),m_vecTrainers[m_iTrainer-1]->GetNumberOfOptions());
  
    CreateSettings();
    SetupPage();
    SET_CONTROL_FOCUS(CONTROL_START, 0);
    m_bNeedSave = false;
  }
  else
    m_bNeedSave = true;
}

bool CGUIDialogTrainerSettings::ShowForTitle(unsigned int iTitleId, CProgramDatabase* database)
{
  CStdString strTrainer = database->GetActiveTrainer(iTitleId);
  CGUIDialogTrainerSettings *dialog = (CGUIDialogTrainerSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_TRAINER_SETTINGS);
  if (!dialog) return false;
  dialog->m_iTitleId = iTitleId;
  dialog->m_database = database;
  dialog->DoModal(m_gWindowManager.GetActiveWindow());
  if (database->GetActiveTrainer(iTitleId) != strTrainer)
    return true;

  return false;
}

void CGUIDialogTrainerSettings::OnInitWindow()
{
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
    m_database->GetTrainerOptions(m_vecTrainers[m_iTrainer-1]->GetPath(),m_iTitleId,m_vecTrainers[m_iTrainer-1]->GetOptions(), m_vecTrainers[m_iTrainer-1]->GetNumberOfOptions());

  CGUIDialogSettings::OnInitWindow();
}