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

#include "Application.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "epg/EpgContainer.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRGuide::CGUIWindowPVRGuide(CGUIWindowPVR *parent) :
  CGUIWindowPVRCommon(parent, PVR_WINDOW_EPG, CONTROL_BTNGUIDE, CONTROL_LIST_GUIDE_NOW_NEXT),
  Observer(),
  m_iGuideView(CSettings::Get().GetInt("epg.defaultguideview"))
{
  m_cachedTimeline = new CFileItemList;
  m_cachedChannelGroup = CPVRChannelGroupPtr(new CPVRChannelGroup);
}

CGUIWindowPVRGuide::~CGUIWindowPVRGuide(void)
{
  delete m_cachedTimeline;
}

void CGUIWindowPVRGuide::UnregisterObservers(void)
{
  g_EpgContainer.UnregisterObserver(this);
}

void CGUIWindowPVRGuide::ResetObservers(void)
{
  g_EpgContainer.RegisterObserver(this);
}

void CGUIWindowPVRGuide::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageEpg || msg == ObservableMessageEpgContainer)
  {
    m_bUpdateRequired = true;

    /* update the current window if the EPG timeline view is visible */
    if (IsFocused() && m_iGuideView == GUIDE_VIEW_TIMELINE)
      UpdateData(false);
  }
  else if (msg == ObservableMessageEpgActiveItem)
  {
    if (IsVisible() && m_iGuideView != GUIDE_VIEW_TIMELINE)
      SetInvalid();
    else
      m_bUpdateRequired = true;
  }
}

void CGUIWindowPVRGuide::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);         /* switch channel */

  CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(pItem.get());
  if (timer && timer->HasPVRTimerInfoTag())
  {
    if (timer->GetPVRTimerInfoTag()->IsRecording())
      buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059);  /* stop recording */
    else
      buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19060);  /* delete timer */
  }
  else if (pItem->GetEPGInfoTag()->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
  {
    if (pItem->GetEPGInfoTag()->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
      buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);   /* record */
    else
      buttons.Add(CONTEXT_BUTTON_START_RECORD, 19061); /* add timer */
  }

  buttons.Add(CONTEXT_BUTTON_INFO, 19047);              /* epg info */
  buttons.Add(CONTEXT_BUTTON_FIND, 19003);              /* find similar program */

  if (m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    buttons.Add(CONTEXT_BUTTON_BEGIN, 19063);           /* go to begin */
    buttons.Add(CONTEXT_BUTTON_NOW, 19070);             /* go to now */
    buttons.Add(CONTEXT_BUTTON_END, 19064);             /* go to end */
  }

  if (pItem->GetEPGInfoTag()->HasPVRChannel() &&
      g_PVRClients->HasMenuHooks(pItem->GetEPGInfoTag()->ChannelTag()->ClientID(), PVR_MENUHOOK_EPG))
    buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */
}


bool CGUIWindowPVRGuide::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= (int) m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  return OnContextButtonPlay(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonStartRecord(pItem.get(), button) ||
      OnContextButtonStopRecord(pItem.get(), button) ||
      OnContextButtonBegin(pItem.get(), button) ||
      OnContextButtonEnd(pItem.get(), button) ||
      OnContextButtonNow(pItem.get(), button) ||
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRGuide::UpdateViewChannel(bool bUpdateSelectedFile)
{
  CPVRChannelPtr CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(CurrentChannel);

  m_parent->m_guideGrid = NULL;
  m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_CHANNEL);

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19029));
  if (bGotCurrentChannel && CurrentChannel.get())
    m_parent->SetLabel(CONTROL_LABELGROUP, CurrentChannel->ChannelName().c_str());

  if ((!bGotCurrentChannel || g_PVRManager.GetCurrentEpg(*m_parent->m_vecItems) == 0) && CurrentChannel.get())
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/" + CurrentChannel->ChannelName() + "/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    m_parent->m_vecItems->Add(item);
  }
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
}

void CGUIWindowPVRGuide::UpdateViewNow(bool bUpdateSelectedFile)
{
  CPVRChannelPtr CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(CurrentChannel);
  bool bRadio = bGotCurrentChannel ? CurrentChannel->IsRadio() : false;

  m_parent->m_guideGrid = NULL;
  m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_NOW_NEXT);

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19030));
  m_parent->SetLabel(CONTROL_LABELGROUP, g_localizeStrings.Get(19030));

  int iEpgItems = g_PVRManager.GetPlayingGroup(bRadio)->GetEPGNow(*m_parent->m_vecItems);
  if (iEpgItems == 0 && bRadio)
    // if we didn't get any events for radio, get tv instead
    iEpgItems = g_PVRManager.GetPlayingGroup(false)->GetEPGNow(*m_parent->m_vecItems);

  if (iEpgItems == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/now/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    m_parent->m_vecItems->Add(item);
  }
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
}

