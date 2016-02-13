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

#include "Application.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSP.h"
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
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/timers/PVRTimers.h"

#include "GUIWindowPVRBase.h"
#include "GUIWindowPVRRecordings.h"

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

void CGUIWindowPVRBase::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (IsActive())
  {
    // Only the active window must set the selected item path which is shared
    // between all PVR windows, not the last notified window (observer).
    UpdateSelectedItemPath();
  }

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
  if (!g_PVRManager.IsStarted() || !g_PVRClients->HasConnectedClients())
  {
    // wait until the PVR manager has been started
    CGUIDialogProgress* dialog = static_cast<CGUIDialogProgress*>(g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
    if (dialog)
    {
      dialog->SetHeading(CVariant{19235});
      dialog->SetText(CVariant{19045});
      dialog->ShowProgressBar(false);
      dialog->Open();

      // do not block the gfx context while waiting
      CSingleExit exit(g_graphicsContext);

      CEvent event(true);
      while(!event.WaitMSec(1))
      {
        if (g_PVRManager.IsStarted() && g_PVRClients->HasConnectedClients())
          event.Set();

        if (dialog->IsCanceled())
        {
          // return to previous window if canceled
          dialog->Close();
          g_windowManager.PreviousWindow();
          return;
        }

        g_windowManager.ProcessRenderLoop(false);
      }

      dialog->Close();
    }
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
}

void CGUIWindowPVRBase::OnDeinitWindow(int nextWindowID)
{
  UpdateSelectedItemPath();
}

bool CGUIWindowPVRBase::OnMessage(CGUIMessage& message)
{
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
  }

  return CGUIMediaWindow::OnMessage(message);
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

    if (ActiveAE::CActiveAEDSP::GetInstance().IsProcessing())
      g_windowManager.ActivateWindow(WINDOW_DIALOG_AUDIO_DSP_OSD_SETTINGS);
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
  return CSettings::GetInstance().GetBool(CSettings::SETTING_PVRMANAGER_ENABLED);
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

bool CGUIWindowPVRBase::ShowTimerSettings(CFileItem *item)
{
  if (!item->IsPVRTimer())
    return false;

  CGUIDialogPVRTimerSettings* pDlgInfo = (CGUIDialogPVRTimerSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);

  if (!pDlgInfo)
    return false;

  pDlgInfo->SetTimer(item);
  pDlgInfo->Open();

  return pDlgInfo->IsConfirmed();
}

bool CGUIWindowPVRBase::AddTimer(CFileItem *item, bool bAdvanced)
{
  CFileItemPtr epgTag;
  if (item->IsEPG())
  {
    epgTag.reset(new CFileItem(*item));
    if (!epgTag->GetEPGInfoTag()->HasPVRChannel())
      return false;
  }
  else if (item->IsPVRChannel())
  {
    CPVRChannelPtr channel(item->GetPVRChannelInfoTag());
    if (!channel)
      return false;

    CEpgInfoTagPtr epgNow(channel->GetEPGNow());
    if (!epgNow)
      return false;

    epgTag.reset(new CFileItem(epgNow));
  }

  const CEpgInfoTagPtr tag = epgTag->GetEPGInfoTag();
  CPVRChannelPtr channel = tag->ChannelTag();

  if (!channel || !g_PVRManager.CheckParentalLock(channel))
    return false;

  CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(item);
  if (timer && timer->HasPVRTimerInfoTag())
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19034});
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
    CPVRTimerInfoTagPtr newTimer = CPVRTimerInfoTag::CreateFromEpg(tag);

    if (newTimer)
      bReturn = g_PVRTimers->AddTimer(newTimer);
  }
  return bReturn;
}

bool CGUIWindowPVRBase::DeleteTimer(CFileItem *item)
{
  return DeleteTimer(item, false);
}

bool CGUIWindowPVRBase::StopRecordFile(CFileItem *item)
{
  return DeleteTimer(item, true);
}

