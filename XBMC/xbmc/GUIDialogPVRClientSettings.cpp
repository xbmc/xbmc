/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "stdafx.h"
#include "GUIDialogPVRClientSettings.h"
#include "GUIWindowSettingsCategory.h"
#include "GUIControlGroupList.h"
#include "utils/GUIInfoManager.h"

#define CONTROL_SETTINGS_LABEL      2
#define CONTROL_NONE_AVAILABLE      3
#define CONTROL_GROUP_LIST          5
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#define CONTROL_AREA                5
#define CONTROL_GAP                 6
#endif
#define CONTROL_DEFAULT_BUTTON      7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN        9
#define CONTROL_START              50
#define CONTROL_PAGE               60

CGUIDialogPVRClientSettings::CGUIDialogPVRClientSettings(void)
: CGUIDialog(WINDOW_DIALOG_PVRCLIENT_SETTINGS, "PVRClientSettings.xml")
{
  m_pOriginalSpin = NULL;
  m_pOriginalRadioButton = NULL;
  m_pOriginalSettingsButton = NULL;
  m_pClient = NULL;
  m_pSettings = NULL;
  LoadOnDemand(true);    // we must query which client we are configuring
}

CGUIDialogPVRClientSettings::~CGUIDialogPVRClientSettings(void)
{
}

bool CGUIDialogPVRClientSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl >= CONTROL_START && iControl < CONTROL_PAGE)
        OnClick(iControl);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      FreeControls();
      m_pClient = NULL;
      m_pSettings = NULL;
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
      SetClient((CPVRClient *)message.GetLPVOID());
      SetupPage();
      SET_CONTROL_FOCUS(CONTROL_START, 0);
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRClientSettings::SetupPage()
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

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_GROUP_LIST);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (!group || group->GetControlType() != CGUIControl::GUICONTROL_GROUPLIST)
  {
    // our controls for layout...
    CGUIControl *pArea = (CGUIControl *)GetControl(CONTROL_AREA);
    const CGUIControl *pGap = GetControl(CONTROL_GAP);
    if (!pArea || !pGap)
      return;
    Remove(pArea);
    group = new CGUIControlGroupList(GetID(), CONTROL_GROUP_LIST, pArea->GetXPosition(), pArea->GetYPosition(),
      pArea->GetWidth(), pArea->GetHeight(), pGap->GetHeight() - m_pOriginalSettingsButton->GetHeight(),
      0, VERTICAL, false);
    group->SetNavigation(CONTROL_GROUP_LIST, CONTROL_GROUP_LIST, CONTROL_GROUP_LIST, CONTROL_GROUP_LIST);
    Insert(group, pGap);
    pArea->FreeResources();
    delete pArea;
    SET_CONTROL_HIDDEN(CONTROL_PAGE);
  }
#endif
  if (!group)
    return;

  if (!m_pSettings || !m_pSettings->size())
  { // no settings available
    SET_CONTROL_VISIBLE(CONTROL_NONE_AVAILABLE);
    return;
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_NONE_AVAILABLE);
  }


  // run through and create our controls
  for (unsigned int i = 0; i < m_pSettings->size(); i++)
  {
    PVRSetting &setting = m_pSettings->at(i);
    AddSetting(setting, group->GetWidth(), CONTROL_START + i);
  }
  UpdateSettings();
}


void CGUIDialogPVRClientSettings::UpdateSettings()
{
}

void CGUIDialogPVRClientSettings::OnClick(int iID)
{
  if (!m_pSettings || !m_pClient) return;
  unsigned int settingNum = iID - CONTROL_START;
  if (settingNum >= m_pSettings->size()) return;
  PVRSetting &setting = m_pSettings->at(settingNum);
  if (setting.type == PVRSetting::SPIN)
  {
    CGUISpinControlEx *pControl = (CGUISpinControlEx *)GetControl(iID);
    setting.current = pControl->GetValue();
  }
  else if (setting.type == PVRSetting::CHECK)
  {
    CGUIRadioButtonControl *pControl = (CGUIRadioButtonControl *)GetControl(iID);
    setting.current = pControl->IsSelected() ? 1 : 0;
  }
  m_pClient->UpdateSetting(settingNum);
  UpdateSettings();
}

void CGUIDialogPVRClientSettings::FreeControls()
{
  // just clear our group list
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_GROUP_LIST);
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  if (group && group->GetControlType() == CGUIControl::GUICONTROL_GROUPLIST)
#else
  if (group)
#endif
  {
    group->FreeResources();
    group->ClearAll();
  }
}

void CGUIDialogPVRClientSettings::AddSetting(PVRSetting &setting, float width, int iControlID)
{
  CGUIControl *pControl = NULL;
  if (setting.type == PVRSetting::CHECK)
  {
    pControl = new CGUIRadioButtonControl(*m_pOriginalRadioButton);
    if (!pControl) return ;
    ((CGUIRadioButtonControl *)pControl)->SetLabel(setting.name);
    pControl->SetWidth(width);
    ((CGUIRadioButtonControl *)pControl)->SetSelected(setting.current == 1);
  }
  else if (setting.type == PVRSetting::SPIN && setting.entry.size() > 0)
  {
    pControl = new CGUISpinControlEx(*m_pOriginalSpin);
    if (!pControl) return ;
    pControl->SetWidth(width);
    ((CGUISpinControlEx *)pControl)->SetText(setting.name);
    pControl->SetWidth(width);
    for (unsigned int i = 0; i < setting.entry.size(); i++)
      ((CGUISpinControlEx *)pControl)->AddLabel(setting.entry[i], i);
    ((CGUISpinControlEx *)pControl)->SetValue(setting.current);
  }
  if (!pControl) return;
  pControl->SetID(iControlID);
  pControl->SetVisible(true);
  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(CONTROL_GROUP_LIST);
  if (group)
  {
    pControl->AllocResources();
    group->AddControl(pControl);
  }
  else
    delete pControl;
}

void CGUIDialogPVRClientSettings::Render()
{
  CGUIDialog::Render();
}

void CGUIDialogPVRClientSettings::SetClient(CPVRClient *pClient)
{
  m_pClient = pClient;
  if (m_pClient)
  {
    m_pClient->GetSettings(&m_pSettings);
  }
}

void CGUIDialogPVRClientSettings::OnInitWindow()
{
 /* CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
  g_graphicsContext.SendMessage(msg);
  SetClient((CPVRClient *)msg.GetLPVOID());*/

  SetupPage();
  // reset the default control
  m_lastControlID = CONTROL_START;
  CGUIDialog::OnInitWindow();
}

