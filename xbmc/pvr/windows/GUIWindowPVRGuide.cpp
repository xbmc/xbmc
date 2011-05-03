/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindowPVRGuide.h"

#include "Application.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/PVREpgContainer.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;
using namespace EPG;

CGUIWindowPVRGuide::CGUIWindowPVRGuide(CGUIWindowPVR *parent) :
  CGUIWindowPVRCommon(parent, PVR_WINDOW_EPG, CONTROL_BTNGUIDE, CONTROL_LIST_GUIDE_NOW_NEXT),
  Observer()
{
  m_iGuideView     = g_guiSettings.GetInt("pvrmenu.defaultguideview");
  m_epgData        = new CFileItemList();
  m_bLastEpgView   = false;
  m_bGotInitialEpg = false;
  m_bObservingEpg  = false;
}

CGUIWindowPVRGuide::~CGUIWindowPVRGuide()
{
  delete m_epgData;
}

void CGUIWindowPVRGuide::Notify(const Observable &obs, const CStdString& msg)
{
  if (msg.Equals("epg") || msg.Equals("timers-reset") || msg.Equals("timers"))
  {
    UpdateEpgCache(m_bLastEpgView, true);

    /* update the current window if the EPG timeline view is active */
    if (IsActive() && m_iGuideView == GUIDE_VIEW_TIMELINE)
      UpdateData();
  }
}

void CGUIWindowPVRGuide::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  if (pItem->GetEPGInfoTag()->EndAsLocalTime() > CDateTime::GetCurrentDateTime())
  {
    CPVRTimerInfoTag *timer = g_PVRTimers->GetMatch(pItem->GetEPGInfoTag());
    if (!timer)
    {
      if (pItem->GetEPGInfoTag()->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
        buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);   /* record program */
      else
        buttons.Add(CONTEXT_BUTTON_START_RECORD, 19061); /* stop recording */
    }
    else
    {
      if (pItem->GetEPGInfoTag()->StartAsLocalTime() < CDateTime::GetCurrentDateTime())
        buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059);
      else
        buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19060);
    }
  }

  buttons.Add(CONTEXT_BUTTON_INFO, 19047);              /* epg info */
  buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);         /* switch channel */
  buttons.Add(CONTEXT_BUTTON_FIND, 19003);              /* find similar program */
  if (m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    buttons.Add(CONTEXT_BUTTON_BEGIN, 19063);           /* go to begin */
    buttons.Add(CONTEXT_BUTTON_END, 19064);             /* go to end */
  }
  if (g_PVRClients->HasMenuHooks(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ClientID()))
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
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRGuide::UpdateViewChannel(void)
{
  CPVRChannel CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(&CurrentChannel);

  m_parent->m_guideGrid = NULL;
  m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_CHANNEL);

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19029));
  if (bGotCurrentChannel)
    m_parent->SetLabel(CONTROL_LABELGROUP, CurrentChannel.ChannelName().c_str());

  if (!bGotCurrentChannel || g_PVRManager.GetCurrentEpg(m_parent->m_vecItems) == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/" + CurrentChannel.ChannelName() + "/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    m_parent->m_vecItems->Add(item);
  }
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
}

void CGUIWindowPVRGuide::UpdateViewNow(void)
{
  CPVRChannel CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(&CurrentChannel);
  bool bRadio = bGotCurrentChannel ? CurrentChannel.IsRadio() : false;

  m_parent->m_guideGrid = NULL;
  m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_NOW_NEXT);

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19030));
  m_parent->SetLabel(CONTROL_LABELGROUP, g_localizeStrings.Get(19030));

  if (g_PVREpg->GetEPGNow(m_parent->m_vecItems, bRadio) == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/now/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    m_parent->m_vecItems->Add(item);
  }
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
}

void CGUIWindowPVRGuide::UpdateViewNext(void)
{
  CPVRChannel CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(&CurrentChannel);
  bool bRadio = bGotCurrentChannel ? CurrentChannel.IsRadio() : false;

  m_parent->m_guideGrid = NULL;
  m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_NOW_NEXT);

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19031));
  m_parent->SetLabel(CONTROL_LABELGROUP, g_localizeStrings.Get(19031));

  if (g_PVREpg->GetEPGNext(m_parent->m_vecItems, bRadio) == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/next/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19028));
    item->SetLabelPreformated(true);
    m_parent->m_vecItems->Add(item);
  }
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);
}

