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

#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "GUIInfoManager.h"
#include "profiles/ProfilesManager.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"
#include "epg/EpgContainer.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
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
  CPVRChannel *channel = pItem->GetPVRChannelInfoTag();

  if (pItem->GetPath() == "pvr://channels/.add.channel")
  {
    /* If yes show only "New Channel" on context menu */
    buttons.Add(CONTEXT_BUTTON_ADD, 19046);                                           /* add new channel */
  }
  else
  {
    buttons.Add(CONTEXT_BUTTON_INFO, 19047);                                          /* channel info */
    buttons.Add(CONTEXT_BUTTON_FIND, 19003);                                          /* find similar program */
    buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);                                     /* switch to channel */
    buttons.Add(CONTEXT_BUTTON_RECORD_ITEM, channel->IsRecording() ? 19256 : 19255);  /* start/stop recording on channel */
    buttons.Add(CONTEXT_BUTTON_SET_THUMB, 19284);                                     /* change icon */
    buttons.Add(CONTEXT_BUTTON_GROUP_MANAGER, 19048);                                 /* group manager */
    buttons.Add(CONTEXT_BUTTON_HIDE, m_bShowHiddenChannels ? 19049 : 19054);          /* show/hide channel */

    if (m_vecItems->Size() > 1 && !m_bShowHiddenChannels)
      buttons.Add(CONTEXT_BUTTON_MOVE, 116);                                          /* move channel up or down */

    if (m_bShowHiddenChannels || g_PVRChannelGroups->GetGroupAllTV()->GetNumHiddenChannels() > 0)
      buttons.Add(CONTEXT_BUTTON_SHOW_HIDDEN, m_bShowHiddenChannels ? 19050 : 19051); /* show hidden/visible channels */

    if (g_PVRClients->HasMenuHooks(pItem->GetPVRChannelInfoTag()->ClientID(), PVR_MENUHOOK_CHANNEL))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);                                  /* PVR client specific action */

    CPVRChannel *channel = pItem->GetPVRChannelInfoTag();
    buttons.Add(CONTEXT_BUTTON_ADD_LOCK, channel->IsLocked() ? 19258 : 19257);        /* show lock/unlock channel */

    buttons.Add(CONTEXT_BUTTON_FILTER, 19249);                                        /* filter channels */
    buttons.Add(CONTEXT_BUTTON_UPDATE_EPG, 19251);                                    /* update EPG information */
  }

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
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

  return OnContextButtonPlay(pItem.get(), button) ||
      OnContextButtonMove(pItem.get(), button) ||
      OnContextButtonHide(pItem.get(), button) ||
      OnContextButtonShowHidden(pItem.get(), button) ||
      OnContextButtonSetThumb(pItem.get(), button) ||
      OnContextButtonAdd(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonGroupManager(pItem.get(), button) ||
      OnContextButtonFilter(pItem.get(), button) ||
      OnContextButtonUpdateEpg(pItem.get(), button) ||
      OnContextButtonRecord(pItem.get(), button) ||
      OnContextButtonLock(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowPVRChannels::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  CSingleLock lock(m_critSection);
  bool bReturn = CGUIWindowPVRBase::Update(strDirectory);

  /* empty list for hidden channels */
  if (m_vecItems->Size() == 0 && m_bShowHiddenChannels)
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
    CGUIDialogOK::ShowAndGetInput(19033,0,19038,0);
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

bool CGUIWindowPVRChannels::OnContextButtonHide(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_HIDE)
  {
    CPVRChannel *channel = item->GetPVRChannelInfoTag();
    if (!channel || channel->IsRadio() != m_bRadio)
      return bReturn;

    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;

    pDialog->SetHeading(19039);
    pDialog->SetLine(0, "");
    pDialog->SetLine(1, channel->ChannelName());
    pDialog->SetLine(2, "");
    pDialog->DoModal();

    if (!pDialog->IsConfirmed())
      return bReturn;

    g_PVRManager.GetPlayingGroup(m_bRadio)->RemoveFromGroup(*channel);
    Refresh(true);

    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonLock(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ADD_LOCK)
  {
    // ask for PIN first
    if (!g_PVRManager.CheckParentalPIN(g_localizeStrings.Get(19262).c_str()))
      return bReturn;

    CPVRChannelGroupPtr group = g_PVRChannelGroups->GetGroupAll(m_bRadio);
    if (!group)
      return bReturn;

    group->ToggleChannelLocked(*item);
    Refresh(true);

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

bool CGUIWindowPVRChannels::OnContextButtonMove(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_MOVE)
  {
    CPVRChannel *channel = item->GetPVRChannelInfoTag();
    if (!channel || channel->IsRadio() != m_bRadio)
      return bReturn;

    std::string strIndex;
    strIndex = StringUtils::Format("%i", channel->ChannelNumber());
    CGUIDialogNumeric::ShowAndGetNumber(strIndex, g_localizeStrings.Get(19052));
    int newIndex = atoi(strIndex.c_str());

    if (newIndex != channel->ChannelNumber())
    {
      g_PVRManager.GetPlayingGroup()->MoveChannel(channel->ChannelNumber(), newIndex);
      Refresh(true);
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_PLAY_ITEM)
  {
    /* play channel */
    bReturn = PlayFile(item, CSettings::Get().GetBool("pvrplayback.playminimized"));
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonSetThumb(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SET_THUMB)
  {
    if (CProfilesManager::Get().GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return bReturn;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return bReturn;

    /* setup our thumb list */
    CFileItemList items;
    CPVRChannel *channel = item->GetPVRChannelInfoTag();

    if (!channel->IconPath().empty())
    {
      /* add the current icon, if available */
      CFileItemPtr current(new CFileItem("thumb://Current", false));
      current->SetArt("thumb", channel->IconPath());
      current->SetLabel(g_localizeStrings.Get(19282));
      items.Add(current);
    }
    else if (item->HasArt("thumb"))
    {
      /* already have a thumb that the share doesn't know about - must be a local one, so we may as well reuse it */
      CFileItemPtr current(new CFileItem("thumb://Current", false));
      current->SetArt("thumb", item->GetArt("thumb"));
      current->SetLabel(g_localizeStrings.Get(19282));
      items.Add(current);
    }

    /* and add a "no thumb" entry as well */
    CFileItemPtr nothumb(new CFileItem("thumb://None", false));
    nothumb->SetIconImage(item->GetIconImage());
    nothumb->SetLabel(g_localizeStrings.Get(19283));
    items.Add(nothumb);

    std::string strThumb;
    VECSOURCES shares;
    if (CSettings::Get().GetString("pvrmenu.iconpath") != "")
    {
      CMediaSource share1;
      share1.strPath = CSettings::Get().GetString("pvrmenu.iconpath");
      share1.strName = g_localizeStrings.Get(19066);
      shares.push_back(share1);
    }
    g_mediaManager.GetLocalDrives(shares);
    if (!CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(19285), strThumb, NULL, 19285))
      return bReturn;

    if (strThumb != "thumb://Current")
    {
      if (strThumb == "thumb://None")
        strThumb = "";

      CPVRChannelGroupPtr group = g_PVRChannelGroups->GetGroupAll(channel->IsRadio());
      CPVRChannelPtr channelPtr = group->GetByUniqueID(channel->UniqueID());

      channelPtr->SetIconPath(strThumb, true);
      channelPtr->Persist();
      Refresh(true);
    }

    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonShowHidden(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SHOW_HIDDEN)
  {
    m_bShowHiddenChannels = !m_bShowHiddenChannels;
    Update(GetDirectoryPath());
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonFilter(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_FILTER)
  {
    std::string filter = GetProperty("filter").asString();
    CGUIKeyboardFactory::ShowAndGetFilter(filter, false);
    OnFilterItems(filter);

    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn(false);
  
  if (button == CONTEXT_BUTTON_RECORD_ITEM)
  {
    CPVRChannel *channel = item->GetPVRChannelInfoTag();

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

    CPVRChannel *channel = item->GetPVRChannelInfoTag();
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
