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
#include "GUIWindowPVRRecordings.h"

#include "Application.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "epg/Epg.h"
#include "epg/GUIEPGGridContainer.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/Observer.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/timers/PVRTimers.h"

#include <utility>

#define MAX_INVALIDATION_FREQUENCY 2000 // limit to one invalidation per X milliseconds

using namespace PVR;
using namespace EPG;
using namespace KODI::MESSAGING;

CCriticalSection CGUIWindowPVRBase::m_selectedItemPathsLock;
std::string CGUIWindowPVRBase::m_selectedItemPaths[2];

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
  CSingleLock lock(m_selectedItemPathsLock);
  m_selectedItemPaths[bRadio] = path;
}

std::string CGUIWindowPVRBase::GetSelectedItemPath(bool bRadio)
{
  CSingleLock lock(m_selectedItemPathsLock);
  return m_selectedItemPaths[bRadio];
}

void CGUIWindowPVRBase::ResetObservers(void)
{
  UnregisterObservers();
  if (IsActive())
    RegisterObservers();
}

void CGUIWindowPVRBase::RegisterObservers(void)
{
  CSingleLock lock(m_critSection);
  if (m_group)
    m_group->RegisterObserver(this);
};

void CGUIWindowPVRBase::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  if (m_group)
    m_group->UnregisterObserver(this);
};

void CGUIWindowPVRBase::Notify(const Observable &obs, const ObservableMessage msg)
{
  CGUIMessage m(GUI_MSG_REFRESH_LIST, GetID(), 0, msg);
  CApplicationMessenger::GetInstance().SendGUIMessage(m);
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
    {
      g_windowManager.ActivateWindow(WINDOW_HOME);
      return true;
    }
    else
      return CGUIWindow::OnBack(actionID);
  }
  return CGUIMediaWindow::OnBack(actionID);
}

void CGUIWindowPVRBase::OnInitWindow(void)
{
  if (!g_PVRManager.IsStarted() || !g_PVRClients->HasCreatedClients())
  {
    return;
  }

  {
    // set window group to playing group
    CPVRChannelGroupPtr group = g_PVRManager.GetPlayingGroup(m_bRadio);
    CSingleLock lock(m_critSection);
    if (m_group != group)
      m_viewControl.SetSelectedItem(0);
    m_group = group;
  }
  SetProperty("IsRadio", m_bRadio ? "true" : "");

  m_vecItems->SetPath(GetDirectoryPath());

  CGUIMediaWindow::OnInitWindow();

  // mark item as selected by channel path
  m_viewControl.SetSelectedItem(GetSelectedItemPath(m_bRadio));

  RegisterObservers();
}

void CGUIWindowPVRBase::OnDeinitWindow(int nextWindowID)
{
  UnregisterObservers();
  UpdateSelectedItemPath();
  CGUIMediaWindow::OnDeinitWindow(nextWindowID);
}

bool CGUIWindowPVRBase::OnMessage(CGUIMessage& message)
{
  bool bReturn = false;
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      switch (message.GetSenderId())
      {
        case CONTROL_BTNCHANNELGROUPS:
          return OpenGroupSelectionDialog();
      }
    }
    break;

    case GUI_MSG_REFRESH_LIST:
    {
      if (IsActive())
      {
        // Only the active window must set the selected item path which is shared
        // between all PVR windows, not the last notified window (observer).
        UpdateSelectedItemPath();
      }
      bReturn = true;
    }
    break;
  }

  return bReturn || CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowPVRBase::IsValidMessage(CGUIMessage& message)
{
  bool bReturn = false;

  // we need to protect the pvr windows against certain messages
  // if the pvr manager is not started yet. we only want to support
  // that the window can be loaded to show the user an info about
  // the manager startup. Any other interactions with the windows
  // would cause access violations.
  switch (message.GetMessage())
  {
    // valid messages
    case GUI_MSG_WINDOW_LOAD:
    case GUI_MSG_WINDOW_INIT:
    case GUI_MSG_WINDOW_DEINIT:
      bReturn = true;
      break;
    default:
      if (g_PVRManager.IsStarted())
        bReturn = true;
      break;
  }

  return bReturn;
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

bool CGUIWindowPVRBase::OnContextButtonActiveAEDSPSettings(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_ACTIVE_ADSP_SETTINGS)
  {
    bReturn = true;

    if (CServiceBroker::GetADSP().IsProcessing())
      g_windowManager.ActivateWindow(WINDOW_DIALOG_AUDIO_DSP_OSD_SETTINGS);
  }

  return bReturn;
}

