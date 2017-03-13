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
#include "ServiceBroker.h"
#include "epg/EpgContainer.h"
#include "view/GUIViewState.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimers.h"

#include "GUIWindowPVRGuide.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRGuide::CGUIWindowPVRGuide(bool bRadio) :
  CGUIWindowPVRBase(bRadio, bRadio ? WINDOW_RADIO_GUIDE : WINDOW_TV_GUIDE, "MyPVRGuide.xml"),
  CPVRChannelNumberInputHandler(1000),
  m_cachedChannelGroup(new CPVRChannelGroup),
  m_bChannelSelectionRestored(false)
{
  m_bRefreshTimelineItems = false;
  g_EpgContainer.RegisterObserver(this);
}

CGUIWindowPVRGuide::~CGUIWindowPVRGuide(void)
{
  g_EpgContainer.UnregisterObserver(this);
  StopRefreshTimelineItemsThread();
}

CGUIEPGGridContainer* CGUIWindowPVRGuide::GetGridControl()
{
  return dynamic_cast<CGUIEPGGridContainer*>(GetControl(m_viewControl.GetCurrentControl()));
}

void CGUIWindowPVRGuide::Init()
{
  CGUIEPGGridContainer *epgGridContainer = GetGridControl();
  if (epgGridContainer)
  {
    m_bChannelSelectionRestored = epgGridContainer->SetChannel(GetSelectedItemPath(m_bRadio));
    epgGridContainer->GoToNow();
  }

  m_bRefreshTimelineItems = true;
  StartRefreshTimelineItemsThread();
}

void CGUIWindowPVRGuide::ClearData()
{
  {
    CSingleLock lock(m_critSection);
    m_cachedChannelGroup.reset(new CPVRChannelGroup);
    m_newTimeline.reset();
  }

  CGUIWindowPVRBase::ClearData();
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

  m_bChannelSelectionRestored = false;

  CGUIWindowPVRBase::OnDeinitWindow(nextWindowID);
}

void CGUIWindowPVRGuide::StartRefreshTimelineItemsThread()
{
  StopRefreshTimelineItemsThread();
  m_refreshTimelineItemsThread.reset(new CPVRRefreshTimelineItemsThread(this));
  m_refreshTimelineItemsThread->Create();
}

void CGUIWindowPVRGuide::StopRefreshTimelineItemsThread()
{
  if (m_refreshTimelineItemsThread)
    m_refreshTimelineItemsThread->StopThread(false);
}

void CGUIWindowPVRGuide::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (IsActive() &&
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

  buttons.Add(CONTEXT_BUTTON_BEGIN, 19063); /* Go to begin */
  buttons.Add(CONTEXT_BUTTON_NOW,   19070); /* Go to now */
  buttons.Add(CONTEXT_BUTTON_END,   19064); /* Go to end */

  CGUIWindowPVRBase::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowPVRGuide::UpdateSelectedItemPath()
{
  CGUIEPGGridContainer *epgGridContainer = GetGridControl();
  if (epgGridContainer)
  {
    CPVRChannelPtr channel(epgGridContainer->GetSelectedChannel());
    if (channel)
      SetSelectedItemPath(m_bRadio, channel->Path());
  }
}

void CGUIWindowPVRGuide::UpdateButtons(void)
{
  CGUIWindowPVRBase::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER1, g_localizeStrings.Get(19032));
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER2, GetChannelGroup()->GroupName());
}

bool CGUIWindowPVRGuide::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  bool bReturn = CGUIWindowPVRBase::Update(strDirectory, updateFilterPath);

  if (bReturn && !m_bChannelSelectionRestored)
  {
    CGUIEPGGridContainer* epgGridContainer = GetGridControl();
    if (epgGridContainer)
      m_bChannelSelectionRestored = epgGridContainer->SetChannel(GetSelectedItemPath(m_bRadio));
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  bool bRefresh = false;

  {
    CSingleLock lock(m_critSection);

    // group change detected reset grid coordinates and refresh grid items
    if (!m_bRefreshTimelineItems && *m_cachedChannelGroup != *GetChannelGroup())
    {
      CGUIEPGGridContainer* epgGridContainer = GetGridControl();
      if (!epgGridContainer)
        return true;

      epgGridContainer->ResetCoordinates();
      m_bRefreshTimelineItems = true;
      bRefresh = true;
    }
  }

  // never call RefreshTimelineItems with locked mutex!
  if (bRefresh)
    RefreshTimelineItems();

  {
    CSingleLock lock(m_critSection);

    // Note: no need to do anything if no new data available. items always contains previous data.
    if (m_newTimeline)
    {
      items.RemoveDiscCache(GetID());
      items.Assign(*m_newTimeline, false);
      m_newTimeline.reset();
    }
  }

  return true;
}

