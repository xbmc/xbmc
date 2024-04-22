/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowPVRChannels.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <mutex>
#include <string>

using namespace PVR;

CGUIWindowPVRChannelsBase::CGUIWindowPVRChannelsBase(bool bRadio,
                                                     int id,
                                                     const std::string& xmlFile)
  : CGUIWindowPVRBase(bRadio, id, xmlFile)
{
  CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().RegisterChannelNumberInputHandler(this);
}

CGUIWindowPVRChannelsBase::~CGUIWindowPVRChannelsBase()
{
  CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().DeregisterChannelNumberInputHandler(
      this);
}

std::string CGUIWindowPVRChannelsBase::GetRootPath() const
{
  //! @todo Would it make sense to change GetRootPath() declaration in CGUIMediaWindow
  //! to be non-const to get rid of the const_cast's here?

  CGUIWindowPVRChannelsBase* pThis = const_cast<CGUIWindowPVRChannelsBase*>(this);
  if (pThis->InitChannelGroup())
    return pThis->GetDirectoryPath();

  return CGUIWindowPVRBase::GetRootPath();
}

void CGUIWindowPVRChannelsBase::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  // Add parent buttons before the Manage button
  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);

  buttons.Add(CONTEXT_BUTTON_EDIT, 16106); /* Manage... */
}

bool CGUIWindowPVRChannelsBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;

  return OnContextButtonManage(m_vecItems->Get(itemNumber), button) ||
         CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRChannelsBase::Update(const std::string& strDirectory,
                                       bool updateFilterPath /* = true */)
{
  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  if (bReturn)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    /* empty list for hidden channels */
    if (m_vecItems->GetObjectCount() == 0 && m_bShowHiddenChannels)
    {
      /* show the visible channels instead */
      m_bShowHiddenChannels = false;
      lock.unlock();
      Update(GetDirectoryPath());
    }
  }
  return bReturn;
}

void CGUIWindowPVRChannelsBase::UpdateButtons()
{
  CGUIRadioButtonControl* btnShowHidden =
      static_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_BTNSHOWHIDDEN));
  if (btnShowHidden)
  {
    btnShowHidden->SetVisible(CServiceBroker::GetPVRManager()
                                  .ChannelGroups()
                                  ->GetGroupAll(m_bRadio)
                                  ->HasHiddenChannels());
    btnShowHidden->SetSelected(m_bShowHiddenChannels);
  }

  CGUIWindowPVRBase::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, m_bShowHiddenChannels ? g_localizeStrings.Get(19022)
                                                                 : GetChannelGroup()->GroupName());
}

bool CGUIWindowPVRChannelsBase::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
      AppendChannelNumberCharacter(static_cast<char>(action.GetID() - REMOTE_0) + '0');
      return true;

    case ACTION_CHANNEL_NUMBER_SEP:
      AppendChannelNumberCharacter(CPVRChannelNumber::SEPARATOR);
      return true;
  }

  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRChannelsBase::OnMessage(CGUIMessage& message)
{
  bool bReturn = false;
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      const CPVRChannelsPath path(message.GetStringParam(0));
      if (path.IsValid() && path.IsChannelGroup())
      {
        // if a path to a channel group is given we must init
        // that group instead of last played/selected group
        if (path.GetGroupName() == "*") // all channels
        {
          // Replace wildcard with real group name
          const auto group =
              CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(path.IsRadio());
          m_channelGroupPath = group->GetPath();
        }
        else
        {
          m_channelGroupPath = message.GetStringParam(0);
        }
      }
      break;
    }

    case GUI_MSG_CLICKED:
      if (message.GetSenderId() == m_viewControl.GetCurrentControl())
      {
        if (message.GetParam1() == ACTION_SELECT_ITEM ||
            message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)
        {
          // If direct channel number input is active, select the entered channel.
          if (CServiceBroker::GetPVRManager()
                  .Get<PVR::GUI::Channels>()
                  .GetChannelNumberInputHandler()
                  .CheckInputAndExecuteAction())
          {
            bReturn = true;
            break;
          }
        }

        int iItem = m_viewControl.GetSelectedItem();
        if (iItem >= 0 && iItem < m_vecItems->Size())
        {
          bReturn = true;
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            case ACTION_PLAYER_PLAY:
              CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().SwitchToChannel(
                  *(m_vecItems->Get(iItem)), true);
              break;
            case ACTION_SHOW_INFO:
              CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ShowEPGInfo(
                  *(m_vecItems->Get(iItem)));
              break;
            case ACTION_DELETE_ITEM:
              CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().HideChannel(
                  *(m_vecItems->Get(iItem)));
              break;
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              break;
            default:
              bReturn = false;
              break;
          }
        }
      }
      else if (message.GetSenderId() == CONTROL_BTNSHOWHIDDEN)
      {
        CGUIRadioButtonControl* radioButton =
            static_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_BTNSHOWHIDDEN));
        if (radioButton)
        {
          m_bShowHiddenChannels = radioButton->IsSelected();
          Update(GetDirectoryPath());
        }

        bReturn = true;
      }
      else if (message.GetSenderId() == CONTROL_BTNFILTERCHANNELS)
      {
        std::string filter = GetProperty("filter").asString();
        CGUIKeyboardFactory::ShowAndGetFilter(filter, false);
        OnFilterItems(filter);
        UpdateButtons();

        bReturn = true;
      }
      break;

    case GUI_MSG_REFRESH_LIST:
    {
      switch (static_cast<PVREvent>(message.GetParam1()))
      {
        case PVREvent::ChannelGroup:
        case PVREvent::CurrentItem:
        case PVREvent::Epg:
        case PVREvent::EpgActiveItem:
        case PVREvent::EpgContainer:
        case PVREvent::RecordingsInvalidated:
        case PVREvent::Timers:
          SetInvalid();
          break;

        case PVREvent::ChannelGroupInvalidated:
          Refresh(true);
          break;

        default:
          break;
      }
      break;
    }
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRChannelsBase::OnContextButtonManage(const CFileItemPtr& item,
                                                      CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_EDIT)
  {
    // Create context sub menu
    CContextButtons buttons;
    buttons.Add(CONTEXT_BUTTON_GROUP_MANAGER, 19048);
    buttons.Add(CONTEXT_BUTTON_CHANNEL_MANAGER, 19199);

    if (item->HasPVRChannelInfoTag())
      buttons.Add(CONTEXT_BUTTON_UPDATE_EPG, 19251);

    // Get the response
    int choice = CGUIDialogContextMenu::ShowAndGetChoice(buttons);
    if (choice >= 0)
    {
      switch (static_cast<CONTEXT_BUTTON>(choice))
      {
        case CONTEXT_BUTTON_GROUP_MANAGER:
          ShowGroupManager();
          break;
        case CONTEXT_BUTTON_CHANNEL_MANAGER:
          ShowChannelManager();
          break;
        case CONTEXT_BUTTON_UPDATE_EPG:
          UpdateEpg(item);
          break;
        default:
          break;
      }

      // Refresh the list in case anything was changed
      Refresh(true);
    }

    bReturn = true;
  }

  return bReturn;
}