bool CGUIWindowPVRBase::OnContextButtonEditTimer(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_EDIT_TIMER)
  {
    EditTimer(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRBase::OnContextButtonEditTimerRule(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_EDIT_TIMER_RULE)
  {
    EditTimerRule(item);
    bReturn = true;
  }

  return bReturn;
}

bool CGUIWindowPVRBase::OnContextButtonDeleteTimerRule(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_DELETE_TIMER_RULE)
  {
    CFileItemPtr parentTimer(g_PVRTimers->GetTimerRule(item));
    if (parentTimer)
      DeleteTimerRule(parentTimer.get());

    bReturn = true;
  }

  return bReturn;
}

void CGUIWindowPVRBase::SetInvalid()
{
  if (m_refreshTimeout.IsTimePast())
  {
    VECFILEITEMS items = m_vecItems->GetList();
    for (VECFILEITEMS::iterator it = items.begin(); it != items.end(); ++it)
      (*it)->SetInvalid();
    CGUIMediaWindow::SetInvalid();
    m_refreshTimeout.Set(MAX_INVALIDATION_FREQUENCY);
  }
}

bool CGUIWindowPVRBase::CanBeActivated() const
{
  // No activation if PVR is not enabled.
  return g_PVRManager.IsStarted();
}

bool CGUIWindowPVRBase::OpenGroupSelectionDialog(void)
{
  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!dialog)
    return false;

  CFileItemList options;
  g_PVRChannelGroups->Get(m_bRadio)->GetGroupList(&options, true);

  dialog->Reset();
  dialog->SetHeading(CVariant{g_localizeStrings.Get(19146)});
  dialog->SetItems(options);
  dialog->SetMultiSelection(false);
  dialog->SetSelected(m_group->GroupName());
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  const CFileItemPtr item = dialog->GetSelectedFileItem();
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

void CGUIWindowPVRBase::SetGroup(const CPVRChannelGroupPtr &group)
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

  CMediaSettings::GetInstance().SetVideoStartWindowed(bPlayMinimized);

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
          pDialog->SetHeading(CVariant{19687}); // Play recording
          pDialog->SetLine(0, CVariant{""});
          pDialog->SetLine(1, CVariant{12021}); // Start from beginning
          pDialog->SetLine(2, CVariant{recording->m_strTitle});
          pDialog->Open();

          if (pDialog->IsConfirmed())
          {
            CFileItem recordingItem(recording);
            return PlayRecording(&recordingItem, CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPLAYBACK_PLAYMINIMIZED), bCheckResume);
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
        CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(*item)));
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

bool CGUIWindowPVRBase::ShowTimerSettings(const CPVRTimerInfoTagPtr &timer)
{
  CGUIDialogPVRTimerSettings* pDlgInfo = (CGUIDialogPVRTimerSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);

  if (!pDlgInfo)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - unable to get WINDOW_DIALOG_PVR_TIMER_SETTING!", __FUNCTION__);
    return false;
  }

  pDlgInfo->SetTimer(timer);
  pDlgInfo->Open();

  return pDlgInfo->IsConfirmed();
}

bool CGUIWindowPVRBase::AddTimer(CFileItem *item, bool bShowTimerSettings)
{
  return AddTimer(item, false, bShowTimerSettings);
}

