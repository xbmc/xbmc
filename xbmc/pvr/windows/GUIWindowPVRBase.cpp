/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRBase.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/PVRThumbLoader.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/filesystem/PVRGUIDirectory.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

#define MAX_INVALIDATION_FREQUENCY 2000ms // limit to one invalidation per X milliseconds

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
  std::shared_ptr<CPVRChannelGroup> GetSelectedChannelGroup() const;
  bool SelectChannelGroup(const std::shared_ptr<CPVRChannelGroup>& newGroup);

private:
  CGUIControl* m_control = nullptr;
  std::vector<std::shared_ptr<CPVRChannelGroup>> m_channelGroups;
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
    CPVRGUIDirectory::GetChannelGroupsDirectory(bRadio, true, channelGroupItems);

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

std::shared_ptr<CPVRChannelGroup> CGUIPVRChannelGroupsSelector::GetSelectedChannelGroup() const
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
  return std::shared_ptr<CPVRChannelGroup>();
}

bool CGUIPVRChannelGroupsSelector::SelectChannelGroup(const std::shared_ptr<CPVRChannelGroup>& newGroup)
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

CGUIWindowPVRBase::CGUIWindowPVRBase(bool bRadio, int id, const std::string& xmlFile)
  : CGUIMediaWindow(id, xmlFile.c_str()),
    m_bRadio(bRadio),
    m_channelGroupsSelector(new CGUIPVRChannelGroupsSelector)
{
  // prevent removable drives to appear in directory listing (base class default behavior).
  m_rootDir.AllowNonLocalSources(false);

  CServiceBroker::GetPVRManager().Events().Subscribe(this, &CGUIWindowPVRBase::Notify);
}

CGUIWindowPVRBase::~CGUIWindowPVRBase()
{
  if (m_channelGroup)
    m_channelGroup->Events().Unsubscribe(this);

  CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
}

void CGUIWindowPVRBase::UpdateSelectedItemPath()
{
  CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().SetSelectedChannelPath(
      m_bRadio, m_viewControl.GetSelectedItemPath());
}

void CGUIWindowPVRBase::Notify(const PVREvent& event)
{
  // call virtual event handler function
  NotifyEvent(event);
}

void CGUIWindowPVRBase::NotifyEvent(const PVREvent& event)
{
  if (event == PVREvent::ManagerStopped)
  {
    ClearData();
  }
  else if (m_active)
  {
    if (event == PVREvent::SystemSleep)
    {
      CGUIMessage m(GUI_MSG_SYSTEM_SLEEP, GetID(), 0, static_cast<int>(event));
      CServiceBroker::GetAppMessenger()->SendGUIMessage(m);
    }
    else if (event == PVREvent::SystemWake)
    {
      CGUIMessage m(GUI_MSG_SYSTEM_WAKE, GetID(), 0, static_cast<int>(event));
      CServiceBroker::GetAppMessenger()->SendGUIMessage(m);
    }
    else
    {
      CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, static_cast<int>(event));
      CServiceBroker::GetAppMessenger()->SendGUIMessage(m);
    }
  }
}

bool CGUIWindowPVRBase::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_PREVIOUS_CHANNELGROUP:
      ActivatePreviousChannelGroup();
      return true;

    case ACTION_NEXT_CHANNELGROUP:
      ActivateNextChannelGroup();
      return true;

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

bool CGUIWindowPVRBase::ActivatePreviousChannelGroup()
{
  const std::shared_ptr<const CPVRChannelGroup> channelGroup = GetChannelGroup();
  if (channelGroup)
  {
    const CPVRChannelGroups* groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(channelGroup->IsRadio());
    if (groups)
    {
      SetChannelGroup(groups->GetPreviousGroup(*channelGroup));
      return true;
    }
  }
  return false;
}

bool CGUIWindowPVRBase::ActivateNextChannelGroup()
{
  const std::shared_ptr<const CPVRChannelGroup> channelGroup = GetChannelGroup();
  if (channelGroup)
  {
    const CPVRChannelGroups* groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(channelGroup->IsRadio());
    if (groups)
    {
      SetChannelGroup(groups->GetNextGroup(*channelGroup));
      return true;
    }
  }
  return false;
}

void CGUIWindowPVRBase::ClearData()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_channelGroup.reset();
  m_channelGroupsSelector = std::make_unique<CGUIPVRChannelGroupsSelector>();
}

