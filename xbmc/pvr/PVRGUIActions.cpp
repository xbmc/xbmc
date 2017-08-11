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
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
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
#include "pvr/dialogs/GUIDialogPVRChannelGuide.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingSettings.h"
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
#include "threads/Thread.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include "PVRGUIActions.h"

using namespace KODI::MESSAGING;

namespace PVR
{
  class AsyncRecordingAction : private IRunnable
  {
  public:
    bool Execute(const CFileItemPtr &item);

  protected:
    AsyncRecordingAction() : m_bSuccess(false) {}

  private:
    // IRunnable implementation
    void Run() override;

    // the worker function
    virtual bool DoRun(const CFileItemPtr &item) = 0;

    CFileItemPtr m_item;
    bool m_bSuccess;
  };

  bool AsyncRecordingAction::Execute(const CFileItemPtr &item)
  {
    m_item = item;
    CGUIDialogBusy::Wait(this, 100, false);
    return m_bSuccess;
  }

  void AsyncRecordingAction::Run()
  {
    m_bSuccess = DoRun(m_item);

    if (m_bSuccess)
      CServiceBroker::GetPVRManager().TriggerRecordingsUpdate();
  }

  class AsyncRenameRecording : public AsyncRecordingAction
  {
  public:
    AsyncRenameRecording(const std::string &strNewName) : m_strNewName(strNewName) {}

  private:
    bool DoRun(const CFileItemPtr &item) override { return CServiceBroker::GetPVRManager().Recordings()->RenameRecording(*item, m_strNewName); }
    std::string m_strNewName;
  };

  class AsyncDeleteRecording : public AsyncRecordingAction
  {
  private:
    bool DoRun(const CFileItemPtr &item) override { return CServiceBroker::GetPVRManager().Recordings()->Delete(*item); }
  };

  class AsyncEmptyRecordingsTrash : public AsyncRecordingAction
  {
  private:
    bool DoRun(const CFileItemPtr &item) override { return CServiceBroker::GetPVRManager().Recordings()->DeleteAllRecordingsFromTrash(); }
  };

  class AsyncUndeleteRecording : public AsyncRecordingAction
  {
  private:
    bool DoRun(const CFileItemPtr &item) override { return CServiceBroker::GetPVRManager().Recordings()->Undelete(*item); }
  };

  class AsyncSetRecordingPlayCount : public AsyncRecordingAction
  {
  private:
    bool DoRun(const CFileItemPtr &item) override
    {
      PVR_ERROR error;
      CServiceBroker::GetPVRManager().Clients()->SetRecordingPlayCount(*item->GetPVRRecordingInfoTag(), item->GetPVRRecordingInfoTag()->GetLocalPlayCount(), &error);
      return error == PVR_ERROR_NO_ERROR;
    }
  };

  class AsyncSetRecordingLifetime : public AsyncRecordingAction
  {
  private:
    bool DoRun(const CFileItemPtr &item) override
    {
      PVR_ERROR error;
      CServiceBroker::GetPVRManager().Clients()->SetRecordingLifetime(*item->GetPVRRecordingInfoTag(), &error);
      return error == PVR_ERROR_NO_ERROR;
    }
  };

  CPVRGUIActions::CPVRGUIActions()
  : m_bChannelScanRunning(false),
    m_settings({
      CSettings::SETTING_LOOKANDFEEL_STARTUPACTION,
      CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME,
      CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION,
      CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREEN,
      CSettings::SETTING_PVRPARENTAL_PIN,
      CSettings::SETTING_PVRPARENTAL_ENABLED,
      CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO
    })
  {
  }

  bool CPVRGUIActions::ShowEPGInfo(const CFileItemPtr &item) const
  {
    const CPVRChannelPtr channel(CPVRItem(item).GetChannel());
    if (channel && !CheckParentalLock(channel))
      return false;

    const CPVREpgInfoTagPtr epgTag(CPVRItem(item).GetEpgInfoTag());
    if (!epgTag)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no epg tag!", __FUNCTION__);
      return false;
    }

    CGUIDialogPVRGuideInfo* pDlgInfo = g_windowManager.GetWindow<CGUIDialogPVRGuideInfo>(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_PVR_GUIDE_INFO!", __FUNCTION__);
      return false;
    }