bool CGUIWindowPVRBase::AddTimerRule(CFileItem *item, bool bShowTimerSettings)
{
  return AddTimer(item, true, bShowTimerSettings);
}

bool CGUIWindowPVRBase::AddTimer(CFileItem *item, bool bCreateRule, bool bShowTimerSettings)
{
  CEpgInfoTagPtr epgTag;
  CPVRChannelPtr channel;

  if (item->IsEPG())
  {
    epgTag  = item->GetEPGInfoTag();
    channel = epgTag->ChannelTag();
  }
  else if (item->IsPVRChannel())
  {
    channel = item->GetPVRChannelInfoTag();
    epgTag  = channel->GetEPGNow();
  }

  if (!channel)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - no channel!", __FUNCTION__);
    return false;
  }

  if (!g_PVRManager.CheckParentalLock(channel))
    return false;

  if (!epgTag && bCreateRule)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - no epg tag!", __FUNCTION__);
    return false;
  }

  CPVRTimerInfoTagPtr timer(bCreateRule || !epgTag ? nullptr : epgTag->Timer());
  CPVRTimerInfoTagPtr rule (bCreateRule ? g_PVRTimers->GetTimerRule(timer) : nullptr);
  if (timer || rule)
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19034}); // "Information", "There is already a timer set for this event"
    return false;
  }

  CPVRTimerInfoTagPtr newTimer(epgTag ? CPVRTimerInfoTag::CreateFromEpg(epgTag, bCreateRule) : CPVRTimerInfoTag::CreateInstantTimerTag(channel));
  if (!newTimer)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - unable to create timer for epg tag!", __FUNCTION__);
    return false;
  }

  if (bShowTimerSettings)
  {
    if (!ShowTimerSettings(newTimer))
      return false;
  }

  return g_PVRTimers->AddTimer(newTimer);
}

bool CGUIWindowPVRBase::EditTimer(CFileItem *item)
{
  CPVRTimerInfoTagPtr timer;

  if (item->IsPVRTimer())
  {
    timer = item->GetPVRTimerInfoTag();
  }
  else if (item->IsEPG())
  {
    timer = item->GetEPGInfoTag()->Timer();
  }

  if (!timer)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - no timer!", __FUNCTION__);
    return false;
  }

  // clone the timer.
  const CPVRTimerInfoTagPtr newTimer(new CPVRTimerInfoTag);
  newTimer->UpdateEntry(timer);

  if (ShowTimerSettings(newTimer) && !timer->GetTimerType()->IsReadOnly())
  {
    if (newTimer->GetTimerType() == timer->GetTimerType())
    {
      return g_PVRTimers->UpdateTimer(newTimer);
    }
    else
    {
      // timer type changed. delete the original timer, then create the new timer. this order is
      // important. for instance, the new timer might be a rule which schedules the original timer.
      // deleting the original timer after creating the rule would do literally this and we would
      // end up with one timer missing wrt to the rule defined by the new timer.
      if (g_PVRTimers->DeleteTimer(timer, timer->IsRecording(), false))
      {
        if (g_PVRTimers->AddTimer(newTimer))
          return true;

        // rollback.
        return g_PVRTimers->AddTimer(timer);
      }
    }
  }
  return false;
}

bool CGUIWindowPVRBase::EditTimerRule(CFileItem *item)
{
  CFileItemPtr parentTimer(g_PVRTimers->GetTimerRule(item));
  if (parentTimer)
    return EditTimer(parentTimer.get());

  return false;
}

bool CGUIWindowPVRBase::DeleteTimer(CFileItem *item)
{
  return DeleteTimer(item, false, false);
}

bool CGUIWindowPVRBase::StopRecordFile(CFileItem *item)
{
  return DeleteTimer(item, true, false);
}

bool CGUIWindowPVRBase::DeleteTimerRule(CFileItem *item)
{
  return DeleteTimer(item, false, true);
}

