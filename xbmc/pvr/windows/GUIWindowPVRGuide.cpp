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

#include "GUIWindowPVRGuide.h"

#include "ContextMenuManager.h"
#include "GUIUserMessages.h"
#include "input/Key.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "epg/EpgContainer.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"

#define MAX_UPDATE_FREQUENCY 3000 // limit to maximum one update/refresh in x milliseconds

using namespace PVR;
using namespace EPG;

CGUIWindowPVRGuide::CGUIWindowPVRGuide(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_GUIDE : WINDOW_TV_GUIDE, "MyPVRGuide.xml")
{
  m_bUpdateRequired = false;
  m_cachedTimeline = new CFileItemList;
  m_cachedChannelGroup = CPVRChannelGroupPtr(new CPVRChannelGroup);
}

CGUIWindowPVRGuide::~CGUIWindowPVRGuide(void)
{
  delete m_cachedTimeline;
}

void CGUIWindowPVRGuide::OnInitWindow()
{
  if (m_guiState.get())
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl(), false);

  CGUIEPGGridContainer *epgGridContainer =
    dynamic_cast<CGUIEPGGridContainer*>(GetControl(m_viewControl.GetCurrentControl()));
  if (epgGridContainer)
    epgGridContainer->GoToNow();

  CGUIWindowPVRBase::OnInitWindow();
}

void CGUIWindowPVRGuide::ResetObservers(void)
{
  UnregisterObservers();
  g_EpgContainer.RegisterObserver(this);
}

void CGUIWindowPVRGuide::UnregisterObservers(void)
{
  g_EpgContainer.UnregisterObserver(this);
}

void CGUIWindowPVRGuide::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);         /* switch channel */

  if (pItem->HasEPGInfoTag() && pItem->GetEPGInfoTag()->HasRecording())
    buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 19687);      /* play recording */

  CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(pItem.get());
  if (timer && timer->HasPVRTimerInfoTag())
  {
    if (timer->GetPVRTimerInfoTag()->IsRecording())
      buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059);  /* stop recording */
    else
      buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19060);  /* delete timer */
  }
  else if (pItem->HasEPGInfoTag() && pItem->GetEPGInfoTag()->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
  {
    if (pItem->GetEPGInfoTag()->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
      buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);   /* record */
    else
      buttons.Add(CONTEXT_BUTTON_START_RECORD, 19061); /* add timer */
  }

  buttons.Add(CONTEXT_BUTTON_INFO, 19047);              /* epg info */
  buttons.Add(CONTEXT_BUTTON_FIND, 19003);              /* find similar program */

  if (m_viewControl.GetCurrentControl() == GUIDE_VIEW_TIMELINE)
  {
    buttons.Add(CONTEXT_BUTTON_BEGIN, 19063);           /* go to begin */
    buttons.Add(CONTEXT_BUTTON_NOW, 19070);             /* go to now */
    buttons.Add(CONTEXT_BUTTON_END, 19064);             /* go to end */
  }

  if (pItem->HasEPGInfoTag() &&
      pItem->GetEPGInfoTag()->HasPVRChannel() &&
      g_PVRClients->HasMenuHooks(pItem->GetEPGInfoTag()->ChannelTag()->ClientID(), PVR_MENUHOOK_EPG))
    buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
  CContextMenuManager::Get().AddVisibleItems(pItem, buttons);
}

void CGUIWindowPVRGuide::UpdateSelectedItemPath()
{
  if (m_viewControl.GetCurrentControl() == GUIDE_VIEW_TIMELINE)
  {
    CGUIEPGGridContainer *epgGridContainer = (CGUIEPGGridContainer*) GetControl(m_viewControl.GetCurrentControl());
    if (epgGridContainer)
    {
      CPVRChannelPtr channel(epgGridContainer->GetChannel(epgGridContainer->GetSelectedChannel()));
      if (channel)
        SetSelectedItemPath(m_bRadio, channel->Path());
    }
  }
  else
    CGUIWindowPVRBase::UpdateSelectedItemPath();
}

