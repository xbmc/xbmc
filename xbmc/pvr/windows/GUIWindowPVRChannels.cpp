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

#include "GUIWindowPVRChannels.h"

#include "ContextMenuManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "GUIInfoManager.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"
#include "epg/EpgContainer.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRChannels::CGUIWindowPVRChannels(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_CHANNELS : WINDOW_TV_CHANNELS, "MyPVRChannels.xml"),
  m_bShowHiddenChannels(false)
{
}

void CGUIWindowPVRChannels::ResetObservers(void)
{
  CSingleLock lock(m_critSection);
  UnregisterObservers();
  g_EpgContainer.RegisterObserver(this);
  g_PVRTimers->RegisterObserver(this);
  g_infoManager.RegisterObserver(this);
}

void CGUIWindowPVRChannels::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  g_EpgContainer.UnregisterObserver(this);
  if (g_PVRTimers)
    g_PVRTimers->UnregisterObserver(this);
  g_infoManager.UnregisterObserver(this);
}

void CGUIWindowPVRChannels::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);
  CPVRChannelPtr channel(pItem->GetPVRChannelInfoTag());

  buttons.Add(CONTEXT_BUTTON_INFO, 19047);                                          /* channel info */
  buttons.Add(CONTEXT_BUTTON_FIND, 19003);                                          /* find similar program */
  buttons.Add(CONTEXT_BUTTON_RECORD_ITEM, channel->IsRecording() ? 19256 : 19255);  /* start/stop recording on channel */

  if (g_PVRClients->HasMenuHooks(pItem->GetPVRChannelInfoTag()->ClientID(), PVR_MENUHOOK_CHANNEL))
    buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);                                  /* PVR client specific action */

  // Add parent buttons before the Manage button
  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);

  buttons.Add(CONTEXT_BUTTON_EDIT, 16106);                                          /* "Manage" submenu */

  CContextMenuManager::Get().AddVisibleItems(pItem, buttons);
}

std::string CGUIWindowPVRChannels::GetDirectoryPath(void)
{
  return StringUtils::Format("pvr://channels/%s/%s/",
      m_bRadio ? "radio" : "tv",
      m_bShowHiddenChannels ? ".hidden" : GetGroup()->GroupName().c_str());
}

bool CGUIWindowPVRChannels::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonAdd(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonGroupManager(pItem.get(), button) ||
      OnContextButtonUpdateEpg(pItem.get(), button) ||
      OnContextButtonRecord(pItem.get(), button) ||
      OnContextButtonManage(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRChannels::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  CSingleLock lock(m_critSection);
  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  /* empty list for hidden channels */
  if (m_vecItems->GetObjectCount() == 0 && m_bShowHiddenChannels)
  {
    /* show the visible channels instead */
    m_bShowHiddenChannels = false;
    lock.Leave();
    Update(GetDirectoryPath());
  }

  return bReturn;
}

void CGUIWindowPVRChannels::UpdateButtons(void)
{
  CGUIRadioButtonControl *btnShowHidden = (CGUIRadioButtonControl*) GetControl(CONTROL_BTNSHOWHIDDEN);
  if (btnShowHidden)
  {
    btnShowHidden->SetVisible(g_PVRChannelGroups->GetGroupAll(m_bRadio)->GetNumHiddenChannels() > 0);
    btnShowHidden->SetSelected(m_bShowHiddenChannels);
  }

  CGUIWindowPVRBase::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, m_bShowHiddenChannels ? g_localizeStrings.Get(19022) : GetGroup()->GroupName());
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
      return ActionInputChannelNumber(action.GetID() - REMOTE_0);
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
             ActionPlayChannel(m_vecItems->Get(iItem).get());
             break;
           case ACTION_SHOW_INFO:
             ShowEPGInfo(m_vecItems->Get(iItem).get());
             break;
           case ACTION_DELETE_ITEM:
             ActionDeleteChannel(m_vecItems->Get(iItem).get());
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

        bReturn = true;
      }
      break;
    case GUI_MSG_REFRESH_LIST:
      switch(message.GetParam1())
      {
        case ObservableMessageChannelGroup:
        case ObservableMessageTimers:
        case ObservableMessageEpgActiveItem:
        case ObservableMessageCurrentItem:
        {
          if (IsActive())
            SetInvalid();
          bReturn = true;
          break;
        }
        case ObservableMessageChannelGroupReset:
        {
          if (IsActive())
            Refresh(true);
          bReturn = true;
          break;
        }
      }
      break;
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRChannels::OnContextButtonAdd(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ADD)
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19038);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonGroupManager(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_GROUP_MANAGER)
  {
    ShowGroupManager();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    ShowEPGInfo(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonManage(CFileItem *item, CONTEXT_BUTTON button)
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
          OnContextButtonUpdateEpg(item, (CONTEXT_BUTTON)button);
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

bool CGUIWindowPVRChannels::OnContextButtonRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn(false);

  if (button == CONTEXT_BUTTON_RECORD_ITEM)
  {
    CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

    if (channel)
      return g_PVRManager.ToggleRecordingOnChannel(channel->ChannelID());
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonUpdateEpg(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_UPDATE_EPG)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;

    CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

    pDialog->SetHeading(19251);
    pDialog->SetLine(0, g_localizeStrings.Get(19252));
    pDialog->SetLine(1, channel->ChannelName());
    pDialog->SetLine(2, "");
    pDialog->DoModal();

    if (!pDialog->IsConfirmed())
      return bReturn;

    bReturn = UpdateEpgForChannel(item);

    std::string strMessage = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(bReturn ? 19253 : 19254).c_str(), channel->ChannelName().c_str());
    CGUIDialogKaiToast::QueueNotification(bReturn ? CGUIDialogKaiToast::Info : CGUIDialogKaiToast::Error,
        g_localizeStrings.Get(19166),
        strMessage);
  }

  return bReturn;
}

void CGUIWindowPVRChannels::ShowChannelManager()
{
  CGUIDialogPVRChannelManager *dialog = (CGUIDialogPVRChannelManager *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
  if (dialog)
    dialog->DoModal();
}

void CGUIWindowPVRChannels::ShowGroupManager(void)
{
  /* Load group manager dialog */
  CGUIDialogPVRGroupManager* pDlgInfo = (CGUIDialogPVRGroupManager*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GROUP_MANAGER);
  if (!pDlgInfo)
    return;

  pDlgInfo->SetRadio(m_bRadio);
  pDlgInfo->DoModal();

  return;
}