bool CGUIWindowPVRBase::DeleteTimer(CFileItem *item, bool bIsRecording, bool bDeleteRule)
{
  CPVRTimerInfoTagPtr timer;

  if (item->IsPVRTimer())
  {
    timer = item->GetPVRTimerInfoTag();
  }
  else if (item->IsEPG())
  {
    timer = item->GetEPGInfoTag()->Timer();
  }
  else if (item->IsPVRChannel())
  {
    const CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
    if (epgTag)
      timer = epgTag->Timer(); // cheap method, but not reliable as timers get set at epg tags asychrounously

    if (!timer)
      timer = g_PVRTimers->GetActiveTimerForChannel(item->GetPVRChannelInfoTag()); // more expensive, but reliable and works even for channels with no epg data
  }

  if (!timer)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - no timer!", __FUNCTION__);
    return false;
  }

  if (bDeleteRule && !timer->IsTimerRule())
    timer = g_PVRTimers->GetTimerRule(timer);

  if (!timer)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - no timer rule!", __FUNCTION__);
    return false;
  }

  if (bIsRecording)
  {
    if (ConfirmStopRecording(timer))
      return g_PVRTimers->DeleteTimer(timer, true, false);
  }
  else if (timer->HasTimerType() && timer->GetTimerType()->IsReadOnly())
  {
    return false;
  }
  else
  {
    bool bAlsoDeleteRule(false);
    if (ConfirmDeleteTimer(timer, bAlsoDeleteRule))
      return g_PVRTimers->DeleteTimer(timer, false, bAlsoDeleteRule);
  }
  return false;
}

bool CGUIWindowPVRBase::CheckResumeRecording(CFileItem *item)
{
  bool bPlayIt(true);
  std::string resumeString = CGUIWindowPVRRecordings::GetResumeString(*item);
  if (!resumeString.empty())
  {
    CContextButtons choices;
    choices.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
    choices.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); // Start from beginning
    int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (choice > 0)
      item->m_lStartOffset = choice == CONTEXT_BUTTON_RESUME_ITEM ? STARTOFFSET_RESUME : 0;
    else
      bPlayIt = false; // context menu cancelled
  }
  return bPlayIt;
}

bool CGUIWindowPVRBase::PlayRecording(CFileItem *item, bool bPlayMinimized /* = false */, bool bCheckResume /* = true */)
{
  if (!item->HasPVRRecordingInfoTag())
    return false;

  std::string stream = item->GetPVRRecordingInfoTag()->m_strStreamURL;
  if (stream.empty())
  {
    if (!bCheckResume || CheckResumeRecording(item))
      CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(*item)));

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
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - can't open recording: no valid filename", __FUNCTION__);
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19036});
    return false;
  }

  if (!bCheckResume || CheckResumeRecording(item))
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(*item)));

  return true;
}

void CGUIWindowPVRBase::ShowRecordingInfo(CFileItem *item)
{
  CGUIDialogPVRRecordingInfo* pDlgInfo = (CGUIDialogPVRRecordingInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_RECORDING_INFO);
  if (item->IsPVRRecording() && pDlgInfo)
  {
    pDlgInfo->SetRecording(item);
    pDlgInfo->Open();
  }
}

void CGUIWindowPVRBase::ShowEPGInfo(CFileItem *item)
{
  CEpgInfoTagPtr epgTag;
  CPVRChannelPtr channel;

  if (item->IsEPG())
  {
    epgTag  = item->GetEPGInfoTag();
    channel = epgTag->ChannelTag();
  }
  else if (item->IsPVRChannel())
  {
    channel = item->GetPVRChannelInfoTag();
    epgTag  = channel->GetEPGNow();
  }
  else if (item->IsPVRTimer())
  {
    epgTag = item->GetPVRTimerInfoTag()->GetEpgInfoTag();
    if (epgTag && epgTag->HasPVRChannel())
      channel = epgTag->ChannelTag();
  }

  if (channel && !g_PVRManager.CheckParentalLock(channel))
    return;

  if (!epgTag)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - no epg tag!", __FUNCTION__);
    return;
  }

  CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
  if (!pDlgInfo)
  {
    CLog::Log(LOGERROR, "CGUIWindowPVRBase - %s - unable to get WINDOW_DIALOG_PVR_GUIDE_INFO!", __FUNCTION__);
    return;
  }

  pDlgInfo->SetProgInfo(epgTag);
  pDlgInfo->Open();
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
        ++itemIndex;
      }
    }
  }

  return false;
}

