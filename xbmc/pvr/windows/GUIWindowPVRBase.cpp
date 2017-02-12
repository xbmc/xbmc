/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowPVRBase.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "epg/Epg.h"
#include "GUIUserMessages.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimers.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "utils/Variant.h"

#define MAX_INVALIDATION_FREQUENCY 2000 // limit to one invalidation per X milliseconds

using namespace PVR;
using namespace EPG;
using namespace KODI::MESSAGING;

CCriticalSection CGUIWindowPVRBase::m_selectedItemPathsLock;
std::string CGUIWindowPVRBase::m_selectedItemPaths[2];

CGUIWindowPVRBase::CGUIWindowPVRBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIMediaWindow(id, xmlFile.c_str()),
  m_bRadio(bRadio),
  m_progressHandle(nullptr)
{
  // prevent removable drives to appear in directory listing (base class default behavior).
  m_rootDir.AllowNonLocalSources(false);

  m_selectedItemPaths[false] = "";
  m_selectedItemPaths[true] = "";

  RegisterObservers();
}

CGUIWindowPVRBase::~CGUIWindowPVRBase(void)
{
  UnregisterObservers();
}

void CGUIWindowPVRBase::SetSelectedItemPath(bool bRadio, const std::string &path)
{
  CSingleLock lock(m_selectedItemPathsLock);
  m_selectedItemPaths[bRadio] = path;
}

std::string CGUIWindowPVRBase::GetSelectedItemPath(bool bRadio)
{
  CSingleLock lock(m_selectedItemPathsLock);
  return m_selectedItemPaths[bRadio];
}

void CGUIWindowPVRBase::UpdateSelectedItemPath()
{
  CSingleLock lock(m_selectedItemPathsLock);
  m_selectedItemPaths[m_bRadio] = m_viewControl.GetSelectedItemPath();
}

void CGUIWindowPVRBase::RegisterObservers(void)
{
  g_PVRManager.RegisterObserver(this);

  CSingleLock lock(m_critSection);
  if (m_channelGroup)
    m_channelGroup->RegisterObserver(this);
};

void CGUIWindowPVRBase::UnregisterObservers(void)
{
  {
    CSingleLock lock(m_critSection);
    if (m_channelGroup)
      m_channelGroup->UnregisterObserver(this);
  }
  g_PVRManager.UnregisterObserver(this);
};

void CGUIWindowPVRBase::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageManagerStopped)
    ClearData();

  if (IsActive())
  {
    CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, msg);
    CApplicationMessenger::GetInstance().SendGUIMessage(m);
  }
}

bool CGUIWindowPVRBase::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_PREVIOUS_CHANNELGROUP:
    case ACTION_NEXT_CHANNELGROUP:
      // switch to next or previous group
      SetChannelGroup(action.GetID() == ACTION_NEXT_CHANNELGROUP ? m_channelGroup->GetNextGroup() : m_channelGroup->GetPreviousGroup());
      return true;
  }

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowPVRBase::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK)
  {
    // don't call CGUIMediaWindow as it will attempt to go to the parent folder which we don't want.
    if (GetPreviousWindow() != WINDOW_FULLSCREEN_LIVETV)
    {
      g_windowManager.ActivateWindow(WINDOW_HOME);
      return true;
    }
    else
      return CGUIWindow::OnBack(actionID);
  }
  return CGUIMediaWindow::OnBack(actionID);
}

void CGUIWindowPVRBase::ClearData()
{
  CSingleLock lock(m_critSection);
  m_channelGroup.reset();
}

void CGUIWindowPVRBase::OnInitWindow(void)
{
  SetProperty("IsRadio", m_bRadio ? "true" : "");

  if (InitChannelGroup())
  {
    CGUIMediaWindow::OnInitWindow();

    // mark item as selected by channel path
    m_viewControl.SetSelectedItem(GetSelectedItemPath(m_bRadio));
  }
  else
  {
    CGUIWindow::OnInitWindow(); // do not call CGUIMediaWindow as it will do a Refresh which in no case works in this state (no channelgroup!)
    ShowProgressDialog(g_localizeStrings.Get(19235), 0); // PVR manager is starting up
  }
}

void CGUIWindowPVRBase::OnDeinitWindow(int nextWindowID)
{
  HideProgressDialog();
  UpdateSelectedItemPath();
  CGUIMediaWindow::OnDeinitWindow(nextWindowID);
}

bool CGUIWindowPVRBase::OnMessage(CGUIMessage& message)
{
  bool bReturn = false;
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      switch (message.GetSenderId())
      {
        case CONTROL_BTNCHANNELGROUPS:
          return OpenChannelGroupSelectionDialog();
      }
    }
    break;

    case GUI_MSG_REFRESH_LIST:
    {
      switch (message.GetParam1())
      {
        case ObservableMessageChannelGroupsLoaded:
        {
          // late init
          InitChannelGroup();
          RegisterObservers();
          HideProgressDialog();
          Refresh(true);
          m_viewControl.SetFocused();
          break;
        }
      }
      if (IsActive())
      {
        // Only the active window must set the selected item path which is shared
        // between all PVR windows, not the last notified window (observer).
        UpdateSelectedItemPath();
      }
      bReturn = true;
    }
    break;

    case GUI_MSG_NOTIFY_ALL:
    {
      switch (message.GetParam1())
      {
        case GUI_MSG_UPDATE_SOURCES:
        {
          // removable drive connected/disconnected. base class triggers a window
          // content refresh, which makes no sense for pvr windows.
          bReturn = true;
          break;
        }
      }
    }
    break;
  }

  return bReturn || CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowPVRBase::SetInvalid()
{
  if (m_refreshTimeout.IsTimePast())
  {
    VECFILEITEMS items = m_vecItems->GetList();
    for (VECFILEITEMS::iterator it = items.begin(); it != items.end(); ++it)
      (*it)->SetInvalid();
    CGUIMediaWindow::SetInvalid();
    m_refreshTimeout.Set(MAX_INVALIDATION_FREQUENCY);
  }
}