bool CGUIWindowPVRGuide::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  switch (m_viewControl.GetCurrentControl())
  {
    case GUIDE_VIEW_TIMELINE:
      GetViewTimelineItems(items);
      break;
    case GUIDE_VIEW_NOW:
      GetViewNowItems(items);
      break;
    case GUIDE_VIEW_NEXT:
      GetViewNextItems(items);
      break;
    case GUIDE_VIEW_CHANNEL:
      GetViewChannelItems(items);
      break;
    default:
      CLog::Log(LOGERROR, "CGUIWindowPVRGuide - %s - Unknown view control. Unable to fill item list.", __FUNCTION__);
      break;
  }

  m_bUpdateRequired = false;
  return true;
}

bool CGUIWindowPVRGuide::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
      if (m_viewControl.GetCurrentControl() != GUIDE_VIEW_CHANNEL)
        return ActionInputChannelNumber(action.GetID() - REMOTE_0);
      break;
  }

  return CGUIWindowPVRBase::OnAction(action);
}

bool CGUIWindowPVRGuide::OnMessage(CGUIMessage& message)
{
  if (!IsValidMessage(message))
    return false;
  
  bool bReturn = false;
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == m_viewControl.GetCurrentControl())
      {
        int iItem = m_viewControl.GetSelectedItem();
        if (iItem >= 0 && iItem < m_vecItems->Size())
        {
          CFileItemPtr pItem = m_vecItems->Get(iItem);
          /* process actions */
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
              switch(CSettings::Get().GetInt("epg.selectaction"))
              {
                case EPG_SELECT_ACTION_CONTEXT_MENU:
                  OnPopupMenu(iItem);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_SWITCH:
                  ActionPlayEpg(pItem.get(), false);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_PLAY_RECORDING:
                  ActionPlayEpg(pItem.get(), true);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_INFO:
                  ShowEPGInfo(pItem.get());
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_RECORD:
                  ActionRecord(pItem.get());
                  bReturn = true;
                  break;
              }
              break;
            case ACTION_SHOW_INFO:
              ShowEPGInfo(pItem.get());
              bReturn = true;
              break;
            case ACTION_PLAY:
              ActionPlayEpg(pItem.get(), true);
              bReturn = true;
              break;
            case ACTION_RECORD:
              ActionRecord(pItem.get());
              bReturn = true;
              break;
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              bReturn = true;
              break;
          }
        }
      }
      else if (message.GetSenderId() == CONTROL_BTNVIEWASICONS)
      {
        // let's set the view mode first before update
        CGUIWindowPVRBase::OnMessage(message);
        Refresh(true);
        bReturn = true;
      }
      break;
    }
    case GUI_MSG_CHANGE_VIEW_MODE:
    {
      // let's set the view mode first before update
      CGUIWindowPVRBase::OnMessage(message);
      m_nextUpdateTimeout.SetExpired();
      Refresh(true);
      bReturn = true;
      break;
    }
    case GUI_MSG_REFRESH_LIST:
      switch(message.GetParam1())
      {
        case ObservableMessageChannelGroup:
        case ObservableMessageEpg:
        case ObservableMessageEpgContainer:
        {
          m_bUpdateRequired = true;
          // do not allow more than MAX_UPDATE_FREQUENCY updates
          if (IsActive() && m_nextUpdateTimeout.IsTimePast())
          {
            Refresh(true);
            m_nextUpdateTimeout.Set(MAX_UPDATE_FREQUENCY);
          }
          bReturn = true;
          break;
        }
        case ObservableMessageEpgActiveItem:
        {
          if (IsActive() && m_viewControl.GetCurrentControl() != GUIDE_VIEW_TIMELINE)
            SetInvalid();
          else
            m_bUpdateRequired = true;
          bReturn = true;
          break;
        }
      }
      break;
  }

  return bReturn || CGUIWindowPVRBase::OnMessage(message);
}