bool CGUIWindowPVRBase::ActionShowTimerRule(CFileItem *item)
{
  if (!g_PVRTimers->GetTimerRule(item))
    return AddTimerRule(item, true);
  else
    return EditTimerRule(item);
}

bool CGUIWindowPVRBase::ActionToggleTimer(CFileItem *item)
{
  if (!item->HasEPGInfoTag())
    return false;

  CPVRTimerInfoTagPtr timer(item->GetEPGInfoTag()->Timer());
  if (timer)
    return DeleteTimer(item, timer->IsRecording(), false);
  else
    return AddTimer(item, false, false);
}

bool CGUIWindowPVRBase::ActionPlayChannel(CFileItem *item)
{
  return PlayFile(item, CSettings::GetInstance().GetBool(CSettings::SETTING_PVRPLAYBACK_PLAYMINIMIZED));
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
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{std::move(msg)});
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
  pDialog->SetHeading(CVariant{19039});
  pDialog->SetLine(0, CVariant{""});
  pDialog->SetLine(1, CVariant{channel->ChannelName()});
  pDialog->SetLine(2, CVariant{""});
  pDialog->Open();

  /* prompt for the user's confirmation */
  if (!pDialog->IsConfirmed())
    return false;

  g_PVRChannelGroups->GetGroupAll(channel->IsRadio())->RemoveFromGroup(channel);
  Refresh(true);

  return true;
}

bool CGUIWindowPVRBase::UpdateEpgForChannel(CFileItem *item)
{
  CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

  CEpgPtr epg = channel->GetEPG();
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
  if (!m_viewControl.GetSelectedItemPath().empty())
  {
    CSingleLock lock(m_selectedItemPathsLock);
    m_selectedItemPaths[m_bRadio] = m_viewControl.GetSelectedItemPath();
  }
}

bool CGUIWindowPVRBase::ConfirmDeleteTimer(const CPVRTimerInfoTagPtr &timer, bool &bDeleteRule)
{
  bool bConfirmed(false);

  if (timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT)
  {
    // timer was scheduled by a timer rule. prompt user for confirmation for deleting the timer rule, including scheduled timers.
    bool bCancel(false);
    bDeleteRule = CGUIDialogYesNo::ShowAndGetInput(
                        CVariant{122}, // "Confirm delete"
                        CVariant{840}, // "Do you want to delete only this timer or also the timer rule that has scheduled it?"
                        CVariant{""},
                        CVariant{timer->Title()},
                        bCancel,
                        CVariant{841}, // "Only this"
                        CVariant{593}, // "All"
                        0); // no autoclose
    bConfirmed = !bCancel;
  }
  else
  {
    bDeleteRule = false;

    // prompt user for confirmation for deleting the timer
    bConfirmed = CGUIDialogYesNo::ShowAndGetInput(
                        CVariant{122}, // "Confirm delete"
                        timer->IsTimerRule()
                          ? CVariant{845}  // "Are you sure you want to delete this timer rule and all timers it has scheduled?"
                          : CVariant{846}, // "Are you sure you want to delete this timer?"
                        CVariant{""},
                        CVariant{timer->Title()});
  }

  return bConfirmed;
}

bool CGUIWindowPVRBase::ConfirmStopRecording(const CPVRTimerInfoTagPtr &timer)
{
  return CGUIDialogYesNo::ShowAndGetInput(
                         CVariant{847}, // "Confirm stop recording"
                         CVariant{848}, // "Are you sure you want to stop this recording?"
                         CVariant{""},
                         CVariant{timer->Title()});
}
