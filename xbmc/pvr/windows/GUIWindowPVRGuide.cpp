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

#include "ContextMenuManager.h"
#include "epg/GUIEPGGridContainer.h"
#include "GUIUserMessages.h"
#include "epg/EpgContainer.h"
#include "view/GUIViewState.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimers.h"

#include "GUIWindowPVRGuide.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRGuide::CGUIWindowPVRGuide(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_GUIDE : WINDOW_TV_GUIDE, "MyPVRGuide.xml"),
  m_refreshTimelineItemsThread(new CPVRRefreshTimelineItemsThread(this)),
  m_cachedChannelGroup(new CPVRChannelGroup)
{
  m_bRefreshTimelineItems = false;
}

CGUIWindowPVRGuide::~CGUIWindowPVRGuide(void)
{
  StopRefreshTimelineItemsThread();
}

CGUIEPGGridContainer* CGUIWindowPVRGuide::GetGridControl()
{
  return dynamic_cast<CGUIEPGGridContainer*>(GetControl(m_viewControl.GetCurrentControl()));
}

bool CGUIWindowPVRGuide::CanBeActivated() const
{
  return true;
}

void CGUIWindowPVRGuide::Init()
{
  CGUIEPGGridContainer *epgGridContainer = GetGridControl();
  if (epgGridContainer)
  {
    epgGridContainer->SetChannel(GetSelectedItemPath(m_bRadio));
    epgGridContainer->GoToNow();
  }

  m_bRefreshTimelineItems = true;
  StartRefreshTimelineItemsThread();
}

void CGUIWindowPVRGuide::OnInitWindow()
{
  if (m_guiState.get())
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl(), false);

  if (InitChannelGroup()) // no channels -> lazy init
    Init();

  CGUIWindowPVRBase::OnInitWindow();
}

void CGUIWindowPVRGuide::OnDeinitWindow(int nextWindowID)
{
  StopRefreshTimelineItemsThread();
  m_bRefreshTimelineItems = false;

  CGUIWindowPVRBase::OnDeinitWindow(nextWindowID);
}

void CGUIWindowPVRGuide::StartRefreshTimelineItemsThread()
{
  StopRefreshTimelineItemsThread();
  m_refreshTimelineItemsThread->Create();
}

void CGUIWindowPVRGuide::StopRefreshTimelineItemsThread()
{
  m_refreshTimelineItemsThread->StopThread(false);
}

void CGUIWindowPVRGuide::RegisterObservers(void)
{
  CSingleLock lock(m_critSection);
  g_EpgContainer.RegisterObserver(this);
  g_PVRManager.RegisterObserver(this);
  CGUIWindowPVRBase::RegisterObservers();
}

void CGUIWindowPVRGuide::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  CGUIWindowPVRBase::UnregisterObservers();
  g_PVRManager.UnregisterObserver(this);
  g_EpgContainer.UnregisterObserver(this);
}

void CGUIWindowPVRGuide::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (m_viewControl.GetCurrentControl() == GUIDE_VIEW_TIMELINE &&
      (msg == ObservableMessageEpg ||
       msg == ObservableMessageEpgContainer ||
       msg == ObservableMessageChannelGroupReset ||
       msg == ObservableMessageChannelGroup))
  {
    CSingleLock lock(m_critSection);
    m_bRefreshTimelineItems = true;
  }
  else
  {
    CGUIWindowPVRBase::Notify(obs, msg);
  }
}

void CGUIWindowPVRGuide::SetInvalid()
{
  CGUIEPGGridContainer *epgGridContainer = GetGridControl();
  if (epgGridContainer)
    epgGridContainer->SetInvalid();

  CGUIWindowPVRBase::SetInvalid();
}