void CGUIWindowPVRGuide::UpdateViewNext(bool bUpdateSelectedFile)
{
  CPVRChannelPtr CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(CurrentChannel);
  bool bRadio = bGotCurrentChannel ? CurrentChannel->IsRadio() : false;

  m_parent->m_guideGrid = NULL;
  m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_NOW_NEXT);

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19031));
  m_parent->SetLabel(CONTROL_LABELGROUP, g_localizeStrings.Get(19031));

  int iEpgItems = g_PVRManager.GetPlayingGroup(bRadio)->GetEPGNext(*m_parent->m_vecItems);
  if (iEpgItems == 0 && bRadio)
    // if we didn't get any events for radio, get tv instead
    iEpgItems = g_PVRManager.GetPlayingGroup(false)->GetEPGNext(*m_parent->m_vecItems);

  if (iEpgItems)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/next/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    m_parent->m_vecItems->Add(item);
  }
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
}

void CGUIWindowPVRGuide::UpdateViewTimeline(bool bUpdateSelectedFile)
{
  m_parent->m_guideGrid = (CGUIEPGGridContainer*) m_parent->GetControl(CONTROL_LIST_TIMELINE);
  if (!m_parent->m_guideGrid)
    return;

  CPVRChannelPtr CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(CurrentChannel);
  bool bRadio = bGotCurrentChannel ? CurrentChannel->IsRadio() : false;

  if (m_bUpdateRequired || m_cachedTimeline->IsEmpty() ||
      *m_cachedChannelGroup != *g_PVRManager.GetPlayingGroup(bRadio))
  {
    m_bUpdateRequired = false;

    m_cachedTimeline->Clear();
    m_cachedChannelGroup = g_PVRManager.GetPlayingGroup(bRadio);
    
    if (m_cachedChannelGroup->GetEPGAll(*m_cachedTimeline) == 0 && bRadio)
    {
      // if we didn't get any events for radio, get tv instead
      m_cachedChannelGroup = g_PVRManager.GetPlayingGroup(false);
      m_cachedChannelGroup->GetEPGAll(*m_cachedTimeline);
    }
  }

  m_parent->m_vecItems->RemoveDiscCache(m_parent->GetID());
  m_parent->m_vecItems->Assign(*m_cachedTimeline, false);

  CDateTime startDate(m_cachedChannelGroup->GetFirstEPGDate());
  CDateTime endDate(m_cachedChannelGroup->GetLastEPGDate());
  CDateTime currentDate = CDateTime::GetCurrentDateTime().GetAsUTCDateTime();
  
  if (!startDate.IsValid())
    startDate = currentDate;
  
  if (!endDate.IsValid() || endDate < startDate)
    endDate = startDate;
  
  // limit start to linger time
  CDateTime maxPastDate = currentDate - CDateTimeSpan(0, 0, g_advancedSettings.m_iEpgLingerTime, 0);
  if(startDate < maxPastDate)
    startDate = maxPastDate;
  
  m_parent->m_guideGrid->SetStartEnd(startDate, endDate);

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19032));
  m_parent->SetLabel(CONTROL_LABELGROUP, g_localizeStrings.Get(19032));
  m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_TIMELINE, true);

  if (bUpdateSelectedFile)
    SelectPlayingFile();
}

bool CGUIWindowPVRGuide::SelectPlayingFile(void)
{
  if (m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    if (m_parent->m_guideGrid && g_PVRManager.IsPlaying())
      m_parent->m_guideGrid->SetChannel(g_application.CurrentFile());
    return true;
  }
  return false;
}

void CGUIWindowPVRGuide::UpdateData(bool bUpdateSelectedFile /* = true */)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG, "CGUIWindowPVRGuide - %s - update window '%s'. set view to %d", __FUNCTION__, GetName(), m_iControlList);

  /* lock the graphics context while updating */
  CSingleLock graphicsLock(g_graphicsContext);
  m_parent->m_viewControl.Clear();
  m_parent->m_vecItems->Clear();

  if (m_iGuideView == GUIDE_VIEW_CHANNEL)
    UpdateViewChannel(bUpdateSelectedFile);
  else if (m_iGuideView == GUIDE_VIEW_NOW)
    UpdateViewNow(bUpdateSelectedFile);
  else if (m_iGuideView == GUIDE_VIEW_NEXT)
    UpdateViewNext(bUpdateSelectedFile);
  else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
    UpdateViewTimeline(bUpdateSelectedFile);

  m_bUpdateRequired = false;
  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(19222));
  UpdateButtons();
}

bool CGUIWindowPVRGuide::IsSelectedButton(CGUIMessage &message) const
{
  unsigned int iControl = message.GetSenderId();
  return (iControl == CONTROL_BTNGUIDE ||
      iControl == CONTROL_BTNGUIDE_CHANNEL ||
      iControl == CONTROL_BTNGUIDE_NOW ||
      iControl == CONTROL_BTNGUIDE_NEXT ||
      iControl == CONTROL_BTNGUIDE_TIMELINE);
}

