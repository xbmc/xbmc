/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRTimersBase.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIComponent.h"
#include "input/Key.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimersPath.h"

using namespace PVR;
using namespace KODI::MESSAGING;

CGUIWindowPVRTimersBase::CGUIWindowPVRTimersBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIWindowPVRBase(bRadio, id, xmlFile)
{
  CServiceBroker::GetGUI()->GetInfoManager().RegisterObserver(this);
}

CGUIWindowPVRTimersBase::~CGUIWindowPVRTimersBase()
{
  CServiceBroker::GetGUI()->GetInfoManager().UnregisterObserver(this);
}

bool CGUIWindowPVRTimersBase::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR ||
      action.GetID() == ACTION_NAV_BACK)
  {
    CPVRTimersPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsTimerRule())
    {
      m_currentFileItem.reset();
      GoParentFolder();
      return true;
    }
  }
  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRTimersBase::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  int iOldCount = m_vecItems->GetObjectCount();
  const std::string oldPath = m_vecItems->GetPath();

  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  if (bReturn && iOldCount > 0 && m_vecItems->GetObjectCount() == 0 && oldPath == m_vecItems->GetPath())
  {
    /* go to the parent folder if we're in a subdirectory and for instance just deleted the last item */
    const CPVRTimersPath path(m_vecItems->GetPath());
    if (path.IsValid() && path.IsTimerRule())
    {
      m_currentFileItem.reset();
      GoParentFolder();
    }
  }

  return bReturn;
}

void CGUIWindowPVRTimersBase::UpdateButtons(void)
{
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTNHIDEDISABLEDTIMERS, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS));

  CGUIWindowPVRBase::UpdateButtons();

  std::string strHeaderTitle;
  if (m_currentFileItem && m_currentFileItem->HasPVRTimerInfoTag())
  {
    CPVRTimerInfoTagPtr timer = m_currentFileItem->GetPVRTimerInfoTag();
    strHeaderTitle = timer->Title();
  }

  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, strHeaderTitle);
}

bool CGUIWindowPVRTimersBase::OnMessage(CGUIMessage &message)
{
  bool bReturn = false;
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
      if (message.GetSenderId() == m_viewControl.GetCurrentControl())
      {
        int iItem = m_viewControl.GetSelectedItem();
        if (iItem >= 0 && iItem < m_vecItems->Size())
        {
          bReturn = true;
          switch (message.GetParam1())
          {
            case ACTION_SHOW_INFO:
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            {
              CFileItemPtr item(m_vecItems->Get(iItem));
              if (item->m_bIsFolder && (message.GetParam1() != ACTION_SHOW_INFO))
              {
                m_currentFileItem = item;
                bReturn = false; // folders are handled by base class
              }
              else
              {
                m_currentFileItem.reset();
                ActionShowTimer(item);
              }
              break;
            }
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              break;
            case ACTION_DELETE_ITEM:
              CServiceBroker::GetPVRManager().GUIActions()->DeleteTimer(m_vecItems->Get(iItem));
              break;
            default:
              bReturn = false;
              break;
          }
        }
      }
      else if (message.GetSenderId() == CONTROL_BTNHIDEDISABLEDTIMERS)
      {
        const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
        settings->ToggleBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);
        settings->Save();
        Update(GetDirectoryPath());
        bReturn = true;
      }
      break;
    case GUI_MSG_REFRESH_LIST:
      switch(message.GetParam1())
      {
        case ObservableMessageTimers:
        case ObservableMessageEpg:
        case ObservableMessageEpgContainer:
        case ObservableMessageEpgActiveItem:
        case ObservableMessageCurrentItem:
        {
          SetInvalid();
          break;
        }
        case ObservableMessageTimersReset:
        {
          Refresh(true);
          break;
        }
      }
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRTimersBase::ActionShowTimer(const CFileItemPtr &item)
{
  bool bReturn = false;

  /* Check if "Add timer..." entry is selected, if yes
     create a new timer and open settings dialog, otherwise
     open settings for selected timer entry */
  if (URIUtils::PathEquals(item->GetPath(), CPVRTimersPath::PATH_ADDTIMER))
    bReturn = CServiceBroker::GetPVRManager().GUIActions()->AddTimer(m_bRadio);
  else
    bReturn = CServiceBroker::GetPVRManager().GUIActions()->EditTimer(item);

  return bReturn;
}
