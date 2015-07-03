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

#include "GUIWindowPVRBase.h"

#include "Application.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/StackDirectory.h"
#include "input/Key.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "GUIWindowPVRRecordings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/timers/PVRTimers.h"
#include "epg/Epg.h"
#include "epg/GUIEPGGridContainer.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/Observer.h"

using namespace PVR;
using namespace EPG;

std::map<bool, std::string> CGUIWindowPVRBase::m_selectedItemPaths;

CGUIWindowPVRBase::CGUIWindowPVRBase(bool bRadio, int id, const std::string &xmlFile) :
  CGUIMediaWindow(id, xmlFile.c_str()),
  m_bRadio(bRadio)
{
  m_selectedItemPaths[false] = "";
  m_selectedItemPaths[true] = "";
}

CGUIWindowPVRBase::~CGUIWindowPVRBase(void)
{
}

void CGUIWindowPVRBase::SetSelectedItemPath(bool bRadio, const std::string &path)
{
  m_selectedItemPaths.at(bRadio) = path;
}

std::string CGUIWindowPVRBase::GetSelectedItemPath(bool bRadio)
{
  if (!m_selectedItemPaths.at(bRadio).empty())
    return m_selectedItemPaths.at(bRadio);
  else if (g_PVRManager.IsPlaying())
    return g_application.CurrentFile();

  return "";
}

void CGUIWindowPVRBase::Notify(const Observable &obs, const ObservableMessage msg)
{
  UpdateSelectedItemPath();
  CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, msg);
  CApplicationMessenger::Get().SendGUIMessage(m);
}

bool CGUIWindowPVRBase::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_PREVIOUS_CHANNELGROUP:
    case ACTION_NEXT_CHANNELGROUP:
      // switch to next or previous group
      SetGroup(action.GetID() == ACTION_NEXT_CHANNELGROUP ? m_group->GetNextGroup() : m_group->GetPreviousGroup());
      return true;
  }

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowPVRBase::OnBack(int actionID)
{
  if (actionID == ACTION_NAV_BACK)
  {
    // don't call CGUIMediaWindow as it will attempt to go to the parent folder which we don't want.
    if (GetPreviousWindow() != WINDOW_FULLSCREEN_LIVETV)
      g_windowManager.ActivateWindow(WINDOW_HOME);
    else
      return CGUIWindow::OnBack(actionID);
  }
  return CGUIMediaWindow::OnBack(actionID);
}

void CGUIWindowPVRBase::OnInitWindow(void)
{
  if (!g_PVRManager.IsStarted() || !g_PVRClients->HasConnectedClients())
  {
    g_windowManager.PreviousWindow();
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning,
        g_localizeStrings.Get(19045),
        g_localizeStrings.Get(19044));
    return;
  }

  m_vecItems->SetPath(GetDirectoryPath());

  CGUIMediaWindow::OnInitWindow();

  // mark item as selected by channel path
  m_viewControl.SetSelectedItem(GetSelectedItemPath(m_bRadio));
}

void CGUIWindowPVRBase::OnDeinitWindow(int nextWindowID)
{
  UpdateSelectedItemPath();
}