bool CGUIWindowPVRBase::DeleteTimer(CFileItem *item, bool bIsRecording)
{
  CFileItemPtr timer;

  if (item->IsPVRTimer())
  {
    timer.reset(new CFileItem(*item));
  }
  else if (item->IsEPG())
  {
    timer = g_PVRTimers->GetTimerForEpgTag(item);
  }
  else if (item->IsPVRChannel())
  {
    CPVRChannelPtr channel(item->GetPVRChannelInfoTag());
    if (!channel)
      return false;

    CFileItemPtr epgNow(new CFileItem(channel->GetEPGNow()));
    timer = g_PVRTimers->GetTimerForEpgTag(epgNow.get());
  }

  if (!timer || !timer->HasPVRTimerInfoTag())
    return false;

  if (bIsRecording)
  {
    if (ConfirmStopRecording(timer.get()))
      return CPVRTimers::DeleteTimer(*timer, true, false);
  }
  else if (timer->GetPVRTimerInfoTag()->HasTimerType() &&
           timer->GetPVRTimerInfoTag()->GetTimerType()->IsReadOnly())
  {
    return false;
  }
  else
  {
    bool bDeleteSchedule(false);
    if (ConfirmDeleteTimer(timer.get(), bDeleteSchedule))
      return CPVRTimers::DeleteTimer(*timer, false, bDeleteSchedule);
  }
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
    CLog::Log(LOGERROR, "CGUIWindowPVRCommon - %s - can't open recording: no valid filename", __FUNCTION__);
    CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19036});
    return false;
  }

  if (bCheckResume)
    CheckResumeRecording(item);
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
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19055});
      return;
    }
    tag = new CFileItem(epgnow);
  }

  CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
  if (tag && (!bHasChannel || g_PVRManager.CheckParentalLock(channel)) && pDlgInfo)
  {
    pDlgInfo->SetProgInfo(tag);
    pDlgInfo->Open();
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

bool CGUIWindowPVRBase::ActionToggleTimer(CFileItem *item)
{
  if (!item->HasEPGInfoTag())
    return false;

  CPVRTimerInfoTagPtr timer(item->GetEPGInfoTag()->Timer());
  if (timer)
    return DeleteTimer(item, timer->IsRecording());
  else
    return AddTimer(item, false);
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
  if (!m_viewControl.GetSelectedItemPath().empty()) {
    CSingleLock lock(m_selectedItemPathsLock);
    m_selectedItemPaths[m_bRadio] = m_viewControl.GetSelectedItemPath();
  }
}

bool CGUIWindowPVRBase::ConfirmDeleteTimer(CFileItem *item, bool &bDeleteSchedule)
{
  bool bConfirmed(false);

  if (item->GetPVRTimerInfoTag()->GetTimerScheduleId() != PVR_TIMER_NO_PARENT)
  {
    // timer was scheduled by a repeating timer. prompt user for confirmation for deleting the complete repeating timer, including scheduled timers.
    bool bCancel(false);
    bDeleteSchedule = CGUIDialogYesNo::ShowAndGetInput(
                        CVariant{122}, // "Confirm delete"
                        CVariant{840}, // "Do you only want to delete this timer or also the repeating timer that has scheduled it?"
                        CVariant{""},
                        CVariant{item->GetPVRTimerInfoTag()->Title()},
                        bCancel,
                        CVariant{841}, // "Only this"
                        CVariant{593}, // "All"
                        0); // no autoclose
    bConfirmed = !bCancel;
  }
  else
  {
    bDeleteSchedule = false;

    // prompt user for confirmation for deleting the timer
    bConfirmed = CGUIDialogYesNo::ShowAndGetInput(
                        CVariant{122}, // "Confirm delete"
                        item->GetPVRTimerInfoTag()->IsRepeating()
                          ? CVariant{845}  // "Are you sure you want to delete this repeating timer and all timers it has scheduled?"
                          : CVariant{846}, // "Are you sure you want to delete this timer?"
                        CVariant{""},
                        CVariant{item->GetPVRTimerInfoTag()->Title()});
  }

  return bConfirmed;
}

bool CGUIWindowPVRBase::ConfirmStopRecording(CFileItem *item)
{
  return CGUIDialogYesNo::ShowAndGetInput(
                         CVariant{847}, // "Confirm stop recording"
                         CVariant{848}, // "Are you sure you want to stop this recording?"
                         CVariant{""},
                         CVariant{item->GetPVRTimerInfoTag()->Title()});
}
