/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "epg/EpgInfoTag.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/windows/GUIWindowPVRSearch.h"
#include "ServiceBroker.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include "PVRGUIActions.h"

using namespace EPG;
using namespace KODI::MESSAGING;

namespace PVR
{
  CPVRGUIActions& CPVRGUIActions::GetInstance()
  {
    static CPVRGUIActions instance;
    return instance;
  }

  bool CPVRGUIActions::ShowEPGInfo(const CFileItemPtr &item) const
  {
    const CPVRChannelPtr channel(CPVRItem(item).GetChannel());
    if (channel && !g_PVRManager.CheckParentalLock(channel))
      return false;

    const CEpgInfoTagPtr epgTag(CPVRItem(item).GetEpgInfoTag());
    if (!epgTag)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no epg tag!", __FUNCTION__);
      return false;
    }

    CGUIDialogPVRGuideInfo* pDlgInfo = dynamic_cast<CGUIDialogPVRGuideInfo*>(g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO));
    if (!pDlgInfo)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_PVR_GUIDE_INFO!", __FUNCTION__);
      return false;
    }

    pDlgInfo->SetProgInfo(epgTag);
    pDlgInfo->Open();
    return true;
  }

  bool CPVRGUIActions::ShowRecordingInfo(const CFileItemPtr &item) const
  {
    if (!item->IsPVRRecording())
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no recording!", __FUNCTION__);
      return false;
    }

    CGUIDialogPVRRecordingInfo* pDlgInfo = dynamic_cast<CGUIDialogPVRRecordingInfo*>(g_windowManager.GetWindow(WINDOW_DIALOG_PVR_RECORDING_INFO));
    if (!pDlgInfo)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_PVR_RECORDING_INFO!", __FUNCTION__);
      return false;
    }

    pDlgInfo->SetRecording(item.get());
    pDlgInfo->Open();
    return true;
  }

  bool CPVRGUIActions::FindSimilar(const CFileItemPtr &item, CGUIWindow *windowToClose /* = nullptr */) const
  {
    const bool bRadio(CPVRItem(item).IsRadio());

    int windowSearchId = bRadio ? WINDOW_RADIO_SEARCH : WINDOW_TV_SEARCH;
    CGUIWindowPVRSearch *windowSearch = dynamic_cast<CGUIWindowPVRSearch*>(g_windowManager.GetWindow(windowSearchId));
    if (!windowSearch)
    {
      CLog::Log(LOGERROR, "PVRGUIActions - %s - unable to get %s!", __FUNCTION__, bRadio ? "WINDOW_RADIO_SEARCH" : "WINDOW_TV_SEARCH");
      return false;
    }

    if (windowToClose)
      windowToClose->Close();

    windowSearch->SetItemToSearch(item);
    g_windowManager.ActivateWindow(windowSearchId);
    return true;
  };

  bool CPVRGUIActions::ShowTimerSettings(const CPVRTimerInfoTagPtr &timer) const
  {
    CGUIDialogPVRTimerSettings* pDlgInfo = dynamic_cast<CGUIDialogPVRTimerSettings*>(g_windowManager.GetWindow(WINDOW_DIALOG_PVR_TIMER_SETTING));
    if (!pDlgInfo)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_PVR_TIMER_SETTING!", __FUNCTION__);
      return false;
    }

    pDlgInfo->SetTimer(timer);
    pDlgInfo->Open();

    return pDlgInfo->IsConfirmed();
  }

  bool CPVRGUIActions::AddTimer(bool bRadio) const
  {
    const CPVRTimerInfoTagPtr newTimer(new CPVRTimerInfoTag(bRadio));
    if (ShowTimerSettings(newTimer))
    {
      /* Add timer to backend */
      return g_PVRTimers->AddTimer(newTimer);
    }
    return false;
  }

  bool CPVRGUIActions::AddTimer(const CFileItemPtr &item, bool bShowTimerSettings) const
  {
    return AddTimer(item, false, bShowTimerSettings);
  }

  bool CPVRGUIActions::AddTimerRule(const CFileItemPtr &item, bool bShowTimerSettings) const
  {
    return AddTimer(item, true, bShowTimerSettings);
  }

  bool CPVRGUIActions::AddTimer(const CFileItemPtr &item, bool bCreateRule, bool bShowTimerSettings) const
  {
    const CPVRChannelPtr channel(CPVRItem(item).GetChannel());
    if (!channel)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no channel!", __FUNCTION__);
      return false;
    }

    if (!g_PVRManager.CheckParentalLock(channel))
      return false;

    const CEpgInfoTagPtr epgTag(CPVRItem(item).GetEpgInfoTag());
    if (!epgTag && bCreateRule)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no epg tag!", __FUNCTION__);
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
      CGUIDialogOK::ShowAndGetInput(CVariant{19033},
                                    bCreateRule
                                      ? CVariant{19095} // "Information", "Timer rule creation failed. The PVR add-on does not support a suitable timer rule type."
                                      : CVariant{19094}); // "Information", "Timer creation failed. The PVR add-on does not support a suitable timer type."
      return false;
    }

    if (bShowTimerSettings)
    {
      if (!ShowTimerSettings(newTimer))
        return false;
    }

    return g_PVRTimers->AddTimer(newTimer);
  }

  bool CPVRGUIActions::ToggleTimer(const CFileItemPtr &item) const
  {
    if (!item->HasEPGInfoTag())
      return false;

    const CPVRTimerInfoTagPtr timer(CPVRItem(item).GetTimerInfoTag());
    if (timer)
    {
      if (timer->IsRecording())
        return StopRecording(item);
      else
        return DeleteTimer(item);
    }
    else
      return AddTimer(item, false);
  }

  bool CPVRGUIActions::ToggleTimerState(const CFileItemPtr &item) const
  {
    if (!item->HasPVRTimerInfoTag())
      return false;

    const CPVRTimerInfoTagPtr timer(item->GetPVRTimerInfoTag());
    if (timer->m_state == PVR_TIMER_STATE_DISABLED)
      timer->m_state = PVR_TIMER_STATE_SCHEDULED;
    else
      timer->m_state = PVR_TIMER_STATE_DISABLED;

    return g_PVRTimers->UpdateTimer(timer);
  }

  bool CPVRGUIActions::EditTimer(const CFileItemPtr &item) const
  {
    const CPVRTimerInfoTagPtr timer(CPVRItem(item).GetTimerInfoTag());
    if (!timer)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no timer!", __FUNCTION__);
      return false;
    }

    // clone the timer.
    const CPVRTimerInfoTagPtr newTimer(new CPVRTimerInfoTag);
    newTimer->UpdateEntry(timer);

    if (ShowTimerSettings(newTimer) && (!timer->GetTimerType()->IsReadOnly() || timer->GetTimerType()->SupportsEnableDisable()))
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

  bool CPVRGUIActions::EditTimerRule(const CFileItemPtr &item) const
  {
    CFileItemPtr parentTimer(g_PVRTimers->GetTimerRule(item.get()));
    if (parentTimer)
      return EditTimer(parentTimer);

    return false;
  }

  bool CPVRGUIActions::RenameTimer(const CFileItemPtr &item) const
  {
    if (!item->HasPVRTimerInfoTag())
      return false;

    const CPVRTimerInfoTagPtr timer(item->GetPVRTimerInfoTag());

    std::string strNewName(timer->m_strTitle);
    if (CGUIKeyboardFactory::ShowAndGetInput(strNewName,
                                             CVariant{g_localizeStrings.Get(19042)}, // "Are you sure you want to rename this timer?"
                                             false))
    {
      if (!g_PVRTimers->RenameTimer(*item, strNewName))
        return false;
    }

    CGUIWindowPVRBase *pvrWindow = dynamic_cast<CGUIWindowPVRBase *>(g_windowManager.GetWindow(g_windowManager.GetActiveWindow()));
    if (pvrWindow)
      pvrWindow->DoRefresh();
    else
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - called on non-pvr window. no refresh possible.", __FUNCTION__);

    return true;
  }

  bool CPVRGUIActions::DeleteTimer(const CFileItemPtr &item) const
  {
    return DeleteTimer(item, false, false);
  }

  bool CPVRGUIActions::DeleteTimerRule(const CFileItemPtr &item) const
  {
    return DeleteTimer(item, false, true);
  }

  bool CPVRGUIActions::DeleteTimer(const CFileItemPtr &item, bool bIsRecording, bool bDeleteRule) const
  {
    CPVRTimerInfoTagPtr timer(CPVRItem(item).GetTimerInfoTag());
    if (!timer)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no timer!", __FUNCTION__);
      return false;
    }

    if (bDeleteRule && !timer->IsTimerRule())
      timer = g_PVRTimers->GetTimerRule(timer);

    if (!timer)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no timer rule!", __FUNCTION__);
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

  bool CPVRGUIActions::ConfirmDeleteTimer(const CPVRTimerInfoTagPtr &timer, bool &bDeleteRule) const
  {
    bool bConfirmed(false);

    if (timer->GetTimerRuleId() != PVR_TIMER_NO_PARENT)
    {
      // timer was scheduled by a timer rule. prompt user for confirmation for deleting the timer rule, including scheduled timers.
      bool bCancel(false);
      bDeleteRule = CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, // "Confirm delete"
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
      bConfirmed = CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, // "Confirm delete"
                                                    timer->IsTimerRule()
                                                      ? CVariant{845}  // "Are you sure you want to delete this timer rule and all timers it has scheduled?"
                                                      : CVariant{846}, // "Are you sure you want to delete this timer?"
                                                    CVariant{""},
                                                    CVariant{timer->Title()});
    }

    return bConfirmed;
  }

  bool CPVRGUIActions::StopRecording(const CFileItemPtr &item) const
  {
    if (!DeleteTimer(item, true, false))
      return false;

    g_PVRManager.TriggerRecordingsUpdate();
    return true;
  }

  bool CPVRGUIActions::ConfirmStopRecording(const CPVRTimerInfoTagPtr &timer) const
  {
    return CGUIDialogYesNo::ShowAndGetInput(CVariant{847}, // "Confirm stop recording"
                                            CVariant{848}, // "Are you sure you want to stop this recording?"
                                            CVariant{""},
                                            CVariant{timer->Title()});
  }

  bool CPVRGUIActions::RenameRecording(const CFileItemPtr &item) const
  {
    const CPVRRecordingPtr recording(item->GetPVRRecordingInfoTag());
    if (!recording)
      return false;

    std::string strNewName(recording->m_strTitle);
    if (!CGUIKeyboardFactory::ShowAndGetInput(strNewName, CVariant{g_localizeStrings.Get(19041)}, false))
      return false;

    if (!g_PVRRecordings->RenameRecording(*item, strNewName))
      return false;

    g_PVRManager.TriggerRecordingsUpdate();
    return true;
  }

  bool CPVRGUIActions::DeleteRecording(const CFileItemPtr &item) const
  {
    if ((!item->IsPVRRecording() && !item->m_bIsFolder) || item->IsParentFolder())
      return false;

    if (!ConfirmDeleteRecording(item))
      return false;

    if (!g_PVRRecordings->Delete(*item))
      return false;

    g_PVRManager.TriggerRecordingsUpdate();
    return true;
  }

  bool CPVRGUIActions::ConfirmDeleteRecording(const CFileItemPtr &item) const
  {
    return CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, // "Confirm delete"
                                            item->m_bIsFolder
                                              ? CVariant{19113} // "Delete all recordings in this folder?"
                                              : item->GetPVRRecordingInfoTag()->IsDeleted()
                                                ? CVariant{19294}  // "Remove this deleted recording from trash? This operation cannot be reverted."
                                                : CVariant{19112}, // "Delete this recording?"
                                            CVariant{""},
                                            CVariant{item->GetLabel()});
  }

  bool CPVRGUIActions::DeleteAllRecordingsFromTrash() const
  {
    if (!ConfirmDeleteAllRecordingsFromTrash())
      return false;

    if (!g_PVRRecordings->DeleteAllRecordingsFromTrash())
      return false;

    g_PVRManager.TriggerRecordingsUpdate();
    return true;
  }

  bool CPVRGUIActions::ConfirmDeleteAllRecordingsFromTrash() const
  {
    return CGUIDialogYesNo::ShowAndGetInput(CVariant{19292},  // "Delete all permanently"
                                            CVariant{19293}); // "Remove all deleted recordings from trash? This operation cannot be reverted."
  }

  bool CPVRGUIActions::UndeleteRecording(const CFileItemPtr &item) const
  {
    if (!item->IsDeletedPVRRecording())
      return false;

    /* undelete the recording */
    if (!g_PVRRecordings->Undelete(*item))
      return false;

    g_PVRManager.TriggerRecordingsUpdate();
    return true;
  }

  std::string CPVRGUIActions::GetResumeLabel(const CFileItem &item) const
  {
    std::string resumeString;

    const CPVRRecordingPtr recording(CPVRItem(CFileItemPtr(new CFileItem(item))).GetRecording());
    if (recording && !recording->IsDeleted())
    {
      int positionInSeconds = lrint(recording->GetResumePoint().timeInSeconds);
      if (positionInSeconds > 0)
        resumeString = StringUtils::Format(g_localizeStrings.Get(12022).c_str(),
                                           StringUtils::SecondsToTimeString(positionInSeconds, TIME_FORMAT_HH_MM_SS).c_str());
    }
    return resumeString;
  }

  bool CPVRGUIActions::CheckResumeRecording(const CFileItemPtr &item) const
  {
    bool bPlayIt(true);
    std::string resumeString(GetResumeLabel(*item));
    if (!resumeString.empty())
    {
      CContextButtons choices;
      choices.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
      choices.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); // Play from beginning
      int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (choice > 0)
        item->m_lStartOffset = choice == CONTEXT_BUTTON_RESUME_ITEM ? STARTOFFSET_RESUME : 0;
      else
        bPlayIt = false; // context menu cancelled
    }
    return bPlayIt;
  }

  bool CPVRGUIActions::ResumePlayRecording(const CFileItemPtr &item, bool bFallbackToPlay) const
  {
    bool bCanResume = !GetResumeLabel(*item).empty();
    if (bCanResume)
    {
      item->m_lStartOffset = STARTOFFSET_RESUME;
    }
    else
    {
      if (bFallbackToPlay)
        item->m_lStartOffset = 0;
      else
        return false;
    }

    return PlayRecording(item, false);
  }

  void CPVRGUIActions::CheckAndSwitchToFullscreen() const
  {
    const bool bFullscreen(CServiceBroker::GetSettings().GetBool(CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREEN));
    CMediaSettings::GetInstance().SetVideoStartWindowed(!bFullscreen);

    if (bFullscreen)
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
    }
  }

  bool CPVRGUIActions::TryFastChannelSwitch(const CPVRChannelPtr &channel) const
  {
    bool bSwitchSuccessful(false);

    if (channel->StreamURL().empty() &&
        (g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio()) &&
        (channel->IsRadio() == g_PVRManager.IsPlayingRadio()))
    {
      bSwitchSuccessful = g_application.m_pPlayer->SwitchChannel(channel);

      if (bSwitchSuccessful)
        CheckAndSwitchToFullscreen();
    }

    return bSwitchSuccessful;
  }

  void CPVRGUIActions::StartPlayback(CFileItem *item) const
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
    CheckAndSwitchToFullscreen();
  }

  bool CPVRGUIActions::PlayRecording(const CFileItemPtr &item, bool bCheckResume) const
  {
    const CPVRRecordingPtr recording(CPVRItem(item).GetRecording());
    if (!recording)
      return false;

    if (g_PVRManager.IsPlayingRecording(recording))
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
      return true;
    }

    std::string stream = recording->m_strStreamURL;
    if (stream.empty())
    {
      if (!bCheckResume || CheckResumeRecording(item))
      {
        CFileItem *itemToPlay = new CFileItem(recording);
        itemToPlay->m_lStartOffset = item->m_lStartOffset;
        StartPlayback(itemToPlay);
      }
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
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - can't open recording: no valid filename", __FUNCTION__);
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19036});
      return false;
    }

    if (!bCheckResume || CheckResumeRecording(item))
      StartPlayback(new CFileItem(*item));

    return true;
  }

  bool CPVRGUIActions::SwitchToChannel(const CFileItemPtr &item, bool bCheckResume) const
  {
    if (item->m_bIsFolder)
      return false;

    const CPVRChannelPtr channel(CPVRItem(item).GetChannel());
    if ((channel && g_PVRManager.IsPlayingChannel(channel)) ||
        (channel && channel->HasRecording() && g_PVRManager.IsPlayingRecording(channel->GetRecording())))
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
      return true;
    }

    // switch to channel or if recording present, ask whether to switch or play recording...
    bool bSwitchSuccessful(false);

    if (channel && g_PVRManager.CheckParentalLock(channel))
    {
      const CPVRRecordingPtr recording(channel->GetRecording());
      if (recording)
      {
        bool bCancel(false);
        bool bPlayRecording = CGUIDialogYesNo::ShowAndGetInput(CVariant{19687}, // "Play recording"
                                                       CVariant{""},
                                                       CVariant{12021}, // "Play from beginning"
                                                       CVariant{recording->m_strTitle},
                                                       bCancel,
                                                       CVariant{19000}, // "Switch to channel"
                                                       CVariant{19687}, // "Play recording"
                                                       0); // no autoclose
        if (bCancel)
          return false;

        if (bPlayRecording)
        {
          const CFileItemPtr recordingItem(new CFileItem(recording));
          return PlayRecording(recordingItem, bCheckResume);
        }
      }

      /* optimization: try a fast switch */
      bSwitchSuccessful = TryFastChannelSwitch(channel);

      if (!bSwitchSuccessful)
      {
        StartPlayback(new CFileItem(channel));
        return true;
      }
    }

    if (!bSwitchSuccessful)
    {
      std::string channelName = g_localizeStrings.Get(19029); // Channel
      if (channel)
        channelName = channel->ChannelName();
      std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), channelName.c_str()); // CHANNELNAME could not be played. Check the log for details.

      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(19166), msg); // PVR information
      return false;
    }

    return true;
  }

  bool CPVRGUIActions::PlayMedia(const CFileItemPtr &item) const
  {
    CFileItemPtr pvrItem(item);
    if (URIUtils::IsPVRChannel(item->GetPath()) && !item->HasPVRChannelInfoTag())
      pvrItem = g_PVRChannelGroups->GetByPath(item->GetPath());
    else if (URIUtils::IsPVRRecording(item->GetPath()) && !item->HasPVRRecordingInfoTag())
      pvrItem = g_PVRRecordings->GetByPath(item->GetPath());

    bool bCheckResume = true;
    if (item->HasProperty("check_resume"))
      bCheckResume = item->GetProperty("check_resume").asBoolean();

    if (pvrItem->HasPVRChannelInfoTag())
    {
      return SwitchToChannel(pvrItem, bCheckResume);
    }
    else if (pvrItem->HasPVRRecordingInfoTag())
    {
      return PlayRecording(pvrItem, bCheckResume);
    }

    return false;
  }

  bool CPVRGUIActions::HideChannel(const CFileItemPtr &item) const
  {
    const CPVRChannelPtr channel(item->GetPVRChannelInfoTag());

    /* check if the channel tag is valid */
    if (!channel || channel->ChannelNumber() <= 0)
      return false;

    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{19054}, // "Hide channel"
                                          CVariant{19039}, // "Are you sure you want to hide this channel?"
                                          CVariant{""},
                                          CVariant{channel->ChannelName()}))
      return false;

    if (!g_PVRChannelGroups->GetGroupAll(channel->IsRadio())->RemoveFromGroup(channel))
      return false;

    CGUIWindowPVRBase *pvrWindow = dynamic_cast<CGUIWindowPVRBase *>(g_windowManager.GetWindow(g_windowManager.GetActiveWindow()));
    if (pvrWindow)
      pvrWindow->DoRefresh();
    else
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - called on non-pvr window. no refresh possible.", __FUNCTION__);

    return true;
  }

  CPVRChannelNumberInputHandler &CPVRGUIActions::GetChannelNumberInputHandler()
  {
    // window/dialog specific input handler
    CPVRChannelNumberInputHandler *windowInputHandler
      = dynamic_cast<CPVRChannelNumberInputHandler *>(g_windowManager.GetWindow(g_windowManager.GetFocusedWindow()));
    if (windowInputHandler)
      return *windowInputHandler;

    // default
    return m_channelNumberInputHandler;
  }

  void CPVRChannelSwitchingInputHandler::OnInputDone()
  {
    int iChannelNumber;
    bool bSwitchToPreviousChannel;
    {
      CSingleLock lock(m_mutex);
      iChannelNumber = GetChannelNumber();
      // special case. if only a single zero was typed in, switch to previously played channel.
      bSwitchToPreviousChannel = (iChannelNumber == 0 && GetCurrentDigitCount() == 1);
    }

    if (iChannelNumber > 0)
      SwitchToChannel(iChannelNumber);
    else if (bSwitchToPreviousChannel)
      SwitchToPreviousChannel();
  }

  void CPVRChannelSwitchingInputHandler::SwitchToChannel(int iChannelNumber)
  {
    if (iChannelNumber > 0 && g_PVRManager.IsPlaying())
    {
      const CPVRChannelPtr playingChannel(g_PVRManager.GetCurrentChannel());
      if (playingChannel)
      {
        if (iChannelNumber != playingChannel->ChannelNumber())
        {
          const CPVRChannelGroupPtr selectedGroup(g_PVRManager.GetPlayingGroup(playingChannel->IsRadio()));
          const CFileItemPtr channel(selectedGroup->GetByChannelNumber(iChannelNumber));
          if (channel && channel->HasPVRChannelInfoTag())
          {
            CApplicationMessenger::GetInstance().PostMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1,
              static_cast<void*>(new CAction(ACTION_CHANNEL_SWITCH, static_cast<float>(iChannelNumber))));
          }
        }
      }
    }
  }

  void CPVRChannelSwitchingInputHandler::SwitchToPreviousChannel()
  {
    if (g_PVRManager.IsPlaying())
    {
      const CPVRChannelPtr playingChannel(g_PVRManager.GetCurrentChannel());
      if (playingChannel)
      {
        const CPVRChannelGroupPtr group(g_PVRChannelGroups->GetPreviousPlayedGroup());
        if (group)
        {
          g_PVRManager.SetPlayingGroup(group);
          const CFileItemPtr channel(group->GetLastPlayedChannel(playingChannel->ChannelID()));
          if (channel && channel->HasPVRChannelInfoTag())
          {
            CApplicationMessenger::GetInstance().SendMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1,
              static_cast<void*>(new CAction(ACTION_CHANNEL_SWITCH, static_cast<float>(channel->GetPVRChannelInfoTag()->ChannelNumber()))));
          }
        }
      }
    }
  }

} // namespace PVR