void CGUIWindowPVRGuide::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);         /* Switch channel */
  buttons.Add(CONTEXT_BUTTON_INFO, 19047);              /* Programme information */
  buttons.Add(CONTEXT_BUTTON_FIND, 19003);              /* Find similar */

  CEpgInfoTagPtr epg(pItem->GetEPGInfoTag());
  if (epg)
  {
    CPVRTimerInfoTagPtr timer(epg->Timer());
    if (timer)
    {
      if (timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT)
      {
        buttons.Add(CONTEXT_BUTTON_EDIT_TIMER_RULE, 19243); /* Edit timer rule */
        buttons.Add(CONTEXT_BUTTON_DELETE_TIMER_RULE, 19295); /* Delete timer rule */
      }

      const CPVRTimerTypePtr timerType(timer->GetTimerType());
      if (timerType && !timerType->IsReadOnly() && timer->GetTimerRuleId() == PVR_TIMER_NO_PARENT)
        buttons.Add(CONTEXT_BUTTON_EDIT_TIMER, 19242);    /* Edit timer */
      else
        buttons.Add(CONTEXT_BUTTON_EDIT_TIMER, 19241);    /* View timer information */

      if (timer->IsRecording())
        buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059);   /* Stop recording */
      else
      {
        if (timerType && !timerType->IsReadOnly())
          buttons.Add(CONTEXT_BUTTON_DELETE_TIMER, 19060);  /* Delete timer */
      }
    }
    else if (g_PVRClients->SupportsTimers())
    {
      if (epg->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
        buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);      /* Record */
      buttons.Add(CONTEXT_BUTTON_ADD_TIMER, 19061);       /* Add timer */
    }

    if (epg->HasRecording())
      buttons.Add(CONTEXT_BUTTON_PLAY_OTHER, 19687);      /* Play recording */
  }

  if (m_viewControl.GetCurrentControl() == GUIDE_VIEW_TIMELINE)
  {
    buttons.Add(CONTEXT_BUTTON_BEGIN, 19063);           /* Go to begin */
    buttons.Add(CONTEXT_BUTTON_NOW, 19070);             /* Go to now */
    buttons.Add(CONTEXT_BUTTON_END, 19064);             /* Go to end */
  }

  if (epg)
  {
    CPVRChannelPtr channel(epg->ChannelTag());
    if (channel && g_PVRClients->HasMenuHooks(channel->ClientID(), PVR_MENUHOOK_EPG))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
  }

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowPVRGuide::UpdateSelectedItemPath()
{
  if (m_viewControl.GetCurrentControl() == GUIDE_VIEW_TIMELINE)
  {
    CGUIEPGGridContainer *epgGridContainer = (CGUIEPGGridContainer*) GetControl(m_viewControl.GetCurrentControl());
    if (epgGridContainer)
    {
      CPVRChannelPtr channel(epgGridContainer->GetSelectedChannel());
      if (channel)
        SetSelectedItemPath(m_bRadio, channel->Path());
    }
  }
  else
    CGUIWindowPVRBase::UpdateSelectedItemPath();
}

