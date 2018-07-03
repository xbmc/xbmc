/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/timers/PVRTimers.h"

#define MAX_INVALIDATION_FREQUENCY 2000 // limit to one invalidation per X milliseconds

using namespace PVR;
using namespace KODI::MESSAGING;

namespace PVR
{

class CGUIPVRChannelGroupsSelector
{
public:
  virtual ~CGUIPVRChannelGroupsSelector() = default;

  bool Initialize(CGUIWindow* parent, bool bRadio);

  bool HasFocus() const;
  CPVRChannelGroupPtr GetSelectedChannelGroup() const;
  bool SelectChannelGroup(const CPVRChannelGroupPtr &newGroup);

private:
  CGUIControl *m_control = nullptr;
  std::vector<CPVRChannelGroupPtr> m_channelGroups;
};

} // namespace PVR

bool CGUIPVRChannelGroupsSelector::Initialize(CGUIWindow* parent, bool bRadio)
{
  CGUIControl* control = parent->GetControl(CONTROL_LSTCHANNELGROUPS);
  if (control && control->IsContainer())
  {
    m_control = control;
    m_channelGroups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio)->GetMembers(true);
    CFileItemList channelGroupItems;
    for (const auto& group : m_channelGroups)
    {
      CFileItemPtr item(new CFileItem(group->GetPath(), true));
      item->m_strTitle = group->GroupName();
      item->SetLabel(group->GroupName());
      channelGroupItems.Add(item);
    }

    CGUIMessage msg(GUI_MSG_LABEL_BIND, m_control->GetID(), CONTROL_LSTCHANNELGROUPS, 0, 0, &channelGroupItems);
    m_control->OnMessage(msg);
    return true;
  }
  return false;
}

bool CGUIPVRChannelGroupsSelector::HasFocus() const
{
  return m_control && m_control->HasFocus();
}

CPVRChannelGroupPtr CGUIPVRChannelGroupsSelector::GetSelectedChannelGroup() const
{
  if (m_control)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, m_control->GetID(), CONTROL_LSTCHANNELGROUPS);
    m_control->OnMessage(msg);

    const auto it = std::next(m_channelGroups.begin(), msg.GetParam1());
    if (it != m_channelGroups.end())
    {
      return *it;
    }
  }
  return CPVRChannelGroupPtr();
}

bool CGUIPVRChannelGroupsSelector::SelectChannelGroup(const CPVRChannelGroupPtr &newGroup)
{
  if (m_control && newGroup)
  {
    int iIndex = 0;
    for (const auto& group : m_channelGroups)
    {
      if (*newGroup == *group)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECT, m_control->GetID(), CONTROL_LSTCHANNELGROUPS, iIndex);
        m_control->OnMessage(msg);
        return true;
      }
      ++iIndex;
    }
  }
  return false;
}

CGUIWindowPVRBase::CGUIWindowPVRBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIMediaWindow(id, xmlFile.c_str()),
  m_bRadio(bRadio),
  m_channelGroupsSelector(new CGUIPVRChannelGroupsSelector),
  m_progressHandle(nullptr)
{
  // prevent removable drives to appear in directory listing (base class default behavior).
  m_rootDir.AllowNonLocalSources(false);

  RegisterObservers();
}

CGUIWindowPVRBase::~CGUIWindowPVRBase(void)
{
  UnregisterObservers();
}

void CGUIWindowPVRBase::UpdateSelectedItemPath()
{
  CServiceBroker::GetPVRManager().GUIActions()->SetSelectedItemPath(m_bRadio, m_viewControl.GetSelectedItemPath());
}

void CGUIWindowPVRBase::RegisterObservers(void)
{
  CServiceBroker::GetPVRManager().RegisterObserver(this);

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
  CServiceBroker::GetPVRManager().UnregisterObserver(this);
};

void CGUIWindowPVRBase::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageManagerStopped)
    ClearData();

  if (m_active)
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
    {
      // switch to next or previous group
      if (const CPVRChannelGroupPtr channelGroup = GetChannelGroup())
      {
        SetChannelGroup(action.GetID() == ACTION_NEXT_CHANNELGROUP ? channelGroup->GetNextGroup() : channelGroup->GetPreviousGroup());
      }
      return true;
    }
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_LEFT:
    {
      if (m_channelGroupsSelector->HasFocus() && CGUIMediaWindow::OnAction(action))
      {
        SetChannelGroup(m_channelGroupsSelector->GetSelectedChannelGroup());
        return true;
      }
    }
  }

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowPVRBase::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK)
  {
    // don't call CGUIMediaWindow as it will attempt to go to the parent folder which we don't want.
    if (GetPreviousWindow() != WINDOW_FULLSCREEN_VIDEO)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
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
  m_channelGroupsSelector.reset(new CGUIPVRChannelGroupsSelector);
}

