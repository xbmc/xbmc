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


#include "GUIInfoManager.h"
#include "epg/EpgContainer.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "ServiceBroker.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"

#include "GUIWindowPVRChannels.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRChannels::CGUIWindowPVRChannels(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_CHANNELS : WINDOW_TV_CHANNELS, "MyPVRChannels.xml"),
  CPVRChannelNumberInputHandler(1000),
  m_bShowHiddenChannels(false)
{
  CServiceBroker::GetPVRManager().EpgContainer()->RegisterObserver(this);
  g_infoManager.RegisterObserver(this);
}

CGUIWindowPVRChannels::~CGUIWindowPVRChannels()
{
  g_infoManager.UnregisterObserver(this);
  CServiceBroker::GetPVRManager().EpgContainer()->UnregisterObserver(this);
}

void CGUIWindowPVRChannels::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  // Add parent buttons before the Manage button
  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);

  buttons.Add(CONTEXT_BUTTON_EDIT, 16106); /* Manage... */
}

std::string CGUIWindowPVRChannels::GetDirectoryPath(void)
{
  return StringUtils::Format("pvr://channels/%s/%s/",
      m_bRadio ? "radio" : "tv",
      m_bShowHiddenChannels ? ".hidden" : GetChannelGroup()->GroupName().c_str());
}

bool CGUIWindowPVRChannels::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;

  return OnContextButtonManage(m_vecItems->Get(itemNumber), button) ||
      CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRChannels::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  if (bReturn)
  {
    CSingleLock lock(m_critSection);
    /* empty list for hidden channels */
    if (m_vecItems->GetObjectCount() == 0 && m_bShowHiddenChannels)
    {
      /* show the visible channels instead */
      m_bShowHiddenChannels = false;
      lock.Leave();
      Update(GetDirectoryPath());
    }
  }
  return bReturn;
}

void CGUIWindowPVRChannels::UpdateButtons(void)
{
  CGUIRadioButtonControl *btnShowHidden = (CGUIRadioButtonControl*) GetControl(CONTROL_BTNSHOWHIDDEN);
  if (btnShowHidden)
  {
    btnShowHidden->SetVisible(CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_bRadio)->GetNumHiddenChannels() > 0);
    btnShowHidden->SetSelected(m_bShowHiddenChannels);
  }

  CGUIWindowPVRBase::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, m_bShowHiddenChannels ? g_localizeStrings.Get(19022) : GetChannelGroup()->GroupName());
}

bool CGUIWindowPVRChannels::OnAction(const CAction &action)
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
      AppendChannelNumberDigit(action.GetID() - REMOTE_0);
      return true;
  }

  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRChannels::OnMessage(CGUIMessage& message)
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
           case ACTION_SELECT_ITEM:
           case ACTION_MOUSE_LEFT_CLICK:
           case ACTION_PLAY:
             CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(m_vecItems->Get(iItem), true);
             break;
           case ACTION_SHOW_INFO:
             CServiceBroker::GetPVRManager().GUIActions()->ShowEPGInfo(m_vecItems->Get(iItem));
             break;
           case ACTION_DELETE_ITEM:
             CServiceBroker::GetPVRManager().GUIActions()->HideChannel(m_vecItems->Get(iItem));
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
        CGUIRadioButtonControl *radioButton = (CGUIRadioButtonControl*)GetControl(CONTROL_BTNSHOWHIDDEN);
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
      switch(message.GetParam1())
      {
        case ObservableMessageChannelGroup:
        case ObservableMessageTimers:
        case ObservableMessageEpg:
        case ObservableMessageEpgContainer:
        case ObservableMessageEpgActiveItem:
        case ObservableMessageCurrentItem:
        case ObservableMessageRecordings:
        {
          SetInvalid();
          break;
        }
        case ObservableMessageChannelGroupReset:
        {
          Refresh(true);
          break;
        }
      }
      break;
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRChannels::OnContextButtonManage(const CFileItemPtr &item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_EDIT)
  {
    // Create context sub menu
    CContextButtons buttons;
    buttons.Add(CONTEXT_BUTTON_GROUP_MANAGER, 19048);
    buttons.Add(CONTEXT_BUTTON_CHANNEL_MANAGER, 19199);
    buttons.Add(CONTEXT_BUTTON_UPDATE_EPG, 19251);

    // Get the response
    int button = CGUIDialogContextMenu::ShowAndGetChoice(buttons);
    if (button >= 0)
    {
      switch ((CONTEXT_BUTTON)button)
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

void CGUIWindowPVRChannels::UpdateEpg(const CFileItemPtr &item)
{
  const CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

  if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{19251}, // "Update guide information"
                                        CVariant{19252}, // "Schedule guide update for this channel?"
                                        CVariant{""},
                                        CVariant{channel->ChannelName()}))
    return;

  const CEpgPtr epg = channel->GetEPG();
  if (epg)
  {
    epg->ForceUpdate();

    const std::string strMessage = StringUtils::Format("%s: '%s'",
                                                       g_localizeStrings.Get(19253).c_str(), // "Guide update scheduled for channel"
                                                       channel->ChannelName().c_str());
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                          g_localizeStrings.Get(19166), // "PVR information"
                                          strMessage);
  }
  else
  {
    const std::string strMessage = StringUtils::Format("%s: '%s'",
                                                       g_localizeStrings.Get(19254).c_str(), // "Guide update failed for channel"
                                                       channel->ChannelName().c_str());
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
                                          g_localizeStrings.Get(19166), // "PVR information"
                                          strMessage);
  }
}

void CGUIWindowPVRChannels::ShowChannelManager()
{
  CGUIDialogPVRChannelManager *dialog = g_windowManager.GetWindow<CGUIDialogPVRChannelManager>();
  if (dialog)
    dialog->Open();
}

void CGUIWindowPVRChannels::ShowGroupManager(void)
{
  /* Load group manager dialog */
  CGUIDialogPVRGroupManager* pDlgInfo = g_windowManager.GetWindow<CGUIDialogPVRGroupManager>();
  if (!pDlgInfo)
    return;

  pDlgInfo->SetRadio(m_bRadio);
  pDlgInfo->Open();

  return;
}

void CGUIWindowPVRChannels::OnInputDone()
{
  const int iChannelNumber = GetChannelNumber();
  if (iChannelNumber >= 0)
  {
    int itemIndex = 0;
    for (const CFileItemPtr channel : m_vecItems->GetList())
    {
      if (channel->GetPVRChannelInfoTag()->ChannelNumber() == iChannelNumber)
      {
        m_viewControl.SetSelectedItem(itemIndex);
        return;
      }
      ++itemIndex;
    }
  }
}
