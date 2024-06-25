/*
 *  Copyright (C) 2016-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsTimers.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVREventLogJob.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsParentalControl.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/Settings.h"
#include "threads/IRunnable.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using namespace PVR;
using namespace KODI::MESSAGING;

namespace
{
class AsyncUpdateTimer : private IRunnable
{
public:
  AsyncUpdateTimer(const CPVRGUIActionsTimers& guiActions,
                   const std::shared_ptr<CPVRTimerInfoTag>& oldTimer,
                   const std::shared_ptr<CPVRTimerInfoTag>& newTimer)
    : m_guiActions(guiActions), m_oldTimer(oldTimer), m_newTimer(newTimer)
  {
  }

  bool Execute()
  {
    CGUIDialogBusy::Wait(this, 100, false);
    return m_success;
  }

private:
  // IRunnable implementation
  void Run() override
  {
    m_success = true;

    if (m_newTimer->GetTimerType() == m_oldTimer->GetTimerType() &&
        m_newTimer->ClientID() == m_oldTimer->ClientID())
    {
      if (CServiceBroker::GetPVRManager().Timers()->UpdateTimer(m_newTimer))
        return;

      HELPERS::ShowOKDialogText(CVariant{257},
                                CVariant{19263}); // "Error", "Could not update the timer."
      m_success = false;
      return;
    }
    else
    {
      // Timer type or client changed. Delete the original timer, then create the new timer. This
      // order is important. for instance, the new timer might be a rule which schedules the
      // original timer. Deleting the original timer after creating the rule would do literally this
      // and we would end up with one timer missing wrt to the rule defined by the new timer.
      if (m_guiActions.DeleteTimer(m_oldTimer, m_oldTimer->IsRecording(), false))
      {
        if (m_newTimer->IsTimerRule())
          m_newTimer->ResetChildState();

        m_success = m_guiActions.AddTimer(m_newTimer);
        if (!m_success)
        {
          // rollback.
          m_success = m_guiActions.AddTimer(m_oldTimer);
        }
      }
    }
  }

  const CPVRGUIActionsTimers& m_guiActions;
  std::shared_ptr<CPVRTimerInfoTag> m_oldTimer;
  std::shared_ptr<CPVRTimerInfoTag> m_newTimer;
  bool m_success{false};
};
} // unnamed namespace

CPVRGUIActionsTimers::CPVRGUIActionsTimers()
  : m_settings({CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME,
                CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION,
                CSettings::SETTING_PVRREMINDERS_AUTOCLOSEDELAY,
                CSettings::SETTING_PVRREMINDERS_AUTORECORD,
                CSettings::SETTING_PVRREMINDERS_AUTOSWITCH})
{
}

bool CPVRGUIActionsTimers::ShowTimerSettings(const std::shared_ptr<CPVRTimerInfoTag>& timer) const
{
  CGUIDialogPVRTimerSettings* pDlgInfo =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRTimerSettings>(
          WINDOW_DIALOG_PVR_TIMER_SETTING);
  if (!pDlgInfo)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_TIMER_SETTING!");
    return false;
  }

  pDlgInfo->SetTimer(timer);
  pDlgInfo->Open();

  return pDlgInfo->IsConfirmed();
}

bool CPVRGUIActionsTimers::AddReminder(const CFileItem& item) const
{
  const std::shared_ptr<CPVREpgInfoTag> epgTag = CPVRItem(item).GetEpgInfoTag();
  if (!epgTag)
  {
    CLog::LogF(LOGERROR, "No epg tag!");
    return false;
  }

  if (CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epgTag))
  {
    HELPERS::ShowOKDialogText(CVariant{19033}, // "Information"
                              CVariant{19034}); // "There is already a timer set for this event"
    return false;
  }

  const std::shared_ptr<CPVRTimerInfoTag> newTimer =
      CPVRTimerInfoTag::CreateReminderFromEpg(epgTag);
  if (!newTimer)
  {
    HELPERS::ShowOKDialogText(CVariant{19033}, // "Information"
                              CVariant{19094}); // Timer creation failed. Unsupported timer type.
    return false;
  }

  return AddTimer(newTimer);
}

bool CPVRGUIActionsTimers::AddTimer(bool bRadio) const
{
  const std::shared_ptr<CPVRTimerInfoTag> newTimer(new CPVRTimerInfoTag(bRadio));
  if (ShowTimerSettings(newTimer))
  {
    return AddTimer(newTimer);
  }
  return false;
}

bool CPVRGUIActionsTimers::AddTimer(const CFileItem& item, bool bShowTimerSettings) const
{
  return AddTimer(item, false, bShowTimerSettings, false);
}

bool CPVRGUIActionsTimers::AddTimerRule(const CFileItem& item,
                                        bool bShowTimerSettings,
                                        bool bFallbackToOneShotTimer) const
{
  return AddTimer(item, true, bShowTimerSettings, bFallbackToOneShotTimer);
}

bool CPVRGUIActionsTimers::AddTimer(const CFileItem& item,
                                    bool bCreateRule,
                                    bool bShowTimerSettings,
                                    bool bFallbackToOneShotTimer) const
{
  const std::shared_ptr<CPVRChannel> channel(CPVRItem(item).GetChannel());
  if (!channel)
  {
    CLog::LogF(LOGERROR, "No channel!");
    return false;
  }

  if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Parental>().CheckParentalLock(channel) !=
      ParentalCheckResult::SUCCESS)
    return false;

  std::shared_ptr<CPVREpgInfoTag> epgTag = CPVRItem(item).GetEpgInfoTag();
  if (epgTag)
  {
    if (epgTag->IsGapTag())
      epgTag.reset(); // for gap tags, we can only create instant timers
  }
  else if (bCreateRule)
  {
    CLog::LogF(LOGERROR, "No epg tag!");
    return false;
  }

  std::shared_ptr<CPVRTimerInfoTag> timer(
      bCreateRule || !epgTag ? nullptr
                             : CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epgTag));
  std::shared_ptr<CPVRTimerInfoTag> rule(
      bCreateRule ? CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer) : nullptr);
  if (timer || rule)
  {
    HELPERS::ShowOKDialogText(
        CVariant{19033},
        CVariant{19034}); // "Information", "There is already a timer set for this event"
    return false;
  }

  std::shared_ptr<CPVRTimerInfoTag> newTimer(
      epgTag ? CPVRTimerInfoTag::CreateFromEpg(epgTag, bCreateRule)
             : CPVRTimerInfoTag::CreateInstantTimerTag(channel));
  if (!newTimer)
  {
    if (bCreateRule && bFallbackToOneShotTimer)
      newTimer = CPVRTimerInfoTag::CreateFromEpg(epgTag, false);

    if (!newTimer)
    {
      HELPERS::ShowOKDialogText(
          CVariant{19033}, // "Information"
          bCreateRule ? CVariant{19095} // Timer rule creation failed. Unsupported timer type.
                      : CVariant{19094}); // Timer creation failed. Unsupported timer type.
      return false;
    }
  }

  if (bShowTimerSettings)
  {
    if (!ShowTimerSettings(newTimer))
      return false;
  }

  return AddTimer(newTimer);
}

bool CPVRGUIActionsTimers::AddTimer(const std::shared_ptr<CPVRTimerInfoTag>& item) const
{
  if (!item->Channel() && !item->GetTimerType()->IsEpgBasedTimerRule())
  {
    CLog::LogF(LOGERROR, "No channel given");
    HELPERS::ShowOKDialogText(CVariant{257},
                              CVariant{19109}); // "Error", "Could not save the timer."
    return false;
  }

  if (!item->IsTimerRule() && item->GetEpgInfoTag() && !item->GetEpgInfoTag()->IsRecordable())
  {
    HELPERS::ShowOKDialogText(
        CVariant{19033},
        CVariant{19189}); // "Information", "The PVR backend does not allow to record this event."
    return false;
  }

  if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Parental>().CheckParentalLock(
          item->Channel()) != ParentalCheckResult::SUCCESS)
    return false;

  if (!CServiceBroker::GetPVRManager().Timers()->AddTimer(item))
  {
    HELPERS::ShowOKDialogText(CVariant{257},
                              CVariant{19109}); // "Error", "Could not save the timer"
    return false;
  }

  return true;
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
  explicit InstantRecordingActionSelector(int iInstantRecordTime);
  virtual ~InstantRecordingActionSelector() = default;

  void AddAction(PVRRECORD_INSTANTRECORDACTION eAction, const std::string& title);
  void PreSelectAction(PVRRECORD_INSTANTRECORDACTION eAction);
  PVRRECORD_INSTANTRECORDACTION Select();

private:
  int m_iInstantRecordTime;
  CGUIDialogSelect* m_pDlgSelect; // not owner!
  std::map<PVRRECORD_INSTANTRECORDACTION, int> m_actions;
};

InstantRecordingActionSelector::InstantRecordingActionSelector(int iInstantRecordTime)
  : m_iInstantRecordTime(iInstantRecordTime),
    m_pDlgSelect(CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
        WINDOW_DIALOG_SELECT))
{
  if (m_pDlgSelect)
  {
    m_pDlgSelect->Reset();
    m_pDlgSelect->SetMultiSelection(false);
    m_pDlgSelect->SetHeading(CVariant{19086}); // Instant recording action
  }
  else
  {
    CLog::LogF(LOGERROR, "Unable to obtain WINDOW_DIALOG_SELECT instance");
  }
}

void InstantRecordingActionSelector::AddAction(PVRRECORD_INSTANTRECORDACTION eAction,
                                               const std::string& title)
{
  if (m_actions.find(eAction) == m_actions.end())
  {
    switch (eAction)
    {
      case RECORD_INSTANTRECORDTIME:
        m_pDlgSelect->Add(
            StringUtils::Format(g_localizeStrings.Get(19090),
                                m_iInstantRecordTime)); // Record next <default duration> minutes
        break;
      case RECORD_30_MINUTES:
        m_pDlgSelect->Add(
            StringUtils::Format(g_localizeStrings.Get(19090), 30)); // Record next 30 minutes
        break;
      case RECORD_60_MINUTES:
        m_pDlgSelect->Add(
            StringUtils::Format(g_localizeStrings.Get(19090), 60)); // Record next 60 minutes
        break;
      case RECORD_120_MINUTES:
        m_pDlgSelect->Add(
            StringUtils::Format(g_localizeStrings.Get(19090), 120)); // Record next 120 minutes
        break;
      case RECORD_CURRENT_SHOW:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19091),
                                              title)); // Record current show (<title>)
        break;
      case RECORD_NEXT_SHOW:
        m_pDlgSelect->Add(StringUtils::Format(g_localizeStrings.Get(19092),
                                              title)); // Record next show (<title>)
        break;
      case NONE:
      case ASK:
      default:
        return;
    }

    m_actions.insert(std::make_pair(eAction, static_cast<int>(m_actions.size())));
  }
}

void InstantRecordingActionSelector::PreSelectAction(PVRRECORD_INSTANTRECORDACTION eAction)
{
  const auto& it = m_actions.find(eAction);
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
    const auto it =
        std::find_if(m_actions.cbegin(), m_actions.cend(),
                     [iSelection](const auto& action) { return action.second == iSelection; });

    if (it != m_actions.cend())
      eAction = (*it).first;
  }

  return eAction;
}

} // unnamed namespace

bool CPVRGUIActionsTimers::ToggleRecordingOnPlayingChannel()
{
  const std::shared_ptr<CPVRChannel> channel =
      CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
  if (channel && channel->CanRecord())
    return SetRecordingOnChannel(
        channel, !CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel));

  return false;
}

bool CPVRGUIActionsTimers::SetRecordingOnChannel(const std::shared_ptr<CPVRChannel>& channel,
                                                 bool bOnOff)
{
  bool bReturn = false;

  if (!channel)
    return bReturn;

  if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Parental>().CheckParentalLock(channel) !=
      ParentalCheckResult::SUCCESS)
    return bReturn;

  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(channel->ClientID());
  if (client && client->GetClientCapabilities().SupportsTimers())
  {
    /* timers are supported on this channel */
    if (bOnOff && !CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel))
    {
      std::shared_ptr<CPVREpgInfoTag> epgTag;
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
          const int iDurationDefault =
              m_settings.GetIntValue(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);
          InstantRecordingActionSelector selector(iDurationDefault);
          std::shared_ptr<CPVREpgInfoTag> epgTagNext;

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
            bool bLocked = CServiceBroker::GetPVRManager().IsParentalLocked(epgTag);

            // "now"
            const std::string currentTitle =
                bLocked ? g_localizeStrings.Get(19266) /* Parental locked */ : epgTag->Title();
            selector.AddAction(RECORD_CURRENT_SHOW, currentTitle);
            ePreselect = RECORD_CURRENT_SHOW;

            // "next"
            epgTagNext = channel->GetEPGNext();
            if (epgTagNext)
            {
              const std::string nextTitle = bLocked
                                                ? g_localizeStrings.Get(19266) /* Parental locked */
                                                : epgTagNext->Title();
              selector.AddAction(RECORD_NEXT_SHOW, nextTitle);

              // be smart. if current show is almost over, preselect next show.
              if (epgTag->ProgressPercentage() > 90.0f)
                ePreselect = RECORD_NEXT_SHOW;
            }
          }

          if (ePreselect == RECORD_INSTANTRECORDTIME)
          {
            if (iDurationDefault == 30)
              ePreselect = RECORD_30_MINUTES;
            else if (iDurationDefault == 60)
              ePreselect = RECORD_60_MINUTES;
            else if (iDurationDefault == 120)
              ePreselect = RECORD_120_MINUTES;
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
              CLog::LogF(LOGERROR,
                         "Unknown instant record action selection ({}), defaulting to fixed "
                         "length recording.",
                         static_cast<int>(eSelected));
              epgTag.reset();
              break;
          }
          break;
        }

        default:
          CLog::LogF(LOGERROR,
                     "Unknown instant record action setting value ({}), defaulting to fixed "
                     "length recording.",
                     iAction);
          break;
      }

      const std::shared_ptr<CPVRTimerInfoTag> newTimer(
          epgTag ? CPVRTimerInfoTag::CreateFromEpg(epgTag, false)
                 : CPVRTimerInfoTag::CreateInstantTimerTag(channel, iDuration));

      if (newTimer)
        bReturn = CServiceBroker::GetPVRManager().Timers()->AddTimer(newTimer);

      if (!bReturn)
        HELPERS::ShowOKDialogText(CVariant{257},
                                  CVariant{19164}); // "Error", "Could not start recording."
    }
    else if (!bOnOff && CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel))
    {
      /* delete active timers */
      bReturn =
          CServiceBroker::GetPVRManager().Timers()->DeleteTimersOnChannel(channel, true, true);

      if (!bReturn)
        HELPERS::ShowOKDialogText(CVariant{257},
                                  CVariant{19170}); // "Error", "Could not stop recording."
    }
  }

  return bReturn;
}