bool CGUIWindowPVRBase::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CPVRChannelGroupPtr group = g_PVRManager.GetPlayingGroup(m_bRadio);
      if (m_group != group)
        m_viewControl.SetSelectedItem(0);
      m_group = group;
      SetProperty("IsRadio", m_bRadio ? "true" : "");
    }
    break;

    case GUI_MSG_CLICKED:
    {
      switch (message.GetSenderId())
      {
        case CONTROL_BTNCHANNELGROUPS:
          return OpenGroupSelectionDialog();
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowPVRBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  switch(button)
  {
    case CONTEXT_BUTTON_MENU_HOOKS:
      if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
      {
        CFileItemPtr item = m_vecItems->Get(itemNumber);

        if (item->IsEPG() && item->GetEPGInfoTag()->HasPVRChannel())
          g_PVRClients->ProcessMenuHooks(item->GetEPGInfoTag()->ChannelTag()->ClientID(), PVR_MENUHOOK_EPG, item.get());
        else if (item->IsPVRChannel())
          g_PVRClients->ProcessMenuHooks(item->GetPVRChannelInfoTag()->ClientID(), PVR_MENUHOOK_CHANNEL, item.get());
        else if (item->IsDeletedPVRRecording())
          g_PVRClients->ProcessMenuHooks(item->GetPVRRecordingInfoTag()->m_iClientId, PVR_MENUHOOK_DELETED_RECORDING, item.get());
        else if (item->IsUsablePVRRecording())
          g_PVRClients->ProcessMenuHooks(item->GetPVRRecordingInfoTag()->m_iClientId, PVR_MENUHOOK_RECORDING, item.get());
        else if (item->IsPVRTimer())
          g_PVRClients->ProcessMenuHooks(item->GetPVRTimerInfoTag()->m_iClientId, PVR_MENUHOOK_TIMER, item.get());

        bReturn = true;
      }
      break;
    case CONTEXT_BUTTON_FIND:
    {
      int windowSearchId = m_bRadio ? WINDOW_RADIO_SEARCH : WINDOW_TV_SEARCH;
      CGUIWindowPVRBase *windowSearch = (CGUIWindowPVRBase*) g_windowManager.GetWindow(windowSearchId);
      if (windowSearch && itemNumber >= 0 && itemNumber < m_vecItems->Size())
      {
        CFileItemPtr item = m_vecItems->Get(itemNumber);
        g_windowManager.ActivateWindow(windowSearchId);
        bReturn = windowSearch->OnContextButton(*item.get(), button);
      }
      break;
    }
    default:
      bReturn = false;
  }

  return bReturn || CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRBase::SetInvalid()
{
  VECFILEITEMS items = m_vecItems->GetList();
  for (VECFILEITEMS::iterator it = items.begin(); it != items.end(); ++it)
    (*it)->SetInvalid();
  CGUIMediaWindow::SetInvalid();
}

bool CGUIWindowPVRBase::OpenGroupSelectionDialog(void)
{
  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!dialog)
    return false;

  CFileItemList options;
  g_PVRChannelGroups->Get(m_bRadio)->GetGroupList(&options, true);

  dialog->Reset();
  dialog->SetHeading(g_localizeStrings.Get(19146));
  dialog->SetItems(&options);
  dialog->SetMultiSelection(false);
  dialog->SetSelected(m_group->GroupName());
  dialog->DoModal();

  if (!dialog->IsConfirmed())
    return false;

  const CFileItemPtr item = dialog->GetSelectedItem();
  if (!item)
    return false;

  SetGroup(g_PVRChannelGroups->Get(m_bRadio)->GetByName(item->m_strTitle));

  return true;
}

CPVRChannelGroupPtr CGUIWindowPVRBase::GetGroup(void)
{
  CSingleLock lock(m_critSection);
  return m_group;
}

void CGUIWindowPVRBase::SetGroup(CPVRChannelGroupPtr group)
{
  CSingleLock lock(m_critSection);
  if (!group)
    return;

  if (m_group != group)
  {
    if (m_group)
      m_group->UnregisterObserver(this);
    m_group = group;
    // we need to register the window to receive changes from the new group
    m_group->RegisterObserver(this);
    g_PVRManager.SetPlayingGroup(m_group);
    Update(GetDirectoryPath());
  }
}

bool CGUIWindowPVRBase::PlayFile(CFileItem *item, bool bPlayMinimized /* = false */, bool bCheckResume /* = true */)
{
  if (item->m_bIsFolder)
  {
    return false;
  }

  CPVRChannelPtr channel = item->HasPVRChannelInfoTag() ? item->GetPVRChannelInfoTag() : CPVRChannelPtr();
  if (item->GetPath() == g_application.CurrentFile() ||
      (channel && channel->HasRecording() && channel->GetRecording()->GetPath() == g_application.CurrentFile()))
  {
    CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, GetID());
    g_windowManager.SendMessage(msg);
    return true;
  }

  CMediaSettings::Get().SetVideoStartWindowed(bPlayMinimized);

  if (item->HasPVRRecordingInfoTag())
  {
    return PlayRecording(item, bPlayMinimized, bCheckResume);
  }
  else
  {
    bool bSwitchSuccessful(false);
    CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

    if (channel && g_PVRManager.CheckParentalLock(channel))
    {
      CPVRRecordingPtr recording = channel->GetRecording();
      if (recording)
      {
        CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*) g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
        if (pDialog)
        {
          pDialog->SetHeading(19687); // Play recording
          pDialog->SetLine(0, "");
          pDialog->SetLine(1, 12021); // Start from beginning
          pDialog->SetLine(2, recording->m_strTitle.c_str());
          pDialog->DoModal();

          if (pDialog->IsConfirmed())
          {
            CFileItem recordingItem(recording);
            return PlayRecording(&recordingItem, CSettings::Get().GetBool("pvrplayback.playminimized"), bCheckResume);
          }
        }
      }

      /* try a fast switch */
      if ((g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio()) &&
         (channel->IsRadio() == g_PVRManager.IsPlayingRadio()))
      {
        if (channel->StreamURL().empty())
          bSwitchSuccessful = g_application.m_pPlayer->SwitchChannel(channel);
      }

      if (!bSwitchSuccessful)
      {
        CApplicationMessenger::Get().PlayFile(*item, false);
        return true;
      }
    }

    if (!bSwitchSuccessful)
    {
      std::string channelName = g_localizeStrings.Get(19029); // Channel
      if (channel)
        channelName = channel->ChannelName();
      std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), channelName.c_str()); // CHANNELNAME could not be played. Check the log for details.

      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
              g_localizeStrings.Get(19166), // PVR information
              msg);
      return false;
    }
  }

  return true;
}

