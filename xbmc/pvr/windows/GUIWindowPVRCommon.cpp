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

#include "GUIWindowPVRCommon.h"

#include "Application.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "epg/EpgInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "pvr/windows/GUIWindowPVRSearch.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "GUIUserMessages.h"

using namespace std;
using namespace PVR;
using namespace EPG;

CGUIWindowPVRCommon::CGUIWindowPVRCommon(CGUIWindowPVR *parent, PVRWindow window,
    unsigned int iControlButton, unsigned int iControlList)
{
  m_parent          = parent;
  m_window          = window;
  m_iControlButton  = iControlButton;
  m_iControlList    = iControlList;
  m_bUpdateRequired = false;
  m_iSelected       = 0;
  m_iSortOrder      = SORT_ORDER_ASC;
  m_iSortMethod     = SORT_METHOD_DATE;
  if( m_parent->GetViewState() )
  {
    m_iSortOrder      = m_parent->GetViewState()->GetSortOrder();
    m_iSortMethod     = m_parent->GetViewState()->GetSortMethod();
  }
}

bool CGUIWindowPVRCommon::operator ==(const CGUIWindowPVRCommon &right) const
{
  return (this == &right || m_window == right.m_window);
}

bool CGUIWindowPVRCommon::operator !=(const CGUIWindowPVRCommon &right) const
{
  return !(*this == right);
}

const char *CGUIWindowPVRCommon::GetName(void) const
{
  switch(m_window)
  {
  case PVR_WINDOW_EPG:
    return "epg";
  case PVR_WINDOW_CHANNELS_RADIO:
    return "radio";
  case PVR_WINDOW_CHANNELS_TV:
    return "tv";
  case PVR_WINDOW_RECORDINGS:
    return "recordings";
  case PVR_WINDOW_SEARCH:
    return "search";
  case PVR_WINDOW_TIMERS:
    return "timers";
  default:
    return "unknown";
  }
}

bool CGUIWindowPVRCommon::IsVisible(void) const
{
  return !g_application.IsPlayingFullScreenVideo() &&
      g_windowManager.GetActiveWindow() == WINDOW_PVR &&
      IsActive();
}

bool CGUIWindowPVRCommon::IsActive(void) const
{
  CGUIWindowPVRCommon *window = m_parent->GetActiveView();
  return (window && *window == *this);
}

bool CGUIWindowPVRCommon::IsSavedView(void) const
{
  CGUIWindowPVRCommon *window = m_parent->GetSavedView();
  return (window && *window == *this);
}

bool CGUIWindowPVRCommon::IsSelectedButton(CGUIMessage &message) const
{
  return (message.GetSenderId() == (int) m_iControlButton);
}

bool CGUIWindowPVRCommon::IsSelectedControl(CGUIMessage &message) const
{
  return (message.GetControlId() == (int) m_iControlButton);
}

bool CGUIWindowPVRCommon::IsSelectedList(CGUIMessage &message) const
{
  return (message.GetSenderId() == (int) m_iControlList);
}

void CGUIWindowPVRCommon::SetInvalid()
{
  for (int iItemPtr = 0; iItemPtr < m_parent->m_vecItems->Size(); iItemPtr++)
    m_parent->m_vecItems->Get(iItemPtr)->SetInvalid();
}

void CGUIWindowPVRCommon::OnInitWindow()
{
  m_parent->m_viewControl.SetCurrentView(m_iControlList);
}

