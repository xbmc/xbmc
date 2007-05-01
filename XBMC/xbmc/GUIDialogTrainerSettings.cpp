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
#include "GUIDialogTrainerSettings.h"
#include "utils/Trainer.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_START              30

CGUIDialogTrainerSettings::CGUIDialogTrainerSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_TRAINER_SETTINGS, "TrainerSettings.xml")
{
  m_database = NULL;
  m_iTrainer = 0;
  m_iOldTrainer = 0;
  m_bNeedSave = false;
  m_bCanceled = false;
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
      if (m_database)
      {
        m_database->BeginTransaction();
        for (unsigned int i=0;i<m_vecTrainers.size();++i)
        {
          if (m_bNeedSave && !m_bCanceled)
          {
            m_database->SetTrainerActive(m_vecTrainers[i]->GetPath(),m_iTitleId,m_iTrainer-1 == i);
            m_database->SetTrainerOptions(m_vecTrainers[i]->GetPath(),m_iTitleId,m_vecTrainers[i]->GetOptions(),m_vecTrainers[i]->GetNumberOfOptions());
          }
          delete m_vecTrainers[i];
        }
        m_vecTrainers.clear();
        m_database->CommitTransaction();
      }
      if (m_bCanceled)
        m_bNeedSave = false;

      break;
    }
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogTrainerSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  // update our settings label
  SET_CONTROL_LABEL(CONTROL_SETTINGS_LABEL, g_localizeStrings.Get(12015));
}

void CGUIDialogTrainerSettings::OnCancel()
{
  m_bNeedSave = false;
  m_bCanceled = true;
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
    setting.entry.push_back(g_localizeStrings.Get(231));
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
    CreateSettings();
    SetupPage();
    SET_CONTROL_FOCUS(CONTROL_START, 0);
  }

  m_bNeedSave = true;
}

bool CGUIDialogTrainerSettings::ShowForTitle(unsigned int iTitleId, CProgramDatabase* database)
{
  CGUIDialogTrainerSettings *dialog = (CGUIDialogTrainerSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_TRAINER_SETTINGS);
  if (!dialog) return false;
  dialog->m_iTitleId = iTitleId;
  dialog->m_database = database;
  dialog->DoModal();
  if (dialog->m_bNeedSave)
    return true;

  return false;
}

void CGUIDialogTrainerSettings::OnInitWindow()
{
  if (m_database)
  {
    m_database->GetTrainers(m_iTitleId,m_vecOptions);
    m_strActive = m_database->GetActiveTrainer(m_iTitleId);
  }
  m_iTrainer = 0;
  m_iOldTrainer = 0;
  m_bNeedSave = false;
  m_bCanceled = false;
  for (unsigned int i=0;i<m_vecOptions.size();++i)
  {
    CTrainer* trainer = new CTrainer;
    if (trainer->Load(m_vecOptions[i]))
    {
      if (m_vecOptions[i] == m_strActive)
        m_iTrainer = i+1;
      if (m_database)
        m_database->GetTrainerOptions(trainer->GetPath(),m_iTitleId,trainer->GetOptions(), trainer->GetNumberOfOptions());
      m_vecTrainers.push_back(trainer);
    }
    else
      delete trainer;
  }

  CGUIDialogSettings::OnInitWindow();
}