bool CGUIWindowPVRBase::ShowTimerSettings(CFileItem *item)
{
  if (!item->IsPVRTimer())
    return false;

  CGUIDialogPVRTimerSettings* pDlgInfo = (CGUIDialogPVRTimerSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);

  if (!pDlgInfo)
    return false;

  pDlgInfo->SetTimer(item);
  pDlgInfo->DoModal();

  return pDlgInfo->IsConfirmed();
}

bool CGUIWindowPVRBase::StartRecordFile(CFileItem *item, bool bAdvanced)
{
  if (!item->HasEPGInfoTag())
    return false;

  const CEpgInfoTagPtr tag = item->GetEPGInfoTag();
  CPVRChannelPtr channel = tag->ChannelTag();

  if (!channel || !g_PVRManager.CheckParentalLock(channel))
    return false;

  CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(item);
  if (timer && timer->HasPVRTimerInfoTag())
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19034);
    return false;
  }

  bool bReturn(false);

  if (bAdvanced)
  {
    CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTag::CreateFromEpg(tag, true);
    if (newTimer)
    {
      CFileItem *newItem = new CFileItem(newTimer);

      if (ShowTimerSettings(newItem))
        bReturn = g_PVRTimers->AddTimer(newItem->GetPVRTimerInfoTag());

      delete newItem;
    }
  }
  else
  {
    // ask for confirmation before starting a timer
    if (!CGUIDialogYesNo::ShowAndGetInput(
          264 /* "Stop Rec." */, tag->PVRChannelName(), "", tag->Title()))
      return false;

    CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTag::CreateFromEpg(tag);

    if (newTimer)
      bReturn = g_PVRTimers->AddTimer(newTimer);
  }
  return bReturn;
}

bool CGUIWindowPVRBase::StopRecordFile(CFileItem *item)
{
  if (!item->HasEPGInfoTag())
    return false;

  const CEpgInfoTagPtr tag(item->GetEPGInfoTag());
  if (!tag || !tag->HasPVRChannel())
    return false;

  CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(item);
  if (!timer || !timer->HasPVRTimerInfoTag())
    return false;

  bool bDeleteScheduled(false);
  if (ConfirmDeleteTimer(timer.get(), bDeleteScheduled))
    return g_PVRTimers->DeleteTimer(*timer, false, bDeleteScheduled);

  return false;
}

void CGUIWindowPVRBase::CheckResumeRecording(CFileItem *item)
{
  std::string resumeString = CGUIWindowPVRRecordings::GetResumeString(*item);
  if (!resumeString.empty())
  {
    CContextButtons choices;
    choices.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
    choices.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); // Start from beginning
    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (choice > 0)
      item->m_lStartOffset = choice == CONTEXT_BUTTON_RESUME_ITEM ? STARTOFFSET_RESUME : 0;
  }
}