bool CGUIWindowPVRCommon::SelectPlayingFile(void)
{
  bool bReturn(false);

  if (g_PVRManager.IsPlaying())
  {
    m_parent->m_viewControl.SetSelectedItem(g_application.CurrentFile());
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::OnMessageFocus(CGUIMessage &message)
{
  bool bReturn = false;

  if (message.GetMessage() == GUI_MSG_FOCUSED &&
      (IsSelectedControl(message) || IsSavedView()))
  {
    CLog::Log(LOGDEBUG, "CGUIWindowPVRCommon - %s - focus set to window '%s'", __FUNCTION__, GetName());
    bool bIsActive = IsActive();
    m_parent->SetActiveView(this);

    if (!bIsActive || m_bUpdateRequired)
      UpdateData();
    else
      m_iSelected = m_parent->m_viewControl.GetSelectedItem();

    bReturn = true;
  }

  return bReturn;
}

void CGUIWindowPVRCommon::OnWindowUnload(void)
{
  m_iSelected = m_parent->m_viewControl.GetSelectedItem();
}

bool CGUIWindowPVRCommon::OnAction(const CAction &action)
{
  bool bReturn = false;

  if (action.GetID() == ACTION_NAV_BACK ||
      action.GetID() == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= (int) m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  return (OnContextButtonSortAsc(pItem.get(), button) ||
      OnContextButtonSortBy(pItem.get(), button) ||
      OnContextButtonSortByChannel(pItem.get(), button) ||
      OnContextButtonSortByName(pItem.get(), button) ||
      OnContextButtonSortByDate(pItem.get(), button) ||
      OnContextButtonFind(pItem.get(), button) ||
      OnContextButtonMenuHooks(pItem.get(), button));
}

bool CGUIWindowPVRCommon::OnContextButtonSortByDate(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SORTBY_DATE)
  {
    bReturn = true;

    if (m_iSortMethod != SORT_METHOD_DATE)
    {
      m_iSortMethod = SORT_METHOD_DATE;
      m_iSortOrder  = SORT_ORDER_ASC;
      CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, m_parent->GetID(), 0, m_iSortMethod, 0); 
      m_parent->OnMessage(message);
    }
    else
    {
      m_iSortOrder = m_iSortOrder == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
    }
    CGUIMessage message(GUI_MSG_CHANGE_SORT_DIRECTION, m_parent->GetID(), 0, m_iSortOrder, 0); 
    m_parent->OnMessage(message);
    UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::OnContextButtonSortByName(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SORTBY_NAME)
  {
    bReturn = true;

    if (m_iSortMethod != SORT_METHOD_LABEL)
    {
      m_iSortMethod = SORT_METHOD_LABEL;
      m_iSortOrder  = SORT_ORDER_ASC;
      CGUIMessage message(GUI_MSG_CHANGE_SORT_METHOD, m_parent->GetID(), 0, m_iSortMethod, 0); 
      m_parent->OnMessage(message);
    }
    else
    {
      m_iSortOrder = m_iSortOrder == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
    }
    CGUIMessage message(GUI_MSG_CHANGE_SORT_DIRECTION, m_parent->GetID(), 0, m_iSortOrder, 0); 
    m_parent->OnMessage(message);
    UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::OnContextButtonSortByChannel(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SORTBY_CHANNEL)
  {
    bReturn = true;

    if (m_iSortMethod != SORT_METHOD_CHANNEL)
    {
      m_iSortMethod = SORT_METHOD_CHANNEL;
      m_iSortOrder  = SORT_ORDER_ASC;
    }
    else
    {
      m_iSortOrder = m_iSortOrder == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
    }

    UpdateData();
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::OnContextButtonSortAsc(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SORTASC)
  {
    bReturn = true;

    if (m_parent->m_guiState.get())
      m_parent->m_guiState->SetNextSortOrder();
    m_parent->UpdateFileList();
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::OnContextButtonSortBy(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_SORTBY)
  {
    bReturn = true;

    if (m_parent->m_guiState.get())
      m_parent->m_guiState->SetNextSortMethod();

    m_parent->UpdateFileList();
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::OnContextButtonMenuHooks(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_MENU_HOOKS)
  {
    bReturn = true;

    if (item->IsEPG() && item->GetEPGInfoTag()->HasPVRChannel())
      g_PVRClients->ProcessMenuHooks(item->GetEPGInfoTag()->ChannelTag()->ClientID());
    else if (item->IsPVRChannel())
      g_PVRClients->ProcessMenuHooks(item->GetPVRChannelInfoTag()->ClientID());
    else if (item->IsPVRRecording())
      g_PVRClients->ProcessMenuHooks(item->GetPVRRecordingInfoTag()->m_iClientId);
    else if (item->IsPVRTimer())
      g_PVRClients->ProcessMenuHooks(item->GetPVRTimerInfoTag()->m_iClientId);
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::ActionDeleteTimer(CFileItem *item)
{
  /* check if the timer tag is valid */
  CPVRTimerInfoTag *timerTag = item->GetPVRTimerInfoTag();
  if (!timerTag || timerTag->m_iClientIndex < 0)
    return false;

  /* show a confirmation dialog */
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return false;
  pDialog->SetHeading(122);
  pDialog->SetLine(0, 19040);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, timerTag->m_strTitle);
  pDialog->DoModal();

  /* prompt for the user's confirmation */
  if (!pDialog->IsConfirmed())
    return false;

  /* delete the timer */
  return g_PVRTimers->DeleteTimer(*item);
}

bool CGUIWindowPVRCommon::ShowNewTimerDialog(void)
{
  bool bReturn(false);

  CPVRTimerInfoTag *newTimer = new CPVRTimerInfoTag;
  CFileItem *newItem = new CFileItem(*newTimer);
  if (ShowTimerSettings(newItem))
  {
    /* Add timer to backend */
    bReturn = g_PVRTimers->AddTimer(*newItem);
  }

  delete newItem;
  delete newTimer;

  return bReturn;
}

bool CGUIWindowPVRCommon::ActionShowTimer(CFileItem *item)
{
  bool bReturn = false;

  /* Check if "Add timer..." entry is pressed by OK, if yes
     create a new timer and open settings dialog, otherwise
     open settings for selected timer entry */
  if (item->GetPath() == "pvr://timers/add.timer")
  {
    bReturn = ShowNewTimerDialog();
  }
  else
  {
    if (ShowTimerSettings(item))
    {
      /* Update timer on pvr backend */
      bReturn = g_PVRTimers->UpdateTimer(*item);
    }
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::ActionRecord(CFileItem *item)
{
  bool bReturn = false;

  CEpgInfoTag *epgTag = item->GetEPGInfoTag();
  if (!epgTag)
    return bReturn;

  const CPVRChannel *channel = epgTag->ChannelTag();
  if (!channel || channel->ChannelNumber() > 0)
    return bReturn;

  if (epgTag->Timer() == NULL)
  {
    /* create a confirmation dialog */
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*) g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;

    pDialog->SetHeading(264);
    pDialog->SetLine(0, "");
    pDialog->SetLine(1, epgTag->Title());
    pDialog->SetLine(2, "");
    pDialog->DoModal();

    /* prompt for the user's confirmation */
    if (!pDialog->IsConfirmed())
      return bReturn;

    CPVRTimerInfoTag *newtimer = CPVRTimerInfoTag::CreateFromEpg(*epgTag);
    CFileItem *item = new CFileItem(*newtimer);

    bReturn = g_PVRTimers->AddTimer(*item);
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput(19033,19034,0,0);
    bReturn = true;
  }

  return bReturn;
}


bool CGUIWindowPVRCommon::ActionDeleteRecording(CFileItem *item)
{
  bool bReturn = false;

  /* check if the recording tag is valid */
  CPVRRecording *recTag = (CPVRRecording *) item->GetPVRRecordingInfoTag();
  if (!recTag || recTag->m_strRecordingId.IsEmpty())
    return bReturn;

  /* show a confirmation dialog */
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return bReturn;
  pDialog->SetHeading(122);
  pDialog->SetLine(0, 19043);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, recTag->m_strTitle);
  pDialog->DoModal();

  /* prompt for the user's confirmation */
  if (!pDialog->IsConfirmed())
    return bReturn;

  /* delete the recording */
  if (g_PVRRecordings->DeleteRecording(*item))
  {
    g_PVRManager.TriggerRecordingsUpdate();
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::ActionPlayChannel(CFileItem *item)
{
  bool bReturn = false;

  if (item->GetPath() == "pvr://channels/.add.channel")
  {
    /* show "add channel" dialog */
    CGUIDialogOK::ShowAndGetInput(19033,0,19038,0);
    bReturn = true;
  }
  else
  {
    /* open channel */
    bReturn = PlayFile(item, g_guiSettings.GetBool("pvrplayback.playminimized"));
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::ActionPlayEpg(CFileItem *item)
{
  bool bReturn = false;

  CEpgInfoTag *epgTag = item->GetEPGInfoTag();
  if (!epgTag)
    return bReturn;

  const CPVRChannel *channel = epgTag->ChannelTag();
  if (!channel || channel->ChannelNumber() > 0)
    return bReturn;

  bReturn = g_application.PlayFile(CFileItem(*channel));

  if (!bReturn)
  {
    /* cannot play file */
    CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);
  }

  return bReturn;
}

bool CGUIWindowPVRCommon::ActionDeleteChannel(CFileItem *item)
{
  CPVRChannel *channel = item->GetPVRChannelInfoTag();

  /* check if the channel tag is valid */
  if (!channel || channel->ChannelNumber() <= 0)
    return false;

  /* show a confirmation dialog */
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*) g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
    return false;
  pDialog->SetHeading(19039);
  pDialog->SetLine(0, "");
  pDialog->SetLine(1, channel->ChannelName());
  pDialog->SetLine(2, "");
  pDialog->DoModal();

  /* prompt for the user's confirmation */
  if (!pDialog->IsConfirmed())
    return false;

  g_PVRChannelGroups->GetGroupAll(channel->IsRadio())->RemoveFromGroup(*channel);
  UpdateData();

  return true;
}

bool CGUIWindowPVRCommon::ShowTimerSettings(CFileItem *item)
{
  /* Check item is TV timer information tag */
  if (!item->IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRTimers: Can't open timer settings dialog, no timer info tag!");
    return false;
  }

  /* Load timer settings dialog */
  CGUIDialogPVRTimerSettings* pDlgInfo = (CGUIDialogPVRTimerSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);

  if (!pDlgInfo)
    return false;

  /* inform dialog about the file item */
  pDlgInfo->SetTimer(item);

  /* Open dialog window */
  pDlgInfo->DoModal();

  /* Get modify flag from window and return it to caller */
  return pDlgInfo->GetOK();
}


bool CGUIWindowPVRCommon::PlayRecording(CFileItem *item, bool bPlayMinimized /* = false */)
{
  if (item->GetPath().Left(17) != "pvr://recordings/")
    return false;

  CStdString stream = item->GetPVRRecordingInfoTag()->m_strStreamURL;
  if (stream == "")
    return false;

  /* Isolate the folder from the filename */
  size_t found = stream.find_last_of("/");
  if (found == CStdString::npos)
    found = stream.find_last_of("\\");

  if (found != CStdString::npos)
  {
    /* Check here for asterisk at the begin of the filename */
    if (stream[found+1] == '*')
    {
      /* Create a "stack://" url with all files matching the extension */
      CStdString ext = URIUtils::GetExtension(stream);
      CStdString dir = stream.substr(0, found).c_str();

      CFileItemList items;
      CDirectory::GetDirectory(dir, items);
      items.Sort(SORT_METHOD_FILE ,SORT_ORDER_ASC);

      vector<int> stack;
      for (int i = 0; i < items.Size(); ++i)
      {
        if (URIUtils::GetExtension(items[i]->GetPath()) == ext)
          stack.push_back(i);
      }

      if (stack.size() > 0)
      {
        /* If we have a stack change the path of the item to it */
        CStackDirectory dir;
        CStdString stackPath = dir.ConstructStackPath(items, stack);
        item->SetPath(stackPath);
      }
    }
    else
    {
      /* If no asterisk is present play only the given stream URL */
      item->SetPath(stream);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVRManager - %s - can't open recording: no valid filename", __FUNCTION__);
    CGUIDialogOK::ShowAndGetInput(19033,0,19036,0);
    return false;
  }

  g_application.getApplicationMessenger().PlayFile(*item, false);

  return true;
}

bool CGUIWindowPVRCommon::PlayFile(CFileItem *item, bool bPlayMinimized /* = false */)
{
  if (bPlayMinimized)
  {
    if (item->GetPath() == g_application.CurrentFile())
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, m_parent->GetID());
      g_windowManager.SendMessage(msg);
      return true;
    }
    else
    {
      g_settings.m_bStartVideoWindowed = true;
    }
  }

  if (item->GetPath().Left(17) == "pvr://recordings/")
  {
    return PlayRecording(item, bPlayMinimized);
  }
  else
  {
    bool bSwitchSuccessful(false);

    /* try a fast switch */
    if (item->IsPVRChannel() && (g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio()) &&
        (item->GetPVRChannelInfoTag()->IsRadio() == g_PVRManager.IsPlayingRadio()) && g_application.m_pPlayer)
    {
      CPVRChannel* channel = item->GetPVRChannelInfoTag();
      if (channel->StreamURL().IsEmpty())
        bSwitchSuccessful = g_application.m_pPlayer->SwitchChannel(*channel);
    }

    if (!bSwitchSuccessful)
    {
      g_application.getApplicationMessenger().PlayFile(*item, false);
      return true;
    }

    if (!bSwitchSuccessful)
    {
      CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);
      return false;
    }
  }

  return true;
}

bool CGUIWindowPVRCommon::StartRecordFile(CFileItem *item)
{
  if (!item->HasEPGInfoTag())
    return false;

  CEpgInfoTag *tag = item->GetEPGInfoTag();
  if (!tag || !tag->ChannelTag() || tag->ChannelTag()->ChannelNumber() <= 0)
    return false;

  CPVRTimerInfoTag *timer = g_PVRTimers->GetMatch(item);
  if (timer)
  {
    CGUIDialogOK::ShowAndGetInput(19033,19034,0,0);
    return false;
  }

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return false;
  pDialog->SetHeading(264);
  pDialog->SetLine(0, tag->ChannelTag()->ChannelName());
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, tag->Title());
  pDialog->DoModal();

  if (!pDialog->IsConfirmed())
    return false;

  CPVRTimerInfoTag *newtimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
  CFileItem *newTimerItem = new CFileItem(*newtimer);

  return g_PVRTimers->AddTimer(*newTimerItem);
}

bool CGUIWindowPVRCommon::StopRecordFile(CFileItem *item)
{
  if (!item->HasEPGInfoTag())
    return false;

  CEpgInfoTag *tag = item->GetEPGInfoTag();
  if (!tag || !tag->ChannelTag() || tag->ChannelTag()->ChannelNumber() <= 0)
    return false;

  CPVRTimerInfoTag *timer = g_PVRTimers->GetMatch(item);
  if (!timer || timer->m_bIsRepeating)
    return false;

  return g_PVRTimers->DeleteTimer(*timer);
}

void CGUIWindowPVRCommon::ShowEPGInfo(CFileItem *item)
{
  CFileItem *tag = NULL;
  if (item->IsEPG())
  {
    tag = new CFileItem(*item);
  }
  else if (item->IsPVRChannel())
  {
    CEpgInfoTag epgnow;
    if (!item->GetPVRChannelInfoTag()->GetEPGNow(epgnow))
    {
      CGUIDialogOK::ShowAndGetInput(19033,0,19055,0);
      return;
    }
    tag = new CFileItem(epgnow);
  }

  if (tag)
  {
    CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
      return;

    pDlgInfo->SetProgInfo(tag);
    pDlgInfo->DoModal();

    delete tag;
  }
}

void CGUIWindowPVRCommon::ShowRecordingInfo(CFileItem *item)
{
  if (!item->IsPVRRecording())
    return;

  CGUIDialogPVRRecordingInfo* pDlgInfo = (CGUIDialogPVRRecordingInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_RECORDING_INFO);
  if (!pDlgInfo)
    return;

  pDlgInfo->SetRecording(item);
  pDlgInfo->DoModal();
}

bool CGUIWindowPVRCommon::OnContextButtonFind(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_FIND)
  {
    bReturn = true;
    if (m_parent->m_windowSearch)
    {
      CEpgInfoTag tag;
      m_parent->m_windowSearch->m_searchfilter.Reset();
      if (item->IsEPG())
        m_parent->m_windowSearch->m_searchfilter.m_strSearchTerm = "\"" + item->GetEPGInfoTag()->Title() + "\"";
      else if (item->IsPVRChannel() && item->GetPVRChannelInfoTag()->GetEPGNow(tag))
        m_parent->m_windowSearch->m_searchfilter.m_strSearchTerm = "\"" + tag.Title() + "\"";
      else if (item->IsPVRRecording())
        m_parent->m_windowSearch->m_searchfilter.m_strSearchTerm = "\"" + item->GetPVRRecordingInfoTag()->m_strTitle + "\"";

      m_parent->m_windowSearch->m_bSearchConfirmed = true;
      m_parent->SetLabel(m_iControlButton, 0);
      m_parent->SetActiveView(m_parent->m_windowSearch);
      m_parent->m_windowSearch->UpdateData();
      m_parent->SetLabel(m_iControlList, 0);
    }
  }

  return bReturn;
}