bool CGUIWindowPVRGuide::IsSelectedList(CGUIMessage &message) const
{
  return ((message.GetSenderId() == CONTROL_LIST_TIMELINE && m_iGuideView == GUIDE_VIEW_TIMELINE) ||
      (message.GetSenderId() == CONTROL_LIST_GUIDE_CHANNEL && m_iGuideView == GUIDE_VIEW_CHANNEL) ||
      (message.GetSenderId() == CONTROL_LIST_GUIDE_NOW_NEXT && (m_iGuideView == GUIDE_VIEW_NOW || m_iGuideView == GUIDE_VIEW_NEXT)));
}

bool CGUIWindowPVRGuide::OnClickButton(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedButton(message))
  {
    unsigned int iControl = message.GetSenderId();
    bReturn = true;

    if (iControl == CONTROL_BTNGUIDE)
    {
      if (++m_iGuideView > GUIDE_VIEW_TIMELINE)
        m_iGuideView = GUIDE_VIEW_CHANNEL;
    }
    else if (iControl == CONTROL_BTNGUIDE_CHANNEL)
      m_iGuideView = GUIDE_VIEW_CHANNEL;
    else if (iControl == CONTROL_BTNGUIDE_NOW)
      m_iGuideView = GUIDE_VIEW_NOW;
    else if (iControl == CONTROL_BTNGUIDE_NEXT)
      m_iGuideView = GUIDE_VIEW_NEXT;
    else if (iControl == CONTROL_BTNGUIDE_TIMELINE)
      m_iGuideView = GUIDE_VIEW_TIMELINE;
    else
      bReturn = false;

    if (bReturn)
      UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnClickList(CGUIMessage &message)
{
  if (IsSelectedList(message))
  {
    int iAction = message.GetParam1();
    int iItem = m_parent->m_viewControl.GetSelectedItem();

    if (iItem < 0 || iItem >= (int) m_parent->m_vecItems->Size())
      return false;
    
    /* get the fileitem pointer */
    CFileItemPtr pItem = m_parent->m_vecItems->Get(iItem);

    /* process actions */
    switch (iAction)
    {
      case ACTION_SELECT_ITEM:
      case ACTION_MOUSE_LEFT_CLICK:
        switch(CSettings::Get().GetInt("epg.selectaction"))
        {
          case EPG_SELECT_ACTION_CONTEXT_MENU:
            m_parent->OnPopupMenu(iItem);
            break;
          case EPG_SELECT_ACTION_SWITCH:
            ActionPlayEpg(pItem.get());
            break;
          case EPG_SELECT_ACTION_INFO:
            ShowEPGInfo(pItem.get());
            break;
          case EPG_SELECT_ACTION_RECORD:
            ActionRecord(pItem.get());
            break;
        }
        break;
      case ACTION_SHOW_INFO:
        ShowEPGInfo(pItem.get());
        break;
      case ACTION_PLAY:
        ActionPlayEpg(pItem.get());
        break;
      case ACTION_RECORD:
        ActionRecord(pItem.get());
        break;
      case ACTION_CONTEXT_MENU:
      case ACTION_MOUSE_RIGHT_CLICK:
        m_parent->OnPopupMenu(iItem);
        break;
      default:
        return false;
    }
    
    return true;
  }

  return false;
}

bool CGUIWindowPVRGuide::OnContextButtonBegin(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_BEGIN)
  {
    CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
    if (pWindow)
      pWindow->m_guideGrid->GoToBegin();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonEnd(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_END)
  {
    CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
    if (pWindow)
      pWindow->m_guideGrid->GoToEnd();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonNow(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;
  
  if (button == CONTEXT_BUTTON_NOW)
  {
    CGUIWindowPVR *pWindow = (CGUIWindowPVR *) g_windowManager.GetWindow(WINDOW_PVR);
    if (pWindow)
      pWindow->m_guideGrid->GoToNow();
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

  if (button == CONTEXT_BUTTON_PLAY_ITEM)
  {
    bReturn = ActionPlayEpg(item);
  }

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonStartRecord(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_START_RECORD)
  {
    StartRecordFile(item);
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

void CGUIWindowPVRGuide::UpdateButtons(void)
{
  if (m_iGuideView == GUIDE_VIEW_CHANNEL)
    m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19029));
  else if (m_iGuideView == GUIDE_VIEW_NOW)
    m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19030));
  else if (m_iGuideView == GUIDE_VIEW_NEXT)
    m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19031));
  else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
    m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19032));
}

void CGUIWindowPVRGuide::SettingOptionsEpgGuideViewFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
{
  list.push_back(make_pair(g_localizeStrings.Get(19029), PVR::GUIDE_VIEW_CHANNEL));
  list.push_back(make_pair(g_localizeStrings.Get(19030), PVR::GUIDE_VIEW_NOW));
  list.push_back(make_pair(g_localizeStrings.Get(19031), PVR::GUIDE_VIEW_NEXT));
  list.push_back(make_pair(g_localizeStrings.Get(19032), PVR::GUIDE_VIEW_TIMELINE));
}