bool CGUIWindowPVRGuide::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  return OnContextButtonPlay(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonStartRecord(pItem.get(), button) ||
      OnContextButtonStopRecord(pItem.get(), button) ||
      OnContextButtonBegin(pItem.get(), button) ||
      OnContextButtonEnd(pItem.get(), button) ||
      OnContextButtonNow(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRGuide::GetViewChannelItems(CFileItemList &items)
{
  CPVRChannelPtr currentChannel(g_PVRManager.GetCurrentChannel());

  if (currentChannel)
    SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, currentChannel->ChannelName().c_str());
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, GetGroup()->GroupName());

  items.Clear();
  if (!currentChannel || g_PVRManager.GetCurrentEpg(items) == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/channel/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    items.Add(item);
  }
}

void CGUIWindowPVRGuide::GetViewNowItems(CFileItemList &items)
{
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, g_localizeStrings.Get(19030));
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, GetGroup()->GroupName());

  items.Clear();
  int iEpgItems = GetGroup()->GetEPGNow(items);

  if (iEpgItems == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/now/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    items.Add(item);
  }
}

void CGUIWindowPVRGuide::GetViewNextItems(CFileItemList &items)
{
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, g_localizeStrings.Get(19031));
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, GetGroup()->GroupName());

  items.Clear();
  int iEpgItems = GetGroup()->GetEPGNext(items);

  if (iEpgItems)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/next/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    items.Add(item);
  }
}

void CGUIWindowPVRGuide::GetViewTimelineItems(CFileItemList &items)
{
  CGUIEPGGridContainer* epgGridContainer = (CGUIEPGGridContainer*) GetControl(m_viewControl.GetCurrentControl());
  if (!epgGridContainer)
    return;

  CPVRChannelGroupPtr group = GetGroup();

  if (m_bUpdateRequired || m_cachedTimeline->IsEmpty() || *m_cachedChannelGroup != *group)
  {
    m_bUpdateRequired = false;

    m_cachedTimeline->Clear();
    m_cachedChannelGroup = group;
    m_cachedChannelGroup->GetEPGAll(*m_cachedTimeline);
  }

  items.Clear();
  items.RemoveDiscCache(GetID());
  items.Assign(*m_cachedTimeline, false);

  CDateTime startDate(m_cachedChannelGroup->GetFirstEPGDate());
  CDateTime endDate(m_cachedChannelGroup->GetLastEPGDate());
  CDateTime currentDate = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();

  if (!startDate.IsValid())
    startDate = currentDate;

  if (!endDate.IsValid() || endDate < startDate)
    endDate = startDate;

  // limit start to linger time
  CDateTime maxPastDate = currentDate - CDateTimeSpan(0, 0, g_advancedSettings.m_iEpgLingerTime, 0);
  if (startDate < maxPastDate)
    startDate = maxPastDate;

  epgGridContainer->SetStartEnd(startDate, endDate);

  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, g_localizeStrings.Get(19032));
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, GetGroup()->GroupName());

  epgGridContainer->SetChannel(GetSelectedItemPath(m_bRadio));
}

bool CGUIWindowPVRGuide::OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_BEGIN)
  {
    CGUIEPGGridContainer* epgGridContainer = (CGUIEPGGridContainer*) GetControl(m_viewControl.GetCurrentControl());
    epgGridContainer->GoToBegin();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonEnd(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_END)
  {
    CGUIEPGGridContainer* epgGridContainer = (CGUIEPGGridContainer*) GetControl(m_viewControl.GetCurrentControl());
    epgGridContainer->GoToEnd();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonNow(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_NOW)
  {
    CGUIEPGGridContainer* epgGridContainer = (CGUIEPGGridContainer*) GetControl(m_viewControl.GetCurrentControl());
    epgGridContainer->GoToNow();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    ShowEPGInfo(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_PLAY_ITEM || button == CONTEXT_BUTTON_PLAY_OTHER)
  {
    ActionPlayEpg(item, button == CONTEXT_BUTTON_PLAY_OTHER);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_START_RECORD)
  {
    StartRecordFile(*item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    StopRecordFile(*item);
    bReturn = true;
  }

  return bReturn;
}