void CGUIWindowPVRChannelsBase::UpdateEpg(const CFileItemPtr& item)
{
  const std::shared_ptr<const CPVRChannel> channel(item->GetPVRChannelInfoTag());

  if (!CGUIDialogYesNo::ShowAndGetInput(
          CVariant{19251}, // "Update guide information"
          CVariant{19252}, // "Schedule guide update for this channel?"
          CVariant{""}, CVariant{channel->ChannelName()}))
    return;

  const std::shared_ptr<CPVREpg> epg = channel->GetEPG();
  if (epg)
  {
    epg->ForceUpdate();

    const std::string strMessage =
        StringUtils::Format("{}: '{}'",
                            g_localizeStrings.Get(19253), // "Guide update scheduled for channel"
                            channel->ChannelName());
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                          g_localizeStrings.Get(19166), // "PVR information"
                                          strMessage);
  }
  else
  {
    const std::string strMessage =
        StringUtils::Format("{}: '{}'",
                            g_localizeStrings.Get(19254), // "Guide update failed for channel"
                            channel->ChannelName());
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
                                          g_localizeStrings.Get(19166), // "PVR information"
                                          strMessage);
  }
}

void CGUIWindowPVRChannelsBase::ShowChannelManager()
{
  CGUIDialogPVRChannelManager* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRChannelManager>(
          WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
  if (!dialog)
    return;

  dialog->SetRadio(m_bRadio);

  const int iItem = m_viewControl.GetSelectedItem();
  dialog->Open(iItem >= 0 && iItem < m_vecItems->Size() ? m_vecItems->Get(iItem) : nullptr);
}

void CGUIWindowPVRChannelsBase::ShowGroupManager()
{
  /* Load group manager dialog */
  CGUIDialogPVRGroupManager* pDlgInfo =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRGroupManager>(
          WINDOW_DIALOG_PVR_GROUP_MANAGER);
  if (!pDlgInfo)
    return;

  pDlgInfo->SetRadio(m_bRadio);
  pDlgInfo->Open();
}

void CGUIWindowPVRChannelsBase::OnInputDone()
{
  const CPVRChannelNumber channelNumber = GetChannelNumber();
  if (channelNumber.IsValid())
  {
    int itemIndex = 0;
    for (const CFileItemPtr& channel : *m_vecItems)
    {
      if (channel->GetPVRChannelGroupMemberInfoTag()->ChannelNumber() == channelNumber)
      {
        m_viewControl.SetSelectedItem(itemIndex);
        return;
      }
      ++itemIndex;
    }
  }
}

void CGUIWindowPVRChannelsBase::GetChannelNumbers(std::vector<std::string>& channelNumbers)
{
  const std::shared_ptr<const CPVRChannelGroup> group = GetChannelGroup();
  if (group)
    group->GetChannelNumbers(channelNumbers);
}

CGUIWindowPVRTVChannels::CGUIWindowPVRTVChannels()
  : CGUIWindowPVRChannelsBase(false, WINDOW_TV_CHANNELS, "MyPVRChannels.xml")
{
}

std::string CGUIWindowPVRTVChannels::GetDirectoryPath()
{
  return CPVRChannelsPath(false, m_bShowHiddenChannels, GetChannelGroup()->GroupName(),
                          GetChannelGroup()->GetClientID());
}

CGUIWindowPVRRadioChannels::CGUIWindowPVRRadioChannels()
  : CGUIWindowPVRChannelsBase(true, WINDOW_RADIO_CHANNELS, "MyPVRChannels.xml")
{
}

std::string CGUIWindowPVRRadioChannels::GetDirectoryPath()
{
  return CPVRChannelsPath(true, m_bShowHiddenChannels, GetChannelGroup()->GroupName(),
                          GetChannelGroup()->GetClientID());
}