bool CPVRGUIActionsTimers::ToggleTimer(const CFileItem& item) const
{
  if (!item.HasEPGInfoTag())
    return false;

  const std::shared_ptr<const CPVRTimerInfoTag> timer(CPVRItem(item).GetTimerInfoTag());
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

bool CPVRGUIActionsTimers::ToggleTimerState(const CFileItem& item) const
{
  if (!item.HasPVRTimerInfoTag())
    return false;

  const std::shared_ptr<CPVRTimerInfoTag> timer = item.GetPVRTimerInfoTag();
  if (timer->IsDisabled())
    timer->SetState(PVR_TIMER_STATE_SCHEDULED);
  else
    timer->SetState(PVR_TIMER_STATE_DISABLED);

  if (CServiceBroker::GetPVRManager().Timers()->UpdateTimer(timer))
    return true;

  HELPERS::ShowOKDialogText(CVariant{257},
                            CVariant{19263}); // "Error", "Could not update the timer."
  return false;
}

bool CPVRGUIActionsTimers::EditTimer(const CFileItem& item) const
{
  const std::shared_ptr<CPVRTimerInfoTag> timer(CPVRItem(item).GetTimerInfoTag());
  if (!timer)
  {
    CLog::LogF(LOGERROR, "No timer!");
    return false;
  }

  // clone the timer.
  const std::shared_ptr<CPVRTimerInfoTag> newTimer(new CPVRTimerInfoTag);
  newTimer->UpdateEntry(timer);

  if (ShowTimerSettings(newTimer) &&
      (!timer->GetTimerType()->IsReadOnly() || timer->GetTimerType()->SupportsEnableDisable()))
  {
    AsyncUpdateTimer asyncUpdate(*this, timer, newTimer);
    return asyncUpdate.Execute();
  }
  return false;
}

bool CPVRGUIActionsTimers::EditTimerRule(const CFileItem& item) const
{
  const std::shared_ptr<CFileItem> parentTimer = GetTimerRule(item);
  if (parentTimer)
    return EditTimer(*parentTimer);

  return false;
}

std::shared_ptr<CFileItem> CPVRGUIActionsTimers::GetTimerRule(const CFileItem& item) const
{
  std::shared_ptr<CPVRTimerInfoTag> timer;
  if (item.HasEPGInfoTag())
    timer = CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(item.GetEPGInfoTag());
  else if (item.HasPVRTimerInfoTag())
    timer = item.GetPVRTimerInfoTag();

  if (timer)
  {
    timer = CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer);
    if (timer)
      return std::make_shared<CFileItem>(timer);
  }
  return {};
}