bool CGUIWindowPVRBase::PlayRecording(CFileItem *item, bool bPlayMinimized /* = false */, bool bCheckResume /* = true */)
{
  if (!item->HasPVRRecordingInfoTag())
    return false;

  std::string stream = item->GetPVRRecordingInfoTag()->m_strStreamURL;
  if (stream.empty())
  {
    if (bCheckResume)
      CheckResumeRecording(item);
    CApplicationMessenger::Get().PlayFile(*item, false);
    return true;
  }

  /* Isolate the folder from the filename */
  size_t found = stream.find_last_of("/");
  if (found == std::string::npos)
    found = stream.find_last_of("\\");

  if (found != std::string::npos)
  {
    /* Check here for asterisk at the begin of the filename */
    if (stream[found+1] == '*')
    {
      /* Create a "stack://" url with all files matching the extension */
      std::string ext = URIUtils::GetExtension(stream);
      std::string dir = stream.substr(0, found);

      CFileItemList items;
      XFILE::CDirectory::GetDirectory(dir, items);
      items.Sort(SortByFile, SortOrderAscending);

      std::vector<int> stack;
      for (int i = 0; i < items.Size(); ++i)
      {
        if (URIUtils::HasExtension(items[i]->GetPath(), ext))
          stack.push_back(i);
      }

      if (stack.empty())
      {
        /* If we have a stack change the path of the item to it */
        XFILE::CStackDirectory dir;
        std::string stackPath = dir.ConstructStackPath(items, stack);
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
    CLog::Log(LOGERROR, "CGUIWindowPVRCommon - %s - can't open recording: no valid filename", __FUNCTION__);
    CGUIDialogOK::ShowAndGetInput(19033, 19036);
    return false;
  }

  if (bCheckResume)
    CheckResumeRecording(item);
  CApplicationMessenger::Get().PlayFile(*item, false);

  return true;
}

void CGUIWindowPVRBase::ShowRecordingInfo(CFileItem *item)
{
  CGUIDialogPVRRecordingInfo* pDlgInfo = (CGUIDialogPVRRecordingInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_RECORDING_INFO);
  if (item->IsPVRRecording() && pDlgInfo)
  {
    pDlgInfo->SetRecording(item);
    pDlgInfo->DoModal();
  }
}

void CGUIWindowPVRBase::ShowEPGInfo(CFileItem *item)
{
  CFileItem *tag = NULL;
  bool bHasChannel(false);
  CPVRChannelPtr channel;
  if (item->IsEPG())
  {
    tag = new CFileItem(*item);
    if (item->GetEPGInfoTag()->HasPVRChannel())
    {
      channel = item->GetEPGInfoTag()->ChannelTag();
      bHasChannel = true;
    }
  }
  else if (item->IsPVRTimer())
  {
    CEpgInfoTagPtr epg(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
    if (epg && epg->HasPVRChannel())
    {
      channel = epg->ChannelTag();
      bHasChannel = true;
    }
    tag = new CFileItem(epg);
  }
  else if (item->IsPVRChannel())
  {
    CEpgInfoTagPtr epgnow(item->GetPVRChannelInfoTag()->GetEPGNow());
    channel = item->GetPVRChannelInfoTag();
    bHasChannel = true;
    if (!epgnow)
    {
      CGUIDialogOK::ShowAndGetInput(19033, 19055);
      return;
    }
    tag = new CFileItem(epgnow);
  }

  CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
  if (tag && (!bHasChannel || g_PVRManager.CheckParentalLock(channel)) && pDlgInfo)
  {
    pDlgInfo->SetProgInfo(tag);
    pDlgInfo->DoModal();
  }

  delete tag;
}

bool CGUIWindowPVRBase::ActionInputChannelNumber(int input)
{
  std::string strInput = StringUtils::Format("%i", input);
  if (CGUIDialogNumeric::ShowAndGetNumber(strInput, g_localizeStrings.Get(19103)))
  {
    int iChannelNumber = atoi(strInput.c_str());
    if (iChannelNumber >= 0)
    {
      int itemIndex = 0;
      VECFILEITEMS items = m_vecItems->GetList();
      for (VECFILEITEMS::iterator it = items.begin(); it != items.end(); ++it)
      {
        if(((*it)->HasPVRChannelInfoTag() && (*it)->GetPVRChannelInfoTag()->ChannelNumber() == iChannelNumber) ||
           ((*it)->HasEPGInfoTag() && (*it)->GetEPGInfoTag()->HasPVRChannel() && (*it)->GetEPGInfoTag()->PVRChannelNumber() == iChannelNumber))
        {
          // different handling for guide grid
          if ((GetID() == WINDOW_TV_GUIDE || GetID() == WINDOW_RADIO_GUIDE) &&
              m_viewControl.GetCurrentControl() == GUIDE_VIEW_TIMELINE)
          {
            CGUIEPGGridContainer* epgGridContainer = (CGUIEPGGridContainer*) GetControl(m_viewControl.GetCurrentControl());
            if ((*it)->HasEPGInfoTag() && (*it)->GetEPGInfoTag()->HasPVRChannel())
              epgGridContainer->SetChannel((*it)->GetEPGInfoTag()->ChannelTag());
            else
              epgGridContainer->SetChannel((*it)->GetPVRChannelInfoTag());
          }
          else
            m_viewControl.SetSelectedItem(itemIndex);
          return true;
        }
        itemIndex++;
      }
    }
  }

  return false;
}

bool CGUIWindowPVRBase::ActionPlayChannel(CFileItem *item)
{
  return PlayFile(item, CSettings::Get().GetBool("pvrplayback.playminimized"));
}

bool CGUIWindowPVRBase::ActionPlayEpg(CFileItem *item, bool bPlayRecording)
{
  if (!item || !item->HasEPGInfoTag())
    return false;

  CPVRChannelPtr channel;
  CEpgInfoTagPtr epgTag(item->GetEPGInfoTag());
  if (epgTag && epgTag->HasPVRChannel())
    channel = epgTag->ChannelTag();

  if (!channel || !g_PVRManager.CheckParentalLock(channel))
    return false;

  CFileItem fileItem;
  if (bPlayRecording && epgTag->HasRecording())
    fileItem = CFileItem(epgTag->Recording());
  else
    fileItem = CFileItem(channel);

  g_application.SwitchToFullScreen();
  if (!PlayFile(&fileItem))
  {
    // CHANNELNAME could not be played. Check the log for details.
    std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), channel->ChannelName().c_str());
    CGUIDialogOK::ShowAndGetInput(19033, msg);
    return false;
  }

  return true;
}

bool CGUIWindowPVRBase::ActionDeleteChannel(CFileItem *item)
{
  CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

  /* check if the channel tag is valid */
  if (!channel || channel->ChannelNumber() <= 0)
    return false;

  /* show a confirmation dialog */
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*) g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return false;
  pDialog->SetHeading(19039);
  pDialog->SetLine(0, "");
  pDialog->SetLine(1, channel->ChannelName());
  pDialog->SetLine(2, "");
  pDialog->DoModal();

  /* prompt for the user's confirmation */
  if (!pDialog->IsConfirmed())
    return false;

  g_PVRChannelGroups->GetGroupAll(channel->IsRadio())->RemoveFromGroup(channel);
  Refresh(true);

  return true;
}