void CGUIWindowPVRGuide::UpdateViewTimeline(void)
{
  CPVRChannel CurrentChannel;
  bool bGotCurrentChannel = g_PVRManager.GetCurrentChannel(&CurrentChannel);
  bool bRadio = bGotCurrentChannel ? CurrentChannel.IsRadio() : false;

  m_parent->SetLabel(m_iControlButton, g_localizeStrings.Get(19222) + ": " + g_localizeStrings.Get(19032));
  m_parent->SetLabel(CONTROL_LABELGROUP, g_localizeStrings.Get(19032));

  CSingleLock lock(m_critSection);

  UpdateEpgCache(bRadio, false);

  if (m_epgData->Size() <= 0)
    return;

  m_parent->m_guideGrid = (CGUIEPGGridContainer*) m_parent->GetControl(CONTROL_LIST_TIMELINE);
  if (m_parent->m_guideGrid)
  {
    CDateTime gridStart = CDateTime::GetCurrentDateTime();
    CDateTime firstDate = g_PVREpg->GetFirstEPGDate(bRadio);
    CDateTime lastDate = g_PVREpg->GetLastEPGDate(bRadio);

    /* copy over the cached epg data */
    for (int iEpgPtr = 0; iEpgPtr < m_epgData->Size(); iEpgPtr++)
      m_parent->m_vecItems->Add(m_epgData->Get(iEpgPtr));

    m_parent->m_guideGrid->SetStartEnd(firstDate > gridStart ? firstDate : gridStart, lastDate);
    m_parent->m_viewControl.SetCurrentView(CONTROL_LIST_TIMELINE);
  }
//m_viewControl.SetSelectedItem(m_iSelected_GUIDE);
}

void CGUIWindowPVRGuide::UpdateData(void)
{
  CSingleLock lock(m_critSection);
  if (m_bIsFocusing)
    return;
  CLog::Log(LOGDEBUG, "CGUIWindowPVRGuide - %s - update window '%s'. set view to %d", __FUNCTION__, GetName(), m_iControlList);

  m_bIsFocusing = true;
  m_bUpdateRequired = false;

  /* lock the graphics context while updating */
  CSingleLock graphicsLock(g_graphicsContext);
  m_parent->m_viewControl.Clear();
  m_parent->m_vecItems->Clear();

  if (m_iGuideView == GUIDE_VIEW_CHANNEL)
    UpdateViewChannel();
  else if (m_iGuideView == GUIDE_VIEW_NOW)
    UpdateViewNow();
  else if (m_iGuideView == GUIDE_VIEW_NEXT)
    UpdateViewNext();
  else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
    UpdateViewTimeline();

  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(19222));
  UpdateButtons();

  m_bIsFocusing = false;
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
  bool bReturn = false;

  if (IsSelectedList(message))
  {
    int iAction = message.GetParam1();
    int iItem = m_parent->m_viewControl.GetSelectedItem();

    /* get the fileitem pointer */
    if (iItem < 0 || iItem >= (int) m_parent->m_vecItems->Size())
      return bReturn;
    CFileItemPtr pItem = m_parent->m_vecItems->Get(iItem);

    /* process actions */
    bReturn = true;
    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
    {
      if (g_advancedSettings.m_bPVRShowEpgInfoOnEpgItemSelect)
        ShowEPGInfo(pItem.get());
      else
        PlayEpgItem(pItem.get());
    }
    else if (iAction == ACTION_SHOW_INFO)
      ShowEPGInfo(pItem.get());
    else if (iAction == ACTION_RECORD)
      ActionRecord(pItem.get());
    else if (iAction == ACTION_PLAY)
      ActionPlayEpg(pItem.get());
    else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      m_parent->OnPopupMenu(iItem);
    else
      bReturn = false;
  }

  return bReturn;
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

bool CGUIWindowPVRGuide::PlayEpgItem(CFileItem *item)
{
  const CPVRChannel *channel = ((CPVREpgInfoTag *)item->GetEPGInfoTag())->ChannelTag();
  CLog::Log(LOG_DEBUG, "play channel '%s'", channel->ChannelName().c_str());
  bool bReturn = g_application.PlayFile(CFileItem(*channel));
  if (!bReturn)
    CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);

  return bReturn;
}

bool CGUIWindowPVRGuide::OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_PLAY_ITEM)
  {
    bReturn = PlayEpgItem(item);
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

void CGUIWindowPVRGuide::UpdateEpgCache(bool bRadio /* = false */, bool bForceUpdate /* = false */)
{
  CSingleLock lock(m_critSection);

  /* start observing the EPG for changes, so our cache becomes updated in the background */
  if (!m_bObservingEpg)
  {
    m_bObservingEpg = true;
    g_PVREpg->AddObserver(this);
  }

  if (!m_bObservingTimers)
  {
    m_bObservingTimers = true;
    g_PVRTimers->AddObserver(this);
  }

  if (!m_bGotInitialEpg || m_bLastEpgView != bRadio || bForceUpdate)
  {
    CLog::Log(LOGDEBUG, "CGUIWindowPVRGuide - %s - updating EPG cache", __FUNCTION__);

    m_epgData->Clear();
    g_PVREpg->GetEPGAll(m_epgData, bRadio);
  }
  m_bGotInitialEpg = true;
  m_bLastEpgView = bRadio;
}