bool CPVRGUIActionsTimers::DeleteTimer(const CFileItem& item) const
{
  return DeleteTimer(item, false, false);
}

bool CPVRGUIActionsTimers::DeleteTimerRule(const CFileItem& item) const
{
  return DeleteTimer(item, false, true);
}

bool CPVRGUIActionsTimers::DeleteTimer(const CFileItem& item,
                                       bool bIsRecording,
                                       bool bDeleteRule) const
{
  std::shared_ptr<CPVRTimerInfoTag> timer;
  const std::shared_ptr<const CPVRRecording> recording(CPVRItem(item).GetRecording());
  if (recording)
    timer = recording->GetRecordingTimer();

  if (!timer)
    timer = CPVRItem(item).GetTimerInfoTag();

  if (!timer)
  {
    CLog::LogF(LOGERROR, "No timer!");
    return false;
  }

  if (bDeleteRule && !timer->IsTimerRule())
    timer = CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer);

  if (!timer)
  {
    CLog::LogF(LOGERROR, "No timer rule!");
    return false;
  }

  if (bIsRecording)
  {
    if (ConfirmStopRecording(timer))
    {
      if (CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, true, false) ==
          TimerOperationResult::OK)
        return true;

      HELPERS::ShowOKDialogText(CVariant{257},
                                CVariant{19170}); // "Error", "Could not stop recording."
      return false;
    }
  }
  else if (!timer->GetTimerType()->AllowsDelete())
  {
    return false;
  }
  else
  {
    bool bAlsoDeleteRule(false);
    if (ConfirmDeleteTimer(timer, bAlsoDeleteRule))
      return DeleteTimer(timer, false, bAlsoDeleteRule);
  }
  return false;
}