bool CGUIWindowPVRBase::ActionRecord(CFileItem *item)
{
  bool bReturn = false;

  CEpgInfoTagPtr epgTag(item->GetEPGInfoTag());
  if (!epgTag)
    return bReturn;

  CPVRChannelPtr channel = epgTag->ChannelTag();
  if (!channel || !g_PVRManager.CheckParentalLock(channel))
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

    CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTag::CreateFromEpg(epgTag);
    if (newTimer)
    {
      bReturn = g_PVRTimers->AddTimer(newTimer);
    }
    else
    {
      bReturn = false;
    }
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput(19033, 19034);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRBase::UpdateEpgForChannel(CFileItem *item)
{
  CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

  CEpg *epg = channel->GetEPG();
  if (!epg)
    return false;

  epg->ForceUpdate();
  return true;
}

void CGUIWindowPVRBase::UpdateButtons(void)
{
  CGUIMediaWindow::UpdateButtons();
  SET_CONTROL_LABEL(CONTROL_BTNCHANNELGROUPS, g_localizeStrings.Get(19141) + ": " + m_group->GroupName());
}

void CGUIWindowPVRBase::UpdateSelectedItemPath()
{
  m_selectedItemPaths.at(m_bRadio) = m_viewControl.GetSelectedItemPath();
}

// static
bool CGUIWindowPVRBase::ConfirmDeleteTimer(CFileItem *item, bool &bDeleteSchedule)
{
  bool bConfirmed(false);

  if (item->GetPVRTimerInfoTag()->IsRepeating())
  {
    // prompt user for confirmation for deleting the complete repeating timer, including scheduled timers.
    bool bCancel(false);
    bDeleteSchedule = CGUIDialogYesNo::ShowAndGetInput(
                        122, // "Confirm delete"
                        840, // "You are about to delete a repeating timer. Do you also want to delete all timers currently scheduled by this timer?"
                        "",
                        item->GetPVRTimerInfoTag()->Title(),
                        bCancel);
    bConfirmed = !bCancel;
  }
  else
  {
    // prompt user for confirmation for deleting the timer
    bConfirmed = CGUIDialogYesNo::ShowAndGetInput(
                        122, // Confirm delete
                        19040, // Timer
                        "",
                        item->GetPVRTimerInfoTag()->Title());
    bDeleteSchedule = false;
  }

  return bConfirmed;
}