bool CGUIWindowPVRBase::CanBeActivated() const
{
  // check if there is at least one enabled PVR add-on
  if (!ADDON::CAddonMgr::GetInstance().HasAddons(ADDON::ADDON_PVRDLL))
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{19296}, CVariant{19272}); // No PVR add-on enabled, You need a tuner, backend software...
    return false;
  }

  return true;
}

bool CGUIWindowPVRBase::OpenChannelGroupSelectionDialog(void)
{
  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!dialog)
    return false;

  CFileItemList options;
  g_PVRChannelGroups->Get(m_bRadio)->GetGroupList(&options, true);

  dialog->Reset();
  dialog->SetHeading(CVariant{g_localizeStrings.Get(19146)});
  dialog->SetItems(options);
  dialog->SetMultiSelection(false);
  dialog->SetSelected(m_channelGroup->GroupName());
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  const CFileItemPtr item = dialog->GetSelectedFileItem();
  if (!item)
    return false;

  SetChannelGroup(g_PVRChannelGroups->Get(m_bRadio)->GetByName(item->m_strTitle));

  return true;
}

bool CGUIWindowPVRBase::InitChannelGroup()
{
  const CPVRChannelGroupPtr group(g_PVRManager.GetPlayingGroup(m_bRadio));
  if (group)
  {
    CSingleLock lock(m_critSection);
    if (m_channelGroup != group)
    {
      m_viewControl.SetSelectedItem(0);
      SetChannelGroup(group, false);
    }
    // Path might have changed since last init. Set it always, not just on group change.
    m_vecItems->SetPath(GetDirectoryPath());
    return true;
  }
  return false;
}

CPVRChannelGroupPtr CGUIWindowPVRBase::GetChannelGroup(void)
{
  CSingleLock lock(m_critSection);
  return m_channelGroup;
}

void CGUIWindowPVRBase::SetChannelGroup(const CPVRChannelGroupPtr &group, bool bUpdate /* = true */)
{
  if (!group)
    return;

  CPVRChannelGroupPtr channelGroup;
  {
    CSingleLock lock(m_critSection);
    if (m_channelGroup != group)
    {
      if (m_channelGroup)
        m_channelGroup->UnregisterObserver(this);
      m_channelGroup = group;
      // we need to register the window to receive changes from the new group
      m_channelGroup->RegisterObserver(this);
      channelGroup = m_channelGroup;
    }
  }

  if (bUpdate && channelGroup)
  {
    g_PVRManager.SetPlayingGroup(channelGroup);
    Update(GetDirectoryPath());
  }
}

bool CGUIWindowPVRBase::Update(const std::string &strDirectory, bool updateFilterPath /*= true*/)
{
  if (!GetChannelGroup())
  {
    // no updates before fully initialized
    return false;
  }

  int iOldCount = m_vecItems->Size();
  int iSelectedItem = m_viewControl.GetSelectedItem();
  const std::string oldPath = m_vecItems->GetPath();

  bool bReturn = CGUIMediaWindow::Update(strDirectory, updateFilterPath);

  if (bReturn &&
      iSelectedItem != -1) // something must have been selected
  {
    int iNewCount = m_vecItems->Size();
    if (iOldCount > iNewCount && // at least one item removed by Update()
        oldPath == m_vecItems->GetPath()) // update not due changing into another folder
    {
      // restore selected item if we just deleted one or more items.
      if (iSelectedItem >= iNewCount)
        iSelectedItem = iNewCount - 1;

      m_viewControl.SetSelectedItem(iSelectedItem);
    }
  }

  return bReturn;
}

void CGUIWindowPVRBase::UpdateButtons(void)
{
  CGUIMediaWindow::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_BTNCHANNELGROUPS, g_localizeStrings.Get(19141) + ": " + m_channelGroup->GroupName());
}

void CGUIWindowPVRBase::ShowProgressDialog(const std::string &strText, int iProgress)
{
  if (!m_progressHandle)
  {
    CGUIDialogExtendedProgressBar *loadingProgressDialog = dynamic_cast<CGUIDialogExtendedProgressBar *>(g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS));
    if (!loadingProgressDialog)
    {
      CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - unable to get WINDOW_DIALOG_EXT_PROGRESS!", __FUNCTION__);
      return;
    }
    m_progressHandle = loadingProgressDialog->GetHandle(g_localizeStrings.Get(19235)); // PVR manager is starting up
  }

  m_progressHandle->SetPercentage(static_cast<float>(iProgress));
  m_progressHandle->SetText(strText);
}

void CGUIWindowPVRBase::HideProgressDialog(void)
{
  if (m_progressHandle)
  {
    m_progressHandle->MarkFinished();
    m_progressHandle = nullptr;
  }
}