bool CPVRGUIActionsTimers::DeleteTimer(const std::shared_ptr<CPVRTimerInfoTag>& timer,
                                       bool bIsRecording,
                                       bool bDeleteRule) const
{
  TimerOperationResult result =
      CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, bIsRecording, bDeleteRule);
  switch (result)
  {
    case TimerOperationResult::RECORDING:
    {
      // recording running. ask the user if it should be deleted anyway
      if (HELPERS::ShowYesNoDialogText(
              CVariant{122}, // "Confirm delete"
              CVariant{
                  19122}) // "This timer is still recording. Are you sure you want to delete this timer?"
          != HELPERS::DialogResponse::CHOICE_YES)
        return false;

      return DeleteTimer(timer, true, bDeleteRule);
    }
    case TimerOperationResult::OK:
    {
      return true;
    }
    case TimerOperationResult::FAILED:
    {
      HELPERS::ShowOKDialogText(CVariant{257},
                                CVariant{19110}); // "Error", "Could not delete the timer."
      return false;
    }
    default:
    {
      CLog::LogF(LOGERROR, "Unhandled TimerOperationResult ({})!", static_cast<int>(result));
      break;
    }
  }
  return false;
}

bool CPVRGUIActionsTimers::ConfirmDeleteTimer(const std::shared_ptr<const CPVRTimerInfoTag>& timer,
                                              bool& bDeleteRule) const
{
  bool bConfirmed(false);
  const std::shared_ptr<const CPVRTimerInfoTag> parentTimer(
      CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer));

  if (parentTimer && parentTimer->GetTimerType()->AllowsDelete())
  {
    // timer was scheduled by a deletable timer rule. prompt user for confirmation for deleting the timer rule, including scheduled timers.
    bool bCancel(false);
    bDeleteRule = CGUIDialogYesNo::ShowAndGetInput(
        CVariant{122}, // "Confirm delete"
        CVariant{
            840}, // "Do you want to delete only this timer or also the timer rule that has scheduled it?"
        CVariant{""}, CVariant{timer->Title()}, bCancel, CVariant{841}, // "Only this"
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
            ? CVariant{845}
            // "Are you sure you want to delete this timer rule and all timers it has scheduled?"
            : CVariant{846}, // "Are you sure you want to delete this timer?"
        CVariant{""}, CVariant{timer->Title()});
  }

  return bConfirmed;
}