void CGUIWindowPVRBase::OnInitWindow(void)
{
  SetProperty("IsRadio", m_bRadio ? "true" : "");

  if (InitChannelGroup())
  {
    m_channelGroupsSelector->Initialize(this, m_bRadio);

    CGUIMediaWindow::OnInitWindow();

    // mark item as selected by channel path
    m_viewControl.SetSelectedItem(CServiceBroker::GetPVRManager().GUIActions()->GetSelectedItemPath(m_bRadio));

    // This has to be done after base class OnInitWindow to restore correct selection
    m_channelGroupsSelector->SelectChannelGroup(GetChannelGroup());
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

        case CONTROL_LSTCHANNELGROUPS:
        {
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            {
              SetChannelGroup(m_channelGroupsSelector->GetSelectedChannelGroup());
              bReturn = true;
              break;
            }
          }
        }
      }
      break;
    }

    case GUI_MSG_REFRESH_LIST:
    {
      switch (message.GetParam1())
      {
        case ObservableMessageChannelGroupsLoaded:
        {
          // late init
          InitChannelGroup();
          m_channelGroupsSelector->Initialize(this, m_bRadio);
          m_channelGroupsSelector->SelectChannelGroup(GetChannelGroup());
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
      break;
    }

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
      break;
    }
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
  if (!CServiceBroker::GetAddonMgr().HasAddons(ADDON::ADDON_PVRDLL))
  {
    HELPERS::ShowOKDialogText(CVariant{19296}, CVariant{19272}); // No PVR add-on enabled, You need a tuner, backend software...
    return false;
  }

  return true;
}

bool CGUIWindowPVRBase::OpenChannelGroupSelectionDialog(void)
{
  CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (!dialog)
    return false;

  CFileItemList options;
  CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bRadio)->GetGroupList(&options, true);

  dialog->Reset();
  dialog->SetHeading(CVariant{g_localizeStrings.Get(19146)});
  dialog->SetItems(options);
  dialog->SetMultiSelection(false);
  if (const CPVRChannelGroupPtr channelGroup = GetChannelGroup())
  {
    dialog->SetSelected(channelGroup->GroupName());
  }
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  const CFileItemPtr item = dialog->GetSelectedFileItem();
  if (!item)
    return false;

  SetChannelGroup(CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bRadio)->GetByName(item->m_strTitle));

  return true;
}

bool CGUIWindowPVRBase::InitChannelGroup()
{
  CPVRChannelGroupPtr group(CServiceBroker::GetPVRManager().GetPlayingGroup(m_bRadio));
  if (group)
  {
    CSingleLock lock(m_critSection);
    if (m_channelGroup != group)
    {
      m_viewControl.SetSelectedItem(0);
      SetChannelGroup(std::move(group), false);
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

void CGUIWindowPVRBase::SetChannelGroup(CPVRChannelGroupPtr &&group, bool bUpdate /* = true */)
{
  if (!group)
    return;

  CPVRChannelGroupPtr updateChannelGroup;
  {
    CSingleLock lock(m_critSection);
    if (m_channelGroup != group)
    {
      if (m_channelGroup)
        m_channelGroup->UnregisterObserver(this);
      m_channelGroup = std::move(group);
      // we need to register the window to receive changes from the new group
      m_channelGroup->RegisterObserver(this);
      if (bUpdate)
        updateChannelGroup = m_channelGroup;
    }
  }

  if (updateChannelGroup)
  {
    CServiceBroker::GetPVRManager().SetPlayingGroup(updateChannelGroup);
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

  const CPVRChannelGroupPtr channelGroup = GetChannelGroup();
  if (channelGroup)
  {
    SET_CONTROL_LABEL(CONTROL_BTNCHANNELGROUPS, g_localizeStrings.Get(19141) + ": " + channelGroup->GroupName());
  }

  m_channelGroupsSelector->SelectChannelGroup(channelGroup);
}

void CGUIWindowPVRBase::ShowProgressDialog(const std::string &strText, int iProgress)
{
  if (!m_progressHandle)
  {
    CGUIDialogExtendedProgressBar *loadingProgressDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
    if (!loadingProgressDialog)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_EXT_PROGRESS!");
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