void CGUIWindowPVRBase::OnInitWindow()
{
  SetProperty("IsRadio", m_bRadio ? "true" : "");

  if (InitChannelGroup())
  {
    m_channelGroupsSelector->Initialize(this, m_bRadio);

    CGUIMediaWindow::OnInitWindow();

    // mark item as selected by channel path
    m_viewControl.SetSelectedItem(
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetSelectedChannelPath(m_bRadio));

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
      switch (static_cast<PVREvent>(message.GetParam1()))
      {
        case PVREvent::ManagerStarted:
        case PVREvent::ClientsInvalidated:
        case PVREvent::ChannelGroupsInvalidated:
        {
          if (InitChannelGroup())
          {
            m_channelGroupsSelector->Initialize(this, m_bRadio);
            m_channelGroupsSelector->SelectChannelGroup(GetChannelGroup());
            HideProgressDialog();
            Refresh(true);
            m_viewControl.SetFocused();
          }
          break;
        }
        default:
          break;
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
        case GUI_MSG_UPDATE:
        {
          if (IsActive() && m_bUpdating)
          {
            // no concurrent updates
            CLog::LogF(LOGWARNING, "GUI_MSG_UPDATE: Updating in progress");
            bReturn = true;
          }
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
    for (const auto& item : *m_vecItems)
      item->SetInvalid();

    CGUIMediaWindow::SetInvalid();
    m_refreshTimeout.Set(MAX_INVALIDATION_FREQUENCY);
  }
}

bool CGUIWindowPVRBase::CanBeActivated() const
{
  // check if there is at least one enabled PVR add-on
  if (!CServiceBroker::GetAddonMgr().HasAddons(ADDON::AddonType::PVRDLL))
  {
    HELPERS::ShowOKDialogText(CVariant{19296}, CVariant{19272}); // No PVR add-on enabled, You need a tuner, backend software...
    return false;
  }

  return true;
}

bool CGUIWindowPVRBase::OpenChannelGroupSelectionDialog()
{
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (!dialog)
    return false;

  CFileItemList options;
  CPVRGUIDirectory::GetChannelGroupsDirectory(m_bRadio, true, options);

  dialog->Reset();
  dialog->SetHeading(CVariant{g_localizeStrings.Get(19146)});
  dialog->SetMultiSelection(false);

  auto& pvrMgr = CServiceBroker::GetPVRManager();
  const bool useDetails = pvrMgr.Clients()->CreatedClientAmount() > 1;
  dialog->SetUseDetails(useDetails);
  if (useDetails)
  {
    std::string selectedName;
    std::string selectedClient;

    const std::shared_ptr<const CPVRChannelGroup> channelGroup = GetChannelGroup();
    if (channelGroup)
    {
      selectedName = channelGroup->GroupName();

      auto client = pvrMgr.GetClient(channelGroup->GetClientID());
      if (client)
        selectedClient = client->GetFriendlyName();
    }

    CPVRThumbLoader loader;
    int idx = 0;
    for (auto& group : options)
    {
      // set client name as label2
      const std::shared_ptr<const CPVRClient> client = pvrMgr.GetClient(*group);
      if (client)
        group->SetLabel2(client->GetFriendlyName());

      // set thumbnail
      loader.LoadItem(group.get());

      // if not yet done, find and select currently active channel group
      if (idx >= 0)
      {
        if (group->GetLabel() == selectedName && group->GetLabel2() == selectedClient)
        {
          dialog->SetSelected(idx);
          idx = -1; // done
        }
        else
        {
          idx++;
        }
      }
    }
  }
  else
  {
    const std::shared_ptr<const CPVRChannelGroup> channelGroup = GetChannelGroup();
    if (channelGroup)
    {
      int idx = -1;
      const std::string selectedName = channelGroup->GroupName();
      for (auto& group : options)
      {
        // select currently active channel group
        if (group->GetLabel() == selectedName)
        {
          dialog->SetSelected(idx);
          break;
        }
        idx++;
      }
    }
  }

  dialog->SetItems(options);

  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  const CFileItemPtr item = dialog->GetSelectedFileItem();
  if (!item)
    return false;

  SetChannelGroup(pvrMgr.ChannelGroups()->Get(m_bRadio)->GetGroupByPath(item->GetPath()));

  return true;
}

bool CGUIWindowPVRBase::InitChannelGroup()
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return false;

  std::shared_ptr<CPVRChannelGroup> group;
  if (m_channelGroupPath.empty())
  {
    group = CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(m_bRadio);
  }
  else
  {
    group = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bRadio)->GetGroupByPath(m_channelGroupPath);
    if (group)
      CServiceBroker::GetPVRManager().PlaybackState()->SetActiveChannelGroup(group);
    else
      CLog::LogF(LOGERROR, "Found no {} channel group with path '{}'!", m_bRadio ? "radio" : "TV",
                 m_channelGroupPath);

    m_channelGroupPath.clear();
  }

  if (group)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
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

std::shared_ptr<CPVRChannelGroup> CGUIWindowPVRBase::GetChannelGroup()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_channelGroup;
}

void CGUIWindowPVRBase::SetChannelGroup(std::shared_ptr<CPVRChannelGroup> &&group, bool bUpdate /* = true */)
{
  if (!group)
    return;

  std::shared_ptr<CPVRChannelGroup> updateChannelGroup;
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (m_channelGroup != group)
    {
      if (m_channelGroup)
        m_channelGroup->Events().Unsubscribe(this);
      m_channelGroup = std::move(group);
      // we need to register the window to receive changes from the new group
      m_channelGroup->Events().Subscribe(this, &CGUIWindowPVRBase::Notify);
      if (bUpdate)
        updateChannelGroup = m_channelGroup;
    }
  }

  if (updateChannelGroup)
  {
    CServiceBroker::GetPVRManager().PlaybackState()->SetActiveChannelGroup(updateChannelGroup);
    Update(GetDirectoryPath());
  }
}

bool CGUIWindowPVRBase::Update(const std::string& strDirectory, bool updateFilterPath /*= true*/)
{
  if (m_bUpdating)
  {
    // no concurrent updates
    CLog::LogF(LOGWARNING, "Updating in progress");
    return false;
  }

  CUpdateGuard guard(m_bUpdating);

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

void CGUIWindowPVRBase::UpdateButtons()
{
  CGUIMediaWindow::UpdateButtons();

  const std::shared_ptr<CPVRChannelGroup> channelGroup = GetChannelGroup();
  if (channelGroup)
  {
    SET_CONTROL_LABEL(CONTROL_BTNCHANNELGROUPS, g_localizeStrings.Get(19141) + ": " + channelGroup->GroupName());
  }

  m_channelGroupsSelector->SelectChannelGroup(channelGroup);
}

void CGUIWindowPVRBase::ShowProgressDialog(const std::string& strText, int iProgress)
{
  if (!m_progressHandle)
  {
    CGUIDialogExtendedProgressBar* loadingProgressDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
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

void CGUIWindowPVRBase::HideProgressDialog()
{
  if (m_progressHandle)
  {
    m_progressHandle->MarkFinished();
    m_progressHandle = nullptr;
  }
}