bool CPVRGUIActionsTimers::StopRecording(const CFileItem& item) const
{
  if (!DeleteTimer(item, true, false))
    return false;

  CServiceBroker::GetPVRManager().TriggerRecordingsUpdate();
  return true;
}

bool CPVRGUIActionsTimers::ConfirmStopRecording(
    const std::shared_ptr<const CPVRTimerInfoTag>& timer) const
{
  return CGUIDialogYesNo::ShowAndGetInput(
      CVariant{847}, // "Confirm stop recording"
      CVariant{848}, // "Are you sure you want to stop this recording?"
      CVariant{""}, CVariant{timer->Title()});
}

namespace
{
std::string GetAnnouncerText(const std::shared_ptr<const CPVRTimerInfoTag>& timer,
                             int idEpg,
                             int idNoEpg)
{
  std::string text;
  if (timer->IsEpgBased())
  {
    text = StringUtils::Format(g_localizeStrings.Get(idEpg),
                               timer->Title(), // tv show title
                               timer->ChannelName(),
                               timer->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
  }
  else
  {
    text = StringUtils::Format(g_localizeStrings.Get(idNoEpg), timer->ChannelName(),
                               timer->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
  }
  return text;
}

void AddEventLogEntry(const std::shared_ptr<const CPVRTimerInfoTag>& timer, int idEpg, int idNoEpg)
{
  std::string name;
  std::string icon;

  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(timer->GetTimerType()->GetClientId());
  if (client)
  {
    name = client->GetFullClientName();
    icon = client->Icon();
  }
  else
  {
    name = g_sysinfo.GetAppName();
    icon = "special://xbmc/media/icon256x256.png";
  }

  CPVREventLogJob* job = new CPVREventLogJob;
  job->AddEvent(false, // do not display a toast, only log event
                EventLevel::Information, // info, no error
                name, GetAnnouncerText(timer, idEpg, idNoEpg), icon);
  CServiceBroker::GetJobManager()->AddJob(job, nullptr);
}
} // unnamed namespace

void CPVRGUIActionsTimers::AnnounceReminder(const std::shared_ptr<CPVRTimerInfoTag>& timer) const
{
  if (!timer->IsReminder())
  {
    CLog::LogF(LOGERROR, "No reminder timer!");
    return;
  }

  if (timer->EndAsUTC() < CDateTime::GetUTCDateTime())
  {
    // expired. timer end is in the past. write event log entry.
    AddEventLogEntry(timer, 19305, 19306); // Deleted missed PVR reminder ...
    return;
  }

  if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingChannel(timer->Channel()))
  {
    // no need for an announcement. channel in question is already playing.
    return;
  }

  // show the reminder dialog
  CGUIDialogProgress* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
          WINDOW_DIALOG_PROGRESS);
  if (!dialog)
    return;

