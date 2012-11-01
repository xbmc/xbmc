/*
 *      Copyright (C) 2012 Team XBMC
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
#include "GUIInfoManager.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"
#include "epg/EpgContainer.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRChannels::CGUIWindowPVRChannels(CGUIWindowPVR *parent, bool bRadio) :
  CGUIWindowPVRCommon(parent,
                      bRadio ? PVR_WINDOW_CHANNELS_RADIO : PVR_WINDOW_CHANNELS_TV,
                      bRadio ? CONTROL_BTNCHANNELS_RADIO : CONTROL_BTNCHANNELS_TV,
                      bRadio ? CONTROL_LIST_CHANNELS_RADIO: CONTROL_LIST_CHANNELS_TV),
  CThread("PVR Channel Window")
{
  m_bRadio              = bRadio;
  m_bShowHiddenChannels = false;
  m_bThreadCreated      = false;
}

CGUIWindowPVRChannels::~CGUIWindowPVRChannels(void)
{
  if (m_bThreadCreated)
    StopThread(true);
}

void CGUIWindowPVRChannels::ResetObservers(void)
{
  CSingleLock lock(m_critSection);
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

void CGUIWindowPVRChannels::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);
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
    buttons.Add(CONTEXT_BUTTON_SET_THUMB, 20019);                                     /* change icon */
    buttons.Add(CONTEXT_BUTTON_GROUP_MANAGER, 19048);                                 /* group manager */
    buttons.Add(CONTEXT_BUTTON_HIDE, m_bShowHiddenChannels ? 19049 : 19054);          /* show/hide channel */

    if (m_parent->m_vecItems->Size() > 1 && !m_bShowHiddenChannels)
      buttons.Add(CONTEXT_BUTTON_MOVE, 116);                                          /* move channel up or down */

    if (m_bShowHiddenChannels || g_PVRChannelGroups->GetGroupAllTV()->GetNumHiddenChannels() > 0)
      buttons.Add(CONTEXT_BUTTON_SHOW_HIDDEN, m_bShowHiddenChannels ? 19050 : 19051); /* show hidden/visible channels */

    if (g_PVRClients->HasMenuHooks(pItem->GetPVRChannelInfoTag()->ClientID()))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);                                  /* PVR client specific action */

    CPVRChannel *channel = pItem->GetPVRChannelInfoTag();
    buttons.Add(CONTEXT_BUTTON_ADD_LOCK, channel->IsLocked() ? 19258 : 19257);        /* show lock/unlock channel */

    buttons.Add(CONTEXT_BUTTON_FILTER, 19249);                                        /* filter channels */
    buttons.Add(CONTEXT_BUTTON_UPDATE_EPG, 19251);                                    /* update EPG information */
  }
}

bool CGUIWindowPVRChannels::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= (int) m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

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
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

CPVRChannelGroupPtr CGUIWindowPVRChannels::SelectedGroup(void)
{
  if (!m_selectedGroup)
    SetSelectedGroup(g_PVRManager.GetPlayingGroup(m_bRadio));

  return m_selectedGroup;
}

void CGUIWindowPVRChannels::SetSelectedGroup(CPVRChannelGroupPtr group)
{
  if (!group)
    return;

  if (m_selectedGroup)
    m_selectedGroup->UnregisterObserver(this);
  m_selectedGroup = group;
  m_selectedGroup->RegisterObserver(this);
  g_PVRManager.SetPlayingGroup(m_selectedGroup);
}

void CGUIWindowPVRChannels::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageChannelGroup || msg == ObservableMessageTimers || msg == ObservableMessageEpgActiveItem || msg == ObservableMessageCurrentItem)
  {
    if (IsVisible())
      SetInvalid();
    else
      m_bUpdateRequired = true;
  }
  else if (msg == ObservableMessageChannelGroupReset)
  {
    if (IsVisible())
      UpdateData(true);
    else
      m_bUpdateRequired = true;
  }
}

CPVRChannelGroupPtr CGUIWindowPVRChannels::SelectNextGroup(void)
{
  CPVRChannelGroupPtr currentGroup = SelectedGroup();
  CPVRChannelGroupPtr nextGroup = currentGroup->GetNextGroup();
  while (nextGroup && *nextGroup != *currentGroup && nextGroup->Size() == 0)
    nextGroup = nextGroup->GetNextGroup();

  /* always update so users can reset the list */
  if (nextGroup)
  {
    SetSelectedGroup(nextGroup);
    UpdateData();
  }

  return m_selectedGroup;
}