void CGUIWindowPVRGuide::UpdateButtons(void)
{
  CGUIWindowPVRBase::UpdateButtons();
  switch (m_viewControl.GetCurrentControl())
  {
    case GUIDE_VIEW_TIMELINE: {
      SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, g_localizeStrings.Get(19032));
      break;
    }
    case GUIDE_VIEW_NOW: {
      SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, g_localizeStrings.Get(19030));
      break;
    }
    case GUIDE_VIEW_NEXT: {
      SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, g_localizeStrings.Get(19031));
      break;
    }
    case GUIDE_VIEW_CHANNEL: {
      CPVRChannelPtr currentChannel(g_PVRManager.GetCurrentChannel());
      if (currentChannel)
        SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, currentChannel->ChannelName().c_str());
      break;
    }
    default:
      break;
  }

  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, GetChannelGroup()->GroupName());
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
              switch(CSettings::GetInstance().GetInt(CSettings::SETTING_EPG_SELECTACTION))
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
                  ActionToggleTimer(pItem.get());
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
              ActionToggleTimer(pItem.get());
              bReturn = true;
              break;
            case ACTION_PVR_SHOW_TIMER_RULE:
              ActionShowTimerRule(pItem.get());
              bReturn = true;
              break;
            case ACTION_CONTEXT_MENU:
            case ACTION_MOUSE_RIGHT_CLICK:
              OnPopupMenu(iItem);
              bReturn = true;
              break;
          }
        }
        else if (iItem == -1)
        {
          /* process actions */
          switch (message.GetParam1())
          {
            case ACTION_SELECT_ITEM:
            case ACTION_MOUSE_LEFT_CLICK:
            case ACTION_PLAY:
            {
              // EPG "gap" selected => switch to associated channel.
              CGUIEPGGridContainer *epgGridContainer = GetGridControl();
              if (epgGridContainer)
              {
                CFileItemPtr item(epgGridContainer->GetSelectedChannelItem());
                if (item)
                {
                  ActionPlayEpg(item.get(), false);
                  bReturn = true;
                }
              }
              break;
            }
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
      Refresh(true);
      bReturn = true;
      break;
    }
    case GUI_MSG_REFRESH_LIST:
      switch(message.GetParam1())
      {
        case ObservableMessageChannelGroupsLoaded:
        {
          // late init
          InitChannelGroup();
          Init();
          break;
        }
        case ObservableMessageChannelGroupReset:
        case ObservableMessageChannelGroup:
        case ObservableMessageEpg:
        case ObservableMessageEpgContainer:
        {
          Refresh(true);
          break;
        }
        case ObservableMessageTimersReset:
        case ObservableMessageTimers:
        {
          SetInvalid();
          break;
        }
        case ObservableMessageEpgActiveItem:
        {
          if (m_viewControl.GetCurrentControl() != GUIDE_VIEW_TIMELINE)
            SetInvalid();

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
      OnContextButtonEditTimer(pItem.get(), button) ||
      OnContextButtonEditTimerRule(pItem.get(), button) ||
      OnContextButtonDeleteTimer(pItem.get(), button) ||
      OnContextButtonDeleteTimerRule(pItem.get(), button) ||
      OnContextButtonBegin(pItem.get(), button) ||
      OnContextButtonEnd(pItem.get(), button) ||
      OnContextButtonNow(pItem.get(), button) ||
      CGUIWindowPVRBase::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRGuide::GetViewChannelItems(CFileItemList &items)
{
  CPVRChannelPtr currentChannel(g_PVRManager.GetCurrentChannel());
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
  items.Clear();
  int iEpgItems = GetChannelGroup()->GetEPGNow(items);

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
  items.Clear();
  int iEpgItems = GetChannelGroup()->GetEPGNext(items);

  if (iEpgItems)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/next/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    items.Add(item);
  }
}

bool CGUIWindowPVRGuide::RefreshTimelineItems()
{
  if (m_bRefreshTimelineItems)
  {
    m_bRefreshTimelineItems = false;

    CGUIEPGGridContainer* epgGridContainer = GetGridControl();
    if (epgGridContainer)
    {
      const CPVRChannelGroupPtr group(GetChannelGroup());
      std::unique_ptr<CFileItemList> timeline(new CFileItemList);

      // can be very expensive. never call with lock acquired.
      group->GetEPGAll(*timeline, true);

      CDateTime startDate(group->GetFirstEPGDate());
      CDateTime endDate(group->GetLastEPGDate());
      const CDateTime currentDate(CDateTime::GetCurrentDateTime().GetAsUTCDateTime());

      if (!startDate.IsValid())
        startDate = currentDate;

      if (!endDate.IsValid() || endDate < startDate)
        endDate = startDate;

      // limit start to linger time
      const CDateTime maxPastDate(currentDate - CDateTimeSpan(0, 0, g_advancedSettings.m_iEpgLingerTime, 0));
      if (startDate < maxPastDate)
        startDate = maxPastDate;

      // can be very expensive. never call with lock acquired.
      epgGridContainer->SetTimelineItems(timeline, startDate, endDate);

      {
        CSingleLock lock(m_critSection);

        m_newTimeline = std::move(timeline);
        m_cachedChannelGroup = group;
      }
      return true;
    }
  }
  return false;
}

void CGUIWindowPVRGuide::GetViewTimelineItems(CFileItemList &items)
{
  CSingleLock lock(m_critSection);

  // group change detected reset grid coordinates and refresh grid items
  if (!m_bRefreshTimelineItems && *m_cachedChannelGroup != *GetChannelGroup())
  {
    CGUIEPGGridContainer* epgGridContainer = GetGridControl();
    if (!epgGridContainer)
      return;

    epgGridContainer->ResetCoordinates();
    m_bRefreshTimelineItems = true;
    RefreshTimelineItems();
  }

  // Note: no need to do anything if no new data available. items always contains previous data.
  if (m_newTimeline)
  {
    items.RemoveDiscCache(GetID());
    items.Assign(*m_newTimeline, false);
    m_newTimeline.reset();
  }
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
    AddTimer(item);
    bReturn = true;
  }
  else if (button == CONTEXT_BUTTON_ADD_TIMER)
  {
    AddTimerRule(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonStopRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    StopRecordFile(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonDeleteTimer(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_DELETE_TIMER)
  {
    DeleteTimer(item);
    bReturn = true;
  }

  return bReturn;
}

CPVRRefreshTimelineItemsThread::CPVRRefreshTimelineItemsThread(CGUIWindowPVRGuide *pGuideWindow)
: CThread("epg-grid-refresh-timeline-items"),
  m_pGuideWindow(pGuideWindow)
{
}

void CPVRRefreshTimelineItemsThread::Process()
{
  while (!m_bStop)
  {
    if (m_pGuideWindow->RefreshTimelineItems() && !m_bStop)
    {
      CGUIMessage m(GUI_MSG_REFRESH_LIST, m_pGuideWindow->GetID(), 0, ObservableMessageEpg);
      KODI::MESSAGING::CApplicationMessenger::GetInstance().SendGUIMessage(m);
    }
    Sleep(5000);
  }
}