    pDlgInfo->SetProgInfo(epgTag);
    pDlgInfo->Open();
    return true;
  }


  bool CPVRGUIActions::ShowChannelEPG(const CFileItemPtr &item) const
  {
    const CPVRChannelPtr channel(CPVRItem(item).GetChannel());
    if (channel && !CheckParentalLock(channel))
      return false;

    CGUIDialogPVRChannelGuide* pDlgInfo = g_windowManager.GetWindow<CGUIDialogPVRChannelGuide>(WINDOW_DIALOG_PVR_CHANNEL_GUIDE);
    if (!pDlgInfo)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_PVR_CHANNEL_GUIDE!", __FUNCTION__);
      return false;
    }

    pDlgInfo->Open(channel);
    return true;
  }


  bool CPVRGUIActions::ShowRecordingInfo(const CFileItemPtr &item) const
  {
    if (!item->IsPVRRecording())
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no recording!", __FUNCTION__);
      return false;
    }

    CGUIDialogPVRRecordingInfo* pDlgInfo = g_windowManager.GetWindow<CGUIDialogPVRRecordingInfo>(WINDOW_DIALOG_PVR_RECORDING_INFO);
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
    CGUIWindowPVRSearchBase *windowSearch;
    if (bRadio)
      windowSearch = g_windowManager.GetWindow<CGUIWindowPVRRadioSearch>(windowSearchId);
    else
      windowSearch = g_windowManager.GetWindow<CGUIWindowPVRTVSearch>(windowSearchId);

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
    CGUIDialogPVRTimerSettings* pDlgInfo = g_windowManager.GetWindow<CGUIDialogPVRTimerSettings>(WINDOW_DIALOG_PVR_TIMER_SETTING);
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
      return AddTimer(newTimer);
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

    if (!CheckParentalLock(channel))
      return false;

    const CPVREpgInfoTagPtr epgTag(CPVRItem(item).GetEpgInfoTag());
    if (!epgTag && bCreateRule)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no epg tag!", __FUNCTION__);
      return false;
    }

    CPVRTimerInfoTagPtr timer(bCreateRule || !epgTag ? nullptr : epgTag->Timer());
    CPVRTimerInfoTagPtr rule (bCreateRule ? CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer) : nullptr);
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

    return AddTimer(newTimer);
  }

  bool CPVRGUIActions::AddTimer(const CPVRTimerInfoTagPtr &item) const
  {
    if (!item->Channel() && item->GetTimerType() && !item->GetTimerType()->IsEpgBasedTimerRule())
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no channel given", __FUNCTION__);
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19109}); // "Information", "Couldn't save timer. Check the log for more information about this message."
      return false;
    }

    if (!CServiceBroker::GetPVRManager().Clients()->GetClientCapabilities(item->m_iClientId).SupportsTimers())
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19215}); // "Information", "The PVR backend does not support timers."
      return false;
    }

    if (!CheckParentalLock(item->Channel()))
      return false;

    return CServiceBroker::GetPVRManager().Timers()->AddTimer(item);
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
      InstantRecordingActionSelector(int iInstantRecordTime);
      virtual ~InstantRecordingActionSelector() = default;

      void AddAction(PVRRECORD_INSTANTRECORDACTION eAction, const std::string &title);
      void PreSelectAction(PVRRECORD_INSTANTRECORDACTION eAction);
      PVRRECORD_INSTANTRECORDACTION Select();

    private:
      int m_iInstantRecordTime;
      CGUIDialogSelect *m_pDlgSelect; // not owner!
      std::map<PVRRECORD_INSTANTRECORDACTION, int> m_actions;
    };

    InstantRecordingActionSelector::InstantRecordingActionSelector(int iInstantRecordTime)
    : m_iInstantRecordTime(iInstantRecordTime),
      m_pDlgSelect(g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT))
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
            m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19090).c_str(), m_iInstantRecordTime)); // Record next <default duration> minutes
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

    if (!CheckParentalLock(channel))
      return bReturn;

    if (CServiceBroker::GetPVRManager().Clients()->GetClientCapabilities(channel->ClientID()).SupportsTimers())
    {
      /* timers are supported on this channel */
      if (bOnOff && !channel->IsRecording())
      {
        CPVREpgInfoTagPtr epgTag;
        int iDuration = m_settings.GetIntValue(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);

        int iAction = m_settings.GetIntValue(CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION);
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
            const int iDurationDefault = m_settings.GetIntValue(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);
            InstantRecordingActionSelector selector(iDurationDefault);
            CPVREpgInfoTagPtr epgTagNext;

            // fixed length recordings
            selector.AddAction(RECORD_30_MINUTES, "");
            selector.AddAction(RECORD_60_MINUTES, "");
            selector.AddAction(RECORD_120_MINUTES, "");

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
                CLog::Log(LOGERROR, "PVRGUIActions - %s - unknown instant record action selection (%d), defaulting to fixed length recording.", __FUNCTION__, eSelected);
                epgTag.reset();
                break;
            }
            break;
          }

          default:
            CLog::Log(LOGERROR, "PVRGUIActions - %s - unknown instant record action setting value (%d), defaulting to fixed length recording.", __FUNCTION__, iAction);
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
        bReturn = CServiceBroker::GetPVRManager().Timers()->DeleteTimersOnChannel(channel, true, true);
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

    return CServiceBroker::GetPVRManager().Timers()->UpdateTimer(timer);
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
        return CServiceBroker::GetPVRManager().Timers()->UpdateTimer(newTimer);
      }
      else
      {
        // timer type changed. delete the original timer, then create the new timer. this order is
        // important. for instance, the new timer might be a rule which schedules the original timer.
        // deleting the original timer after creating the rule would do literally this and we would
        // end up with one timer missing wrt to the rule defined by the new timer.
        if (CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, timer->IsRecording(), false))
        {
          if (AddTimer(newTimer))
            return true;

          // rollback.
          return AddTimer(timer);
        }
      }
    }
    return false;
  }

  bool CPVRGUIActions::EditTimerRule(const CFileItemPtr &item) const
  {
    const CFileItemPtr parentTimer(CServiceBroker::GetPVRManager().Timers()->GetTimerRule(item));
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
      if (!CServiceBroker::GetPVRManager().Timers()->RenameTimer(*item, strNewName))
        return false;
    }

    CGUIWindowPVRBase *pvrWindow = dynamic_cast<CGUIWindowPVRBase*>(g_windowManager.GetWindow(g_windowManager.GetActiveWindow()));
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
      timer = CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer);

    if (!timer)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no timer rule!", __FUNCTION__);
      return false;
    }

    if (bIsRecording)
    {
      if (ConfirmStopRecording(timer))
        return CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, true, false);
    }
    else if (timer->HasTimerType() && timer->GetTimerType()->IsReadOnly())
    {
      return false;
    }
    else
    {
      bool bAlsoDeleteRule(false);
      if (ConfirmDeleteTimer(timer, bAlsoDeleteRule))
        return CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, false, bAlsoDeleteRule);
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

    CServiceBroker::GetPVRManager().TriggerRecordingsUpdate();
    return true;
  }

  bool CPVRGUIActions::ConfirmStopRecording(const CPVRTimerInfoTagPtr &timer) const
  {
    return CGUIDialogYesNo::ShowAndGetInput(CVariant{847}, // "Confirm stop recording"
                                            CVariant{848}, // "Are you sure you want to stop this recording?"
                                            CVariant{""},
                                            CVariant{timer->Title()});
  }

  bool CPVRGUIActions::EditRecording(const CFileItemPtr &item) const
  {
    const CPVRRecordingPtr recording = CPVRItem(item).GetRecording();
    if (!recording)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - no recording!", __FUNCTION__);
      return false;
    }

    CPVRRecordingPtr origRecording(new CPVRRecording);
    origRecording->Update(*recording);

    if (!ShowRecordingSettings(recording))
      return false;

    if (origRecording->m_strTitle != recording->m_strTitle)
    {
      if (!AsyncRenameRecording(recording->m_strTitle).Execute(item))
        CLog::Log(LOGERROR, "CPVRGUIActions - %s - renaming recording failed!", __FUNCTION__);
    }

    if (origRecording->GetLocalPlayCount() != recording->GetLocalPlayCount())
    {
      if (!AsyncSetRecordingPlayCount().Execute(item))
        CLog::Log(LOGERROR, "CPVRGUIActions - %s - setting recording playcount failed!", __FUNCTION__);
    }

    if (origRecording->m_iLifetime != recording->m_iLifetime)
    {
      if (!AsyncSetRecordingLifetime().Execute(item))
        CLog::Log(LOGERROR, "CPVRGUIActions - %s - setting recording lifetime failed!", __FUNCTION__);
    }

    return true;
  }

  bool CPVRGUIActions::RenameRecording(const CFileItemPtr &item) const
  {
    const CPVRRecordingPtr recording(item->GetPVRRecordingInfoTag());
    if (!recording)
      return false;

    std::string strNewName(recording->m_strTitle);
    if (!CGUIKeyboardFactory::ShowAndGetInput(strNewName, CVariant{g_localizeStrings.Get(19041)}, false))
      return false;

    if (!AsyncRenameRecording(strNewName).Execute(item))
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{257}, CVariant{19111}); // "Error", "PVR backend error. Check the log for more information about this message."
      return false;
    }

    return true;
  }

  bool CPVRGUIActions::DeleteRecording(const CFileItemPtr &item) const
  {
    if ((!item->IsPVRRecording() && !item->m_bIsFolder) || item->IsParentFolder())
      return false;

    if (!ConfirmDeleteRecording(item))
      return false;

    if (!AsyncDeleteRecording().Execute(item))
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{257}, CVariant{19111}); // "Error", "PVR backend error. Check the log for more information about this message."
      return false;
    }

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

    if (!AsyncEmptyRecordingsTrash().Execute(CFileItemPtr()))
      return false;

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

    if (!AsyncUndeleteRecording().Execute(item))
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{257}, CVariant{19111}); // "Error", "PVR backend error. Check the log for more information about this message."
      return false;
    }

    return true;
  }

  bool CPVRGUIActions::ShowRecordingSettings(const CPVRRecordingPtr &recording) const
  {
    CGUIDialogPVRRecordingSettings* pDlgInfo = g_windowManager.GetWindow<CGUIDialogPVRRecordingSettings>(WINDOW_DIALOG_PVR_RECORDING_SETTING);
    if (!pDlgInfo)
    {
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - unable to get WINDOW_DIALOG_PVR_RECORDING_SETTING!", __FUNCTION__);
      return false;
    }

    pDlgInfo->SetRecording(recording);
    pDlgInfo->Open();

    return pDlgInfo->IsConfirmed();
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

  void CPVRGUIActions::StartPlayback(CFileItem *item, bool bFullscreen) const
  {
    const CPVRChannelPtr channel = item->GetPVRChannelInfoTag();
    if (channel)
      CServiceBroker::GetPVRManager().Clients()->FillLiveStreamFileItem(item);

    const CPVRRecordingPtr recording = item->GetPVRRecordingInfoTag();
    if (recording)
      CServiceBroker::GetPVRManager().Clients()->FillRecordingStreamFileItem(item);

    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
    CheckAndSwitchToFullscreen(bFullscreen);
  }

  bool CPVRGUIActions::PlayRecording(const CFileItemPtr &item, bool bCheckResume) const
  {
    const CPVRRecordingPtr recording(CPVRItem(item).GetRecording());
    if (!recording)
      return false;

    if (CServiceBroker::GetPVRManager().IsPlayingRecording(recording))
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
    return SwitchToChannel(item, bCheckResume, m_settings.GetBoolValue(CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREEN));
  }

  bool CPVRGUIActions::SwitchToChannel(const CFileItemPtr &item, bool bCheckResume, bool bFullscreen) const
  {
    if (item->m_bIsFolder)
      return false;

    const CPVRChannelPtr channel(CPVRItem(item).GetChannel());
    if ((channel && CServiceBroker::GetPVRManager().IsPlayingChannel(channel)) ||
        (channel && channel->HasRecording() && CServiceBroker::GetPVRManager().IsPlayingRecording(channel->GetRecording())))
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, g_windowManager.GetActiveWindow());
      g_windowManager.SendMessage(msg);
      return true;
    }

    // switch to channel or if recording present, ask whether to switch or play recording...
    if (channel && CheckParentalLock(channel))
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

      StartPlayback(new CFileItem(channel), bFullscreen);

      int iChannelInfoDisplayTime = m_settings.GetIntValue(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO);
      if (iChannelInfoDisplayTime > 0)
        CServiceBroker::GetPVRManager().ShowPlayerInfo(iChannelInfoDisplayTime);

      return true;
    }

    std::string channelName = g_localizeStrings.Get(19029); // Channel
    if (channel)
      channelName = channel->ChannelName();
    std::string msg = StringUtils::Format(g_localizeStrings.Get(19035).c_str(), channelName.c_str()); // CHANNELNAME could not be played. Check the log for details.

    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(19166), msg); // PVR information
    return false;
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
        if (CServiceBroker::GetPVRManager().IsPlayingRadio())
          return true;

        channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllRadio()->GetLastPlayedChannel();
        bIsRadio = true;
        break;

      case PlaybackTypeTV:
        if (CServiceBroker::GetPVRManager().IsPlayingTV())
          return true;

        channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV()->GetLastPlayedChannel();
        break;

      default:
        if (CServiceBroker::GetPVRManager().IsPlaying())
          return true;

        channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetLastPlayedChannel();
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
      const CPVRChannelGroupPtr channelGroup(CServiceBroker::GetPVRManager().GetPlayingGroup(bIsRadio));
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

  bool CPVRGUIActions::PlayChannelOnStartup() const
  {
    int iAction = m_settings.GetIntValue(CSettings::SETTING_LOOKANDFEEL_STARTUPACTION);
    if (iAction != STARTUP_ACTION_PLAY_TV &&
        iAction != STARTUP_ACTION_PLAY_RADIO)
      return false;

    bool playTV = iAction == STARTUP_ACTION_PLAY_TV;
    const CPVRChannelGroupsContainerPtr groups(CServiceBroker::GetPVRManager().ChannelGroups());
    CPVRChannelGroupPtr group = playTV ? groups->GetGroupAllTV() : groups->GetGroupAllRadio();

    // get the last played channel or fallback to first channel
    CFileItemPtr item(group->GetLastPlayedChannel());
    if (item->HasPVRChannelInfoTag())
    {
      group = groups->GetLastPlayedGroup(item->GetPVRChannelInfoTag()->ChannelID());
    }
    else
    {
      // fallback to first channel
      auto channels(group->GetMembers());
      if (channels.empty())
        return false;

      item = std::make_shared<CFileItem>(channels.front().channel);
    }

    CLog::Log(LOGNOTICE, "PVRGUIActions - %s - start playback of channel '%s'", __FUNCTION__, item->GetPVRChannelInfoTag()->ChannelName().c_str());
    CServiceBroker::GetPVRManager().SetPlayingGroup(group);
    return SwitchToChannel(item, true, m_settings.GetBoolValue(CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREEN));
  }

  bool CPVRGUIActions::PlayMedia(const CFileItemPtr &item) const
  {
    CFileItemPtr pvrItem(item);
    if (URIUtils::IsPVRChannel(item->GetPath()) && !item->HasPVRChannelInfoTag())
      pvrItem = CServiceBroker::GetPVRManager().ChannelGroups()->GetByPath(item->GetPath());
    else if (URIUtils::IsPVRRecording(item->GetPath()) && !item->HasPVRRecordingInfoTag())
      pvrItem = CServiceBroker::GetPVRManager().Recordings()->GetByPath(item->GetPath());

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

    if (!CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(channel->IsRadio())->RemoveFromGroup(channel))
      return false;

    CGUIWindowPVRBase *pvrWindow = dynamic_cast<CGUIWindowPVRBase*>(g_windowManager.GetWindow(g_windowManager.GetActiveWindow()));
    if (pvrWindow)
      pvrWindow->DoRefresh();
    else
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - called on non-pvr window. no refresh possible.", __FUNCTION__);

    return true;
  }

  bool CPVRGUIActions::StartChannelScan()
  {
    if (!CServiceBroker::GetPVRManager().IsStarted() || IsRunningChannelScan())
      return false;

    PVR_CLIENT scanClient;
    std::vector<PVR_CLIENT> possibleScanClients = CServiceBroker::GetPVRManager().Clients()->GetClientsSupportingChannelScan();
    m_bChannelScanRunning = true;

    /* multiple clients found */
    if (possibleScanClients.size() > 1)
    {
      CGUIDialogSelect* pDialog= g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
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
    if (!CServiceBroker::GetPVRManager().IsStarted())
      return false;

    int iClientID = -1;
    PVR_MENUHOOK_CAT menuCategory = PVR_MENUHOOK_SETTING;

    if (item)
    {
      if (item->IsEPG())
      {
        if (item->GetEPGInfoTag()->HasChannel())
        {
          iClientID = item->GetEPGInfoTag()->Channel()->ClientID();
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
    }

    // get client id
    if (iClientID < 0 && menuCategory == PVR_MENUHOOK_SETTING)
    {
      PVR_CLIENTMAP clients;
      CServiceBroker::GetPVRManager().Clients()->GetCreatedClients(clients);

      if (clients.size() == 1)
      {
        iClientID = clients.begin()->first;
      }
      else if (clients.size() > 1)
      {
        // have user select client
        CGUIDialogSelect* pDialog= g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
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
      iClientID = CServiceBroker::GetPVRManager().Clients()->GetPlayingClientID();

    PVR_CLIENT client;
    if (CServiceBroker::GetPVRManager().Clients()->GetCreatedClient(iClientID, client) && client->HasMenuHooks(menuCategory))
    {
      CGUIDialogSelect* pDialog= g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
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

    CGUIDialogProgress* pDlgProgress = g_windowManager.GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
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
      if (!CheckParentalPIN() ||
          !CGUIDialogYesNo::ShowAndGetInput(CVariant{19098},  // "Warning!"
                                            CVariant{19186})) // "All your TV related data (channels, groups, guide) will be cleared. Are you sure?"
        return false;
    }

    CDateTime::ResetTimezoneBias();

    CServiceBroker::GetPVRManager().EpgContainer().Stop();

    pDlgProgress->SetHeading(CVariant{313}); // "Cleaning database"
    pDlgProgress->SetLine(0, CVariant{g_localizeStrings.Get(19187)}); // "Clearing all related data."
    pDlgProgress->SetLine(1, CVariant{""});
    pDlgProgress->SetLine(2, CVariant{""});

    pDlgProgress->Open();
    pDlgProgress->Progress();

    if (CServiceBroker::GetPVRManager().IsPlaying())
    {
      CLog::Log(LOGNOTICE,"CPVRGUIActions - %s - stopping playback", __FUNCTION__);
      CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
    }

    pDlgProgress->SetPercentage(10);
    pDlgProgress->Progress();

    /* reset the EPG pointers */
    const CPVRDatabasePtr database(CServiceBroker::GetPVRManager().GetTVDatabase());
    if (database)
      database->ResetEPG();

    /* stop the thread, close database */
    CServiceBroker::GetPVRManager().Stop();

    pDlgProgress->SetPercentage(20);
    pDlgProgress->Progress();

    if (database && database->Open())
    {
      /* clean the EPG database */
      CServiceBroker::GetPVRManager().EpgContainer().Reset();
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
    CServiceBroker::GetPVRManager().Start();

    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
    return true;
  }

  bool CPVRGUIActions::CheckParentalLock(const CPVRChannelPtr &channel) const
  {
    bool bReturn = !CServiceBroker::GetPVRManager().IsParentalLocked(channel) || CheckParentalPIN();

    if (!bReturn)
      CLog::Log(LOGERROR, "CPVRGUIActions - %s - parental lock verification failed for channel '%s': wrong PIN entered.", __FUNCTION__, channel->ChannelName().c_str());

    return bReturn;
  }

  bool CPVRGUIActions::CheckParentalPIN() const
  {
    std::string pinCode = m_settings.GetStringValue(CSettings::SETTING_PVRPARENTAL_PIN);

    if (!m_settings.GetBoolValue(CSettings::SETTING_PVRPARENTAL_ENABLED) || pinCode.empty())
      return true;

    // Locked channel. Enter PIN:
    bool bValidPIN = CGUIDialogNumeric::ShowAndVerifyInput(pinCode, g_localizeStrings.Get(19262), true); // "Parental control. Enter PIN:"
    if (!bValidPIN)
    {
      // display message: The entered PIN number was incorrect
      CGUIDialogOK::ShowAndGetInput(CVariant{19264}, CVariant{19265}); // "Incorrect PIN", "The entered PIN was incorrect."
    }
    else
    {
      // restart the parental timer
      CServiceBroker::GetPVRManager().RestartParentalTimer();
    }

    return bValidPIN;
  }

  CPVRChannelNumberInputHandler &CPVRGUIActions::GetChannelNumberInputHandler()
  {
    // window/dialog specific input handler
    CPVRChannelNumberInputHandler *windowInputHandler = dynamic_cast<CPVRChannelNumberInputHandler*>(g_windowManager.GetWindow(g_windowManager.GetFocusedWindow()));
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
    if (iChannelNumber > 0 && CServiceBroker::GetPVRManager().IsPlaying())
    {
      const CPVRChannelPtr playingChannel(CServiceBroker::GetPVRManager().GetCurrentChannel());
      if (playingChannel)
      {
        if (iChannelNumber != playingChannel->ChannelNumber())
        {
          const CPVRChannelGroupPtr selectedGroup(CServiceBroker::GetPVRManager().GetPlayingGroup(playingChannel->IsRadio()));
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
    if (CServiceBroker::GetPVRManager().IsPlaying())
    {
      const CPVRChannelPtr playingChannel(CServiceBroker::GetPVRManager().GetCurrentChannel());
      if (playingChannel)
      {
        const CPVRChannelGroupPtr group(CServiceBroker::GetPVRManager().ChannelGroups()->GetPreviousPlayedGroup());
        if (group)
        {
          CServiceBroker::GetPVRManager().SetPlayingGroup(group);
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