void CGUIWindowPVRChannels::UpdateData(bool bUpdateSelectedFile /* = true */)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG, "CGUIWindowPVRChannels - %s - update window '%s'. set view to %d",
      __FUNCTION__, GetName(), m_iControlList);
  m_bUpdateRequired = false;

  /* lock the graphics context while updating */
  CSingleLock graphicsLock(g_graphicsContext);

  CPVRChannelGroupPtr selectedGroup = SelectedGroup();

  m_iSelected = m_parent->m_viewControl.GetSelectedItem();
  m_parent->m_viewControl.Clear();
  m_parent->m_vecItems->Clear();
  m_parent->m_viewControl.SetCurrentView(m_iControlList);

  CPVRChannelGroupPtr currentGroup = g_PVRManager.GetPlayingGroup(m_bRadio);
  if (!currentGroup)
    return;

  SetSelectedGroup(currentGroup);

  CStdString strPath;
  strPath.Format("pvr://channels/%s/%s/",
      m_bRadio ? "radio" : "tv",
      m_bShowHiddenChannels ? ".hidden" : currentGroup->GroupName());

  m_parent->m_vecItems->SetPath(strPath);
  m_parent->Update(m_parent->m_vecItems->GetPath());
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);

  if (bUpdateSelectedFile)
  {
    if (!SelectPlayingFile())
      m_parent->m_viewControl.SetSelectedItem(m_iSelected);
  }

  /* empty list */
  if (m_parent->m_vecItems->Size() == 0)
  {
    if (m_bShowHiddenChannels)
    {
      /* show the visible channels instead */
      m_bShowHiddenChannels = false;
      graphicsLock.Leave();
      lock.Leave();

      UpdateData(bUpdateSelectedFile);
      return;
    }
    else if (currentGroup->GroupID() > 0)
    {
      if (*currentGroup != *SelectNextGroup())
        return;
    }
  }

  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(m_bRadio ? 19024 : 19023));
  if (m_bShowHiddenChannels)
    m_parent->SetLabel(CONTROL_LABELGROUP, g_localizeStrings.Get(19022));
  else
    m_parent->SetLabel(CONTROL_LABELGROUP, currentGroup->GroupName());

  if (!m_bThreadCreated)
  {
    m_bThreadCreated = true;
    Create();
    SetPriority(-1);
  }
}

bool CGUIWindowPVRChannels::OnClickButton(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedButton(message))
  {
    bReturn = true;
    SelectNextGroup();
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnClickList(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedList(message))
  {
    bReturn = true;
    int iAction = message.GetParam1();
    int iItem = m_parent->m_viewControl.GetSelectedItem();

    if (iItem < 0 || iItem >= (int) m_parent->m_vecItems->Size())
      return bReturn;
    CFileItemPtr pItem = m_parent->m_vecItems->Get(iItem);

    /* process actions */
    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK || iAction == ACTION_PLAY)
      ActionPlayChannel(pItem.get());
    else if (iAction == ACTION_SHOW_INFO)
      ShowEPGInfo(pItem.get());
    else if (iAction == ACTION_DELETE_ITEM)
      ActionDeleteChannel(pItem.get());
    else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      m_parent->OnPopupMenu(iItem);
    else
      bReturn = false;
  }

  return bReturn;
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
    UpdateData();

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
    UpdateData();

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

    CStdString strIndex;
    strIndex.Format("%i", channel->ChannelNumber());
    CGUIDialogNumeric::ShowAndGetNumber(strIndex, g_localizeStrings.Get(19052));
    int newIndex = atoi(strIndex.c_str());

    if (newIndex != channel->ChannelNumber())
    {
      g_PVRManager.GetPlayingGroup()->MoveChannel(channel->ChannelNumber(), newIndex);
      UpdateData();
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
    bReturn = PlayFile(item, g_guiSettings.GetBool("pvrplayback.playminimized"));
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonSetThumb(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SET_THUMB)
  {
    if (g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return bReturn;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return bReturn;

    /* setup our thumb list */
    CFileItemList items;
    CPVRChannel *channel = item->GetPVRChannelInfoTag();

    if (!channel->IconPath().IsEmpty())
    {
      /* add the current thumb, if available */
      CFileItemPtr current(new CFileItem("thumb://Current", false));
      current->SetArt("thumb", channel->IconPath());
      current->SetLabel(g_localizeStrings.Get(20016));
      items.Add(current);
    }
    else if (item->HasArt("thumb"))
    {
      /* already have a thumb that the share doesn't know about - must be a local one, so we may as well reuse it */
      CFileItemPtr current(new CFileItem("thumb://Current", false));
      current->SetArt("thumb", item->GetArt("thumb"));
      current->SetLabel(g_localizeStrings.Get(20016));
      items.Add(current);
    }

    /* and add a "no thumb" entry as well */
    CFileItemPtr nothumb(new CFileItem("thumb://None", false));
    nothumb->SetIconImage(item->GetIconImage());
    nothumb->SetLabel(g_localizeStrings.Get(20018));
    items.Add(nothumb);

    CStdString strThumb;
    VECSOURCES shares;
    if (g_guiSettings.GetString("pvrmenu.iconpath") != "")
    {
      CMediaSource share1;
      share1.strPath = g_guiSettings.GetString("pvrmenu.iconpath");
      share1.strName = g_localizeStrings.Get(19018);
      shares.push_back(share1);
    }
    g_mediaManager.GetLocalDrives(shares);
    if (!CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(1030), strThumb))
      return bReturn;

    if (strThumb != "thumb://Current")
    {
      if (strThumb == "thumb://None")
        strThumb = "";

      channel->SetIconPath(strThumb, true);
      channel->Persist();
      UpdateData();
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
    UpdateData();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRChannels::OnContextButtonFilter(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_FILTER)
  {
    CStdString filter = m_parent->GetProperty("filter").asString();
    CGUIKeyboardFactory::ShowAndGetFilter(filter, false);
    m_parent->OnFilterItems(filter);

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

    CStdString strMessage;
    strMessage.Format("%s: '%s'", g_localizeStrings.Get(bReturn ? 19253 : 19254), channel->ChannelName());
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

void CGUIWindowPVRChannels::Process(void)
{
  // ugly hack to refresh the progress bars and item contents every 5 seconds
  int iCount(0);
  while (!m_bStop)
  {
    if (++iCount == 100)
    {
      iCount = 0;
      SetInvalid();
    }
    Sleep(50);
  }
}