bool CGUIWindowPVRGuide::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case REMOTE_0:
      if (GetCurrentDigitCount() == 0)
      {
        // single zero input is handled by epg grid container
        break;
      }
      // fall-thru is intended
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
              switch(CServiceBroker::GetSettings().GetInt(CSettings::SETTING_EPG_SELECTACTION))
              {
                case EPG_SELECT_ACTION_CONTEXT_MENU:
                  OnPopupMenu(iItem);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_SWITCH:
                  CPVRGUIActions::GetInstance().SwitchToChannel(pItem, true);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_PLAY_RECORDING:
                  CPVRGUIActions::GetInstance().PlayRecording(pItem, true);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_INFO:
                  CPVRGUIActions::GetInstance().ShowEPGInfo(pItem);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_RECORD:
                  CPVRGUIActions::GetInstance().ToggleTimer(pItem);
                  bReturn = true;
                  break;
                case EPG_SELECT_ACTION_SMART_SELECT:
                {
                  const CEpgInfoTagPtr tag(pItem->GetEPGInfoTag());
                  if (tag)
                  {
                    const CDateTime start(tag->StartAsUTC());
                    const CDateTime end(tag->EndAsUTC());
                    const CDateTime now(CDateTime::GetUTCDateTime());

                    if (start <= now && now <= end)
                    {
                      // current event
                      CPVRGUIActions::GetInstance().SwitchToChannel(pItem, true);
                    }
                    else if (now < start)
                    {
                      // future event
                      if (tag->HasTimer())
                        CPVRGUIActions::GetInstance().EditTimer(pItem);
                      else
                        CPVRGUIActions::GetInstance().AddTimer(pItem, false);
                    }
                    else
                    {
                      // past event
                      if (tag->HasRecording())
                        CPVRGUIActions::GetInstance().PlayRecording(pItem, true);
                      else
                        CPVRGUIActions::GetInstance().ShowEPGInfo(pItem);
                    }
                    bReturn = true;
                  }
                  break;
                }
              }
              break;
            case ACTION_SHOW_INFO:
              CPVRGUIActions::GetInstance().ShowEPGInfo(pItem);
              bReturn = true;
              break;
            case ACTION_PLAY:
              CPVRGUIActions::GetInstance().PlayRecording(pItem, true);
              bReturn = true;
              break;
            case ACTION_RECORD:
              CPVRGUIActions::GetInstance().ToggleTimer(pItem);
              bReturn = true;
              break;
            case ACTION_PVR_SHOW_TIMER_RULE:
              CPVRGUIActions::GetInstance().AddTimerRule(pItem, true);
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
                const CFileItemPtr item(epgGridContainer->GetSelectedChannelItem());
                if (item)
                {
                  CPVRGUIActions::GetInstance().SwitchToChannel(item, true);
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

  return OnContextButtonBegin(pItem.get(), button) ||
      OnContextButtonEnd(pItem.get(), button) ||
      OnContextButtonNow(pItem.get(), button) ||
      CGUIMediaWindow::OnContextButton(itemNumber, button);
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
      if (!group)
        return false;

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

bool CGUIWindowPVRGuide::OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_BEGIN)
  {
    CGUIEPGGridContainer* epgGridContainer = GetGridControl();
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
    CGUIEPGGridContainer* epgGridContainer = GetGridControl();
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
    CGUIEPGGridContainer* epgGridContainer = GetGridControl();
    epgGridContainer->GoToNow();
    bReturn = true;
  }

  return bReturn;
}

void CGUIWindowPVRGuide::OnInputDone()
{
  const int iChannelNumber = GetChannelNumber();
  if (iChannelNumber >= 0)
  {
    for (const CFileItemPtr event : m_vecItems->GetList())
    {
      const CEpgInfoTagPtr tag(event->GetEPGInfoTag());
      if (tag->HasPVRChannel() && tag->PVRChannelNumber() == iChannelNumber)
      {
        CGUIEPGGridContainer* epgGridContainer = GetGridControl();
        if (epgGridContainer)
        {
          epgGridContainer->SetChannel(tag->ChannelTag());
          return;
        }
      }
    }
  }
}

CPVRRefreshTimelineItemsThread::CPVRRefreshTimelineItemsThread(CGUIWindowPVRGuide *pGuideWindow)
: CThread("epg-grid-refresh-timeline-items"),
  m_pGuideWindow(pGuideWindow)
{
}

void CPVRRefreshTimelineItemsThread::Process()
{
  static const int BOOSTED_SLEEPS_THRESHOLD = 4;

  int iLastEpgItemsCount = 0;
  int iUpdatesWithoutChange = 0;

  while (!m_bStop)
  {
    if (m_pGuideWindow->RefreshTimelineItems() && !m_bStop)
    {
      CGUIMessage m(GUI_MSG_REFRESH_LIST, m_pGuideWindow->GetID(), 0, ObservableMessageEpg);
      KODI::MESSAGING::CApplicationMessenger::GetInstance().SendGUIMessage(m);
    }

    // in order to fill the guide window asap, use a short update interval until we the
    // same amount of epg events for BOOSTED_SLEEPS_THRESHOLD + 1 times in a row .
    if (iUpdatesWithoutChange < BOOSTED_SLEEPS_THRESHOLD)
    {
      int iCurrentEpgItemsCount = m_pGuideWindow->CurrentDirectory().Size();

      if (iCurrentEpgItemsCount == iLastEpgItemsCount)
        iUpdatesWithoutChange++;
      else
        iUpdatesWithoutChange = 0; // reset

      iLastEpgItemsCount = iCurrentEpgItemsCount;

      Sleep(1000); // boosted update cycle
    }
    else
    {
      Sleep(5000); // normal update cycle
    }
  }
}