  dialog->Reset();

  dialog->SetHeading(CVariant{19312}); // "PVR reminder"
  dialog->ShowChoice(0, CVariant{19165}); // "Switch"

  std::string text = GetAnnouncerText(timer, 19307, 19308); // Reminder for ...

  bool bCanRecord = false;
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(timer->ClientID());
  if (client && client->GetClientCapabilities().SupportsTimers())
  {
    bCanRecord = true;
    dialog->ShowChoice(1, CVariant{264}); // "Record"
    dialog->ShowChoice(2, CVariant{222}); // "Cancel"

    if (m_settings.GetBoolValue(CSettings::SETTING_PVRREMINDERS_AUTORECORD))
      text += "\n\n" + g_localizeStrings.Get(
                           19309); // (Auto-close of this reminder will schedule a recording...)
    else if (m_settings.GetBoolValue(CSettings::SETTING_PVRREMINDERS_AUTOSWITCH))
      text += "\n\n" + g_localizeStrings.Get(
                           19331); // (Auto-close of this reminder will switch to channel...)
  }
  else
  {
    dialog->ShowChoice(1, CVariant{222}); // "Cancel"
  }

  dialog->SetText(text);
  dialog->SetPercentage(100);

  dialog->Open();

  int result = CGUIDialogProgress::CHOICE_NONE;

