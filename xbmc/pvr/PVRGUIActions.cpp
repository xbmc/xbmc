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
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "epg/EpgContainer.h"
#include "epg/EpgInfoTag.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
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

  CPVRGUIActions::CPVRGUIActions()
  : m_bChannelScanRunning(false)
  {
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

  namespace
  {
    enum PVRRECORD_INSTANTRECORDACTION
    {
      NONE = -1,
      RECORD_CURRENT_SHOW = 0,
      RECORD_INSTANTRECORDTIME = 1,
      ASK = 2,
      RECORD_30_MINUTES = 3,
      RECORD_60_MINUTES = 4,
      RECORD_120_MINUTES = 5,
      RECORD_NEXT_SHOW = 6
    };

    class InstantRecordingActionSelector
    {
    public:
      InstantRecordingActionSelector();
      virtual ~InstantRecordingActionSelector() {}

      void AddAction(PVRRECORD_INSTANTRECORDACTION eAction, const std::string &title);
      void PreSelectAction(PVRRECORD_INSTANTRECORDACTION eAction);
      PVRRECORD_INSTANTRECORDACTION Select();

    private:
      CGUIDialogSelect *m_pDlgSelect; // not owner!
      std::map<PVRRECORD_INSTANTRECORDACTION, int> m_actions;
    };

    InstantRecordingActionSelector::InstantRecordingActionSelector()
    : m_pDlgSelect(dynamic_cast<CGUIDialogSelect *>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT)))
    {
      if (m_pDlgSelect)
      {
        m_pDlgSelect->SetMultiSelection(false);
        m_pDlgSelect->SetHeading(CVariant{19086}); // Instant recording action
      }
      else
      {
        CLog::Log(LOGERROR, "InstantRecordingActionSelector - %s - unable to obtain WINDOW_DIALOG_SELECT instance", __FUNCTION__);
      }
    }

    void InstantRecordingActionSelector::AddAction(PVRRECORD_INSTANTRECORDACTION eAction, const std::string &title)
    {
      if (m_actions.find(eAction) == m_actions.end())
      {
        switch (eAction)
        {
          case RECORD_INSTANTRECORDTIME:
            m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(),
                                                  CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME))); // Record next <default duration> minutes
            break;
          case RECORD_30_MINUTES:
            m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(), 30));  // Record next 30 minutes
            break;
          case RECORD_60_MINUTES:
            m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(), 60));  // Record next 60 minutes
            break;
          case RECORD_120_MINUTES:
            m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(), 120)); // Record next 120 minutes
            break;
          case RECORD_CURRENT_SHOW:
            m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19091).c_str(), title.c_str())); // Record current show (<title>)
            break;
          case RECORD_NEXT_SHOW:
            m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19092).c_str(), title.c_str())); // Record next show (<title>)
            break;
          case NONE:
          case ASK:
          default:
            return;
        }

        m_actions.insert(std::make_pair(eAction, m_actions.size()));
      }
    }

    void InstantRecordingActionSelector::PreSelectAction(PVRRECORD_INSTANTRECORDACTION eAction)
    {
      const auto &it = m_actions.find(eAction);
      if (it != m_actions.end())
        m_pDlgSelect->SetSelected(it->second);
    }

    PVRRECORD_INSTANTRECORDACTION InstantRecordingActionSelector::Select()
    {
      PVRRECORD_INSTANTRECORDACTION eAction = NONE;

      m_pDlgSelect->Open();

      if (m_pDlgSelect->IsConfirmed())
      {
        int iSelection = m_pDlgSelect->GetSelectedItem();
        for (const auto &action : m_actions)
        {
          if (action.second == iSelection)
          {
            eAction = action.first;
            break;
          }
        }
      }

      return eAction;
    }

  } // unnamed namespace

  bool CPVRGUIActions::SetRecordingOnChannel(const CPVRChannelPtr &channel, bool bOnOff)
  {
    bool bReturn = false;

    if (!channel)
      return bReturn;

    if (!g_PVRManager.CheckParentalLock(channel))
      return bReturn;

    if (g_PVRClients->HasTimerSupport(channel->ClientID()))
    {
      /* timers are supported on this channel */
      if (bOnOff && !channel->IsRecording())
      {
        CEpgInfoTagPtr epgTag;
        int iDuration = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);

        int iAction = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION);
        switch (iAction)
        {
          case RECORD_CURRENT_SHOW:
            epgTag = channel->GetEPGNow();
            break;

          case RECORD_INSTANTRECORDTIME:
            epgTag.reset();
            break;

          case ASK:
          {
            PVRRECORD_INSTANTRECORDACTION ePreselect = RECORD_INSTANTRECORDTIME;
            InstantRecordingActionSelector selector;
            CEpgInfoTagPtr epgTagNext;

            // fixed length recordings
            selector.AddAction(RECORD_30_MINUTES, "");
            selector.AddAction(RECORD_60_MINUTES, "");
            selector.AddAction(RECORD_120_MINUTES, "");

            const int iDurationDefault = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);
            if (iDurationDefault != 30 && iDurationDefault != 60 && iDurationDefault != 120)
              selector.AddAction(RECORD_INSTANTRECORDTIME, "");

            // epg-based recordings
            epgTag = channel->GetEPGNow();
            if (epgTag)
            {
              // "now"
              selector.AddAction(RECORD_CURRENT_SHOW, epgTag->Title());
              ePreselect = RECORD_CURRENT_SHOW;

              // "next"
              epgTagNext = channel->GetEPGNext();
              if (epgTagNext)
              {
                selector.AddAction(RECORD_NEXT_SHOW, epgTagNext->Title());

                // be smart. if current show is almost over, preselect next show.
                if (epgTag->ProgressPercentage() > 90.0f)
                  ePreselect = RECORD_NEXT_SHOW;
              }
            }

            selector.PreSelectAction(ePreselect);

            PVRRECORD_INSTANTRECORDACTION eSelected = selector.Select();
            switch (eSelected)
            {
              case NONE:
                return false; // dialog canceled

              case RECORD_30_MINUTES:
                iDuration = 30;
                epgTag.reset();
                break;

              case RECORD_60_MINUTES:
                iDuration = 60;
                epgTag.reset();
                break;

              case RECORD_120_MINUTES:
                iDuration = 120;
                epgTag.reset();
                break;

              case RECORD_INSTANTRECORDTIME:
                iDuration = iDurationDefault;
                epgTag.reset();
                break;

              case RECORD_CURRENT_SHOW:
                break;

              case RECORD_NEXT_SHOW:
                epgTag = epgTagNext;
                break;

              default:
                CLog::Log(LOGERROR, "PVRManager - %s - unknown instant record action selection (%d), defaulting to fixed length recording.", __FUNCTION__, eSelected);
                epgTag.reset();
                break;
            }
            break;
          }

          default:
            CLog::Log(LOGERROR, "PVRManager - %s - unknown instant record action setting value (%d), defaulting to fixed length recording.", __FUNCTION__, iAction);
            break;
        }

        const CPVRTimerInfoTagPtr newTimer(epgTag ? CPVRTimerInfoTag::CreateFromEpg(epgTag, false) : CPVRTimerInfoTag::CreateInstantTimerTag(channel, iDuration));

        if (newTimer)
          bReturn = newTimer->AddToClient();

        if (!bReturn)
          CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19164}); // "Information", "Can't start recording. Check the log for more information about this message."
      }
      else if (!bOnOff && channel->IsRecording())
      {
        /* delete active timers */
        bReturn = g_PVRTimers->DeleteTimersOnChannel(channel, true, true);
      }
    }

    return bReturn;
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

  void CPVRGUIActions::CheckAndSwitchToFullscreen(bool bFullscreen) const
  {
    CMediaSettings::GetInstance().SetVideoStartWindowed(!bFullscreen);

    if (bFullscreen)
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
    }
  }

  bool CPVRGUIActions::TryFastChannelSwitch(const CPVRChannelPtr &channel, bool bFullscreen) const
  {
    bool bSwitchSuccessful(false);

    if (channel->StreamURL().empty() &&
        (g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio()) &&
        (channel->IsRadio() == g_PVRManager.IsPlayingRadio()))
    {
      bSwitchSuccessful = g_application.m_pPlayer->SwitchChannel(channel);

      if (bSwitchSuccessful)
        CheckAndSwitchToFullscreen(bFullscreen);
    }

    return bSwitchSuccessful;
  }

  void CPVRGUIActions::StartPlayback(CFileItem *item, bool bFullscreen) const
  {
    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
    CheckAndSwitchToFullscreen(bFullscreen);
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
        StartPlayback(itemToPlay, true);
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
      StartPlayback(new CFileItem(*item), true);

    return true;
  }

  bool CPVRGUIActions::SwitchToChannel(const CFileItemPtr &item, bool bCheckResume) const
  {
    return SwitchToChannel(item, bCheckResume, CServiceBroker::GetSettings().GetBool(CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREEN));
  }

  bool CPVRGUIActions::SwitchToChannel(const CFileItemPtr &item, bool bCheckResume, bool bFullscreen) const
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
      bSwitchSuccessful = TryFastChannelSwitch(channel, bFullscreen);

      if (!bSwitchSuccessful)
      {
        StartPlayback(new CFileItem(channel), bFullscreen);
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

  bool CPVRGUIActions::SwitchToChannel(PlaybackType type) const
  {
    CFileItemPtr channel;
    bool bIsRadio(false);

    // check if the desired PlaybackType is already playing,
    // and if not, try to grab the last played channel of this type
    switch (type)
    {
      case PlaybackTypeRadio:
        if (g_PVRManager.IsPlayingRadio())
          return true;

        channel = g_PVRChannelGroups->GetGroupAllRadio()->GetLastPlayedChannel();
        bIsRadio = true;
        break;

      case PlaybackTypeTV:
        if (g_PVRManager.IsPlayingTV())
          return true;

        channel = g_PVRChannelGroups->GetGroupAllTV()->GetLastPlayedChannel();
        break;

      default:
        if (g_PVRManager.IsPlaying())
          return true;

        channel = g_PVRChannelGroups->GetLastPlayedChannel();
        break;
    }

    // if we have a last played channel, start playback
    if (channel && channel->HasPVRChannelInfoTag())
    {
      return SwitchToChannel(channel, true);
    }
    else
    {
      // if we don't, find the active channel group of the demanded type and play it's first channel
      const CPVRChannelGroupPtr channelGroup(g_PVRManager.GetPlayingGroup(bIsRadio));
      if (channelGroup)
      {
        // try to start playback of first channel in this group
        std::vector<PVRChannelGroupMember> groupMembers(channelGroup->GetMembers());
        if (!groupMembers.empty())
        {
          return SwitchToChannel(CFileItemPtr(new CFileItem((*groupMembers.begin()).channel)), true);
        }
      }
    }

    CLog::Log(LOGNOTICE, "PVRGUIActions - %s - could not determine %s channel to start playback with. No last played channel found, and first channel of active group could also not be determined.", __FUNCTION__, bIsRadio ? "Radio": "TV");

    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error,
                                          g_localizeStrings.Get(19166), // PVR information
                                          StringUtils::Format(g_localizeStrings.Get(19035).c_str(),
                                                              g_localizeStrings.Get(bIsRadio ? 19021 : 19020).c_str())); // Radio/TV could not be played. Check the log for details.
    return false;
  }

  bool CPVRGUIActions::ContinueLastPlayedChannel() const
  {
    const CFileItemPtr item(g_PVRChannelGroups->GetLastPlayedChannel());
    const CPVRChannelPtr channel(item ? item->GetPVRChannelInfoTag() : CPVRChannelPtr());
    bool bWasPlaying = false;
    if (channel)
    {
      // Obtain previous 'was playing on last app quit' flag and reset it, then.
      channel->SetWasPlayingOnLastQuit(false, bWasPlaying);
    }

    int iPlayMode = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRPLAYBACK_STARTLAST);
    if (iPlayMode == CONTINUE_LAST_CHANNEL_OFF)
      return false;

    // Only switch to the channel if it was playing on last app quit.
    if (bWasPlaying)
    {
      CLog::Log(LOGNOTICE, "PVRGUIActions - %s - continue playback on channel '%s'", __FUNCTION__, channel->ChannelName().c_str());
      g_PVRManager.SetPlayingGroup(g_PVRChannelGroups->GetLastPlayedGroup(channel->ChannelID()));
      return SwitchToChannel(item, true, iPlayMode == CONTINUE_LAST_CHANNEL_IN_FOREGROUND);
    }

    return false;
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

  bool CPVRGUIActions::StartChannelScan()
  {
    if (!g_PVRManager.IsStarted() || IsRunningChannelScan())
      return false;

    PVR_CLIENT scanClient;
    std::vector<PVR_CLIENT> possibleScanClients = g_PVRClients->GetClientsSupportingChannelScan();
    m_bChannelScanRunning = true;

    /* multiple clients found */
    if (possibleScanClients.size() > 1)
    {
      CGUIDialogSelect* pDialog= dynamic_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
      if (!pDialog)
      {
        CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_SELECT!", __FUNCTION__);
        m_bChannelScanRunning = false;
        return false;
      }

      pDialog->Reset();
      pDialog->SetHeading(CVariant{19119}); // "On which backend do you want to search?"

      for (const auto client : possibleScanClients)
        pDialog->Add(client->GetFriendlyName());

      pDialog->Open();

      int selection = pDialog->GetSelectedItem();
      if (selection >= 0)
        scanClient = possibleScanClients[selection];
    }
    /* one client found */
    else if (possibleScanClients.size() == 1)
    {
      scanClient = possibleScanClients[0];
    }
    /* no clients found */
    else if (!scanClient)
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{19033},  // "Information"
                                    CVariant{19192}); // "None of the connected PVR backends supports scanning for channels."
      m_bChannelScanRunning = false;
      return false;
    }

    /* start the channel scan */
    CLog::Log(LOGNOTICE,"CPVRGUIActions - %s - starting to scan for channels on client %s",
              __FUNCTION__, scanClient->GetFriendlyName().c_str());
    long perfCnt = XbmcThreads::SystemClockMillis();

    /* do the scan */
    if (scanClient->StartChannelScan() != PVR_ERROR_NO_ERROR)
      CGUIDialogOK::ShowAndGetInput(CVariant{257},    // "Error"
                                    CVariant{19193}); // "The channel scan can't be started. Check the log for more information about this message."

    CLog::Log(LOGNOTICE, "CPVRGUIActions - %s - channel scan finished after %li.%li seconds",
              __FUNCTION__, (XbmcThreads::SystemClockMillis() - perfCnt) / 1000, (XbmcThreads::SystemClockMillis() - perfCnt) % 1000);
    m_bChannelScanRunning = false;
    return true;
  }

  bool CPVRGUIActions::ProcessMenuHooks(const CFileItemPtr &item)
  {
    if (!g_PVRManager.IsStarted())
      return false;

    int iClientID = -1;
    PVR_MENUHOOK_CAT menuCategory = PVR_MENUHOOK_SETTING;

    if (item->IsEPG())
    {
      if (item->GetEPGInfoTag()->HasPVRChannel())
      {
        iClientID = item->GetEPGInfoTag()->ChannelTag()->ClientID();
        menuCategory = PVR_MENUHOOK_EPG;
      }
      else
        return false;
    }
    else if (item->IsPVRChannel())
    {
      iClientID = item->GetPVRChannelInfoTag()->ClientID();
      menuCategory = PVR_MENUHOOK_CHANNEL;
    }
    else if (item->IsDeletedPVRRecording())
    {
      iClientID = item->GetPVRRecordingInfoTag()->m_iClientId;
      menuCategory = PVR_MENUHOOK_DELETED_RECORDING;
    }
    else if (item->IsUsablePVRRecording())
    {
      iClientID = item->GetPVRRecordingInfoTag()->m_iClientId;
      menuCategory = PVR_MENUHOOK_RECORDING;
    }
    else if (item->IsPVRTimer())
    {
      iClientID = item->GetPVRTimerInfoTag()->m_iClientId;
      menuCategory = PVR_MENUHOOK_TIMER;
    }

    // get client id
    if (iClientID < 0 && menuCategory == PVR_MENUHOOK_SETTING)
    {
      PVR_CLIENTMAP clients;
      g_PVRClients->GetCreatedClients(clients);

      if (clients.size() == 1)
      {
        iClientID = clients.begin()->first;
      }
      else if (clients.size() > 1)
      {
        // have user select client
        CGUIDialogSelect* pDialog= dynamic_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
        if (!pDialog)
        {
          CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_SELECT!", __FUNCTION__);
          return false;
        }

        pDialog->Reset();
        pDialog->SetHeading(CVariant{19196}); // "PVR client specific actions"

        for (const auto client : clients)
        {
          pDialog->Add(client.second->GetBackendName());
        }

        pDialog->Open();

        int selection = pDialog->GetSelectedItem();
        if (selection >= 0)
        {
          auto client = clients.begin();
          std::advance(client, selection);
          iClientID = client->first;
        }
      }
    }

    if (iClientID < 0)
      iClientID = g_PVRClients->GetPlayingClientID();

    PVR_CLIENT client;
    if (g_PVRClients->GetCreatedClient(iClientID, client) && client->HasMenuHooks(menuCategory))
    {
      CGUIDialogSelect* pDialog= dynamic_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
      if (!pDialog)
      {
        CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_SELECT!", __FUNCTION__);
        return false;
      }

      pDialog->Reset();
      pDialog->SetHeading(CVariant{19196}); // "PVR client specific actions"

      PVR_MENUHOOKS *hooks = client->GetMenuHooks();
      std::vector<int> hookIDs;
      int selection = 0;

      for (unsigned int i = 0; i < hooks->size(); ++i)
      {
        if (hooks->at(i).category == menuCategory || hooks->at(i).category == PVR_MENUHOOK_ALL)
        {
          pDialog->Add(g_localizeStrings.GetAddonString(client->ID(), hooks->at(i).iLocalizedStringId));
          hookIDs.push_back(i);
        }
      }

      if (hookIDs.size() > 1)
      {
        pDialog->Open();
        selection = pDialog->GetSelectedItem();
      }

      if (selection >= 0)
        client->CallMenuHook(hooks->at(hookIDs.at(selection)), item.get());
      else
        return false;
    }

    return true;
  }

  bool CPVRGUIActions::ResetPVRDatabase(bool bResetEPGOnly)
  {
    CLog::Log(LOGNOTICE,"CPVRGUIActions - %s - clearing the PVR database", __FUNCTION__);

    CGUIDialogProgress* pDlgProgress = dynamic_cast<CGUIDialogProgress*>(g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
    if (!pDlgProgress)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_PROGRESS!", __FUNCTION__);
      return false;
    }

    if (bResetEPGOnly)
    {
      if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{19098},  // "Warning!"
                                            CVariant{19188})) // "All your guide data will be cleared. Are you sure?"
        return false;
    }
    else
    {
      if (!g_PVRManager.CheckParentalPIN(g_localizeStrings.Get(19262)) || // "Parental control. Enter PIN:"
          !CGUIDialogYesNo::ShowAndGetInput(CVariant{19098},  // "Warning!"
                                            CVariant{19186})) // "All your TV related data (channels, groups, guide) will be cleared. Are you sure?"
        return false;
    }

    CDateTime::ResetTimezoneBias();

    g_EpgContainer.Stop();

    pDlgProgress->SetHeading(CVariant{313}); // "Cleaning database"
    pDlgProgress->SetLine(0, CVariant{g_localizeStrings.Get(19187)}); // "Clearing all related data."
    pDlgProgress->SetLine(1, CVariant{""});
    pDlgProgress->SetLine(2, CVariant{""});

    pDlgProgress->Open();
    pDlgProgress->Progress();

    if (g_PVRManager.IsPlaying())
    {
      CLog::Log(LOGNOTICE,"CPVRGUIActions - %s - stopping playback", __FUNCTION__);
      CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
    }

    pDlgProgress->SetPercentage(10);
    pDlgProgress->Progress();

    /* reset the EPG pointers */
    const CPVRDatabasePtr database(g_PVRManager.GetTVDatabase());
    if (database)
      database->ResetEPG();

    /* stop the thread, close database */
    g_PVRManager.Stop();

    pDlgProgress->SetPercentage(20);
    pDlgProgress->Progress();

    if (database && database->Open())
    {
      /* clean the EPG database */
      g_EpgContainer.Reset();
      pDlgProgress->SetPercentage(30);
      pDlgProgress->Progress();

      if (!bResetEPGOnly)
      {
        database->DeleteChannelGroups();
        pDlgProgress->SetPercentage(50);
        pDlgProgress->Progress();

        /* delete all channels */
        database->DeleteChannels();
        pDlgProgress->SetPercentage(70);
        pDlgProgress->Progress();

        /* delete all channel and recording settings */
        CVideoDatabase videoDatabase;

        if (videoDatabase.Open())
        {
          videoDatabase.EraseVideoSettings("pvr://channels/");
          videoDatabase.EraseVideoSettings(CPVRRecordingsPath::PATH_RECORDINGS);
          videoDatabase.Close();
        }

        pDlgProgress->SetPercentage(80);
        pDlgProgress->Progress();

        /* delete all client information */
        pDlgProgress->SetPercentage(90);
        pDlgProgress->Progress();
      }

      database->Close();
    }

    CLog::Log(LOGNOTICE,"CPVRGUIActions - %s - %s database cleared", __FUNCTION__, bResetEPGOnly ? "EPG" : "PVR and EPG");

    if (database)
      database->Open();

    CLog::Log(LOGNOTICE,"CPVRGUIActions - %s - restarting the PVRManager", __FUNCTION__);
    g_PVRManager.Start();

    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
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