  static constexpr int PROGRESS_TIMESLICE_MILLISECS = 50;

  const int iWait = m_settings.GetIntValue(CSettings::SETTING_PVRREMINDERS_AUTOCLOSEDELAY) * 1000;
  int iRemaining = iWait;
  while (iRemaining > 0)
  {
    result = dialog->GetChoice();
    if (result != CGUIDialogProgress::CHOICE_NONE)
      break;

    std::this_thread::sleep_for(std::chrono::milliseconds(PROGRESS_TIMESLICE_MILLISECS));

    iRemaining -= PROGRESS_TIMESLICE_MILLISECS;
    dialog->SetPercentage(iRemaining * 100 / iWait);
    dialog->Progress();
  }

  dialog->Close();

  bool bAutoClosed = (iRemaining <= 0);
  bool bSwitch = (result == 0);
  bool bRecord = (result == 1);

  if (bAutoClosed)
  {
    bRecord = (bCanRecord && m_settings.GetBoolValue(CSettings::SETTING_PVRREMINDERS_AUTORECORD));
    bSwitch = m_settings.GetBoolValue(CSettings::SETTING_PVRREMINDERS_AUTOSWITCH);
  }

  if (bRecord)
  {
    std::shared_ptr<CPVRTimerInfoTag> newTimer;

    std::shared_ptr<CPVREpgInfoTag> epgTag = timer->GetEpgInfoTag();
    if (epgTag)
    {
      newTimer = CPVRTimerInfoTag::CreateFromEpg(epgTag, false);
      if (newTimer)
      {
        // an epgtag can only have max one timer - we need to clear the reminder to be able to
        // attach the recording timer
        DeleteTimer(timer, false, false);
      }
    }
    else
    {
      int iDuration = (timer->EndAsUTC() - timer->StartAsUTC()).GetSecondsTotal() / 60;
      newTimer = CPVRTimerInfoTag::CreateTimerTag(timer->Channel(), timer->StartAsUTC(), iDuration);
    }

    if (newTimer)
    {
      // schedule recording
      AddTimer(CFileItem(newTimer), false);
    }

    if (bAutoClosed)
    {
      AddEventLogEntry(timer, 19310,
                       19311); // Scheduled recording for auto-closed PVR reminder ...
    }
  }

  if (bSwitch)
  {
    const std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(
            timer->Channel());
    if (groupMember)
    {
      CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().SwitchToChannel(
          CFileItem(groupMember), false);

      if (bAutoClosed)
      {
        AddEventLogEntry(timer, 19332,
                         19333); // Switched channel for auto-closed PVR reminder ...
      }
    }
  }
}

void CPVRGUIActionsTimers::AnnounceReminders() const
{
  // Prevent multiple yesno dialogs, all on same call stack, due to gui message processing while dialog is open.
  if (m_bReminderAnnouncementRunning)
    return;

  m_bReminderAnnouncementRunning = true;
  std::shared_ptr<CPVRTimerInfoTag> timer =
      CServiceBroker::GetPVRManager().Timers()->GetNextReminderToAnnnounce();
  while (timer)
  {
    AnnounceReminder(timer);
    timer = CServiceBroker::GetPVRManager().Timers()->GetNextReminderToAnnnounce();
  }
  m_bReminderAnnouncementRunning = false;
}
