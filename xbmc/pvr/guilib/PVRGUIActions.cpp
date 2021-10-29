/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActions.h"

#include "Application.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/IDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "network/Network.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVREventLogJob.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/PVRStreamProperties.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClientMenuHooks.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/dialogs/GUIDialogPVRChannelGuide.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingSettings.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgSearchFilter.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVRSearch.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "threads/IRunnable.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <chrono>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace KODI::MESSAGING;

namespace
{
PVR::CGUIWindowPVRSearchBase* GetSearchWindow(bool bRadio)
{
  const int windowSearchId = bRadio ? WINDOW_RADIO_SEARCH : WINDOW_TV_SEARCH;

  PVR::CGUIWindowPVRSearchBase* windowSearch;

  CGUIWindowManager& windowMgr = CServiceBroker::GetGUI()->GetWindowManager();
  if (bRadio)
    windowSearch = windowMgr.GetWindow<PVR::CGUIWindowPVRRadioSearch>(windowSearchId);
  else
    windowSearch = windowMgr.GetWindow<PVR::CGUIWindowPVRTVSearch>(windowSearchId);

  if (!windowSearch)
    CLog::LogF(LOGERROR, "Unable to get {}!", bRadio ? "WINDOW_RADIO_SEARCH" : "WINDOW_TV_SEARCH");

  return windowSearch;
}
} // unnamed namespace

namespace PVR
{
  class AsyncRecordingAction : private IRunnable
  {
  public:
    bool Execute(const CFileItemPtr& item);

  protected:
    AsyncRecordingAction() = default;

  private:
    // IRunnable implementation
    void Run() override;

    // the worker function
    virtual bool DoRun(const CFileItemPtr& item) = 0;

    CFileItemPtr m_item;
    bool m_bSuccess = false;
  };

  bool AsyncRecordingAction::Execute(const CFileItemPtr& item)
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
    explicit AsyncRenameRecording(const std::string& strNewName) : m_strNewName(strNewName) {}

  private:
    bool DoRun(const std::shared_ptr<CFileItem>& item) override
    {
      if (item->IsUsablePVRRecording())
      {
        return item->GetPVRRecordingInfoTag()->Rename(m_strNewName);
      }
      else
      {
        CLog::LogF(LOGERROR, "Cannot rename item '{}': no valid recording tag", item->GetPath());
        return false;
      }
    }
    std::string m_strNewName;
  };

  class AsyncDeleteRecording : public AsyncRecordingAction
  {
  public:
    explicit AsyncDeleteRecording(bool bWatchedOnly = false) : m_bWatchedOnly(bWatchedOnly) {}

  private:
    bool DoRun(const std::shared_ptr<CFileItem>& item) override
    {
      CFileItemList items;
      if (item->m_bIsFolder)
      {
        CUtil::GetRecursiveListing(item->GetPath(), items, "", XFILE::DIR_FLAG_NO_FILE_INFO);
      }
      else
      {
        items.Add(item);
      }

      bool bReturn = true;
      for (const auto& itemToDelete : items)
      {
        if (itemToDelete->IsPVRRecording() &&
            (!m_bWatchedOnly || itemToDelete->GetPVRRecordingInfoTag()->GetPlayCount() > 0))
          bReturn &= itemToDelete->GetPVRRecordingInfoTag()->Delete();
      }
      return bReturn;
    }
    bool m_bWatchedOnly = false;
  };

  class AsyncEmptyRecordingsTrash : public AsyncRecordingAction
  {
  private:
    bool DoRun(const std::shared_ptr<CFileItem>& item) override
    {
      return CServiceBroker::GetPVRManager().Clients()->DeleteAllRecordingsFromTrash() == PVR_ERROR_NO_ERROR;
    }
  };

  class AsyncUndeleteRecording : public AsyncRecordingAction
  {
  private:
    bool DoRun(const std::shared_ptr<CFileItem>& item) override
    {
      if (item->IsDeletedPVRRecording())
      {
        return item->GetPVRRecordingInfoTag()->Undelete();
      }
      else
      {
        CLog::LogF(LOGERROR, "Cannot undelete item '{}': no valid recording tag", item->GetPath());
        return false;
      }
    }
  };

  class AsyncSetRecordingPlayCount : public AsyncRecordingAction
  {
  private:
    bool DoRun(const CFileItemPtr& item) override
    {
      const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
      if (client)
      {
        const std::shared_ptr<CPVRRecording> recording = item->GetPVRRecordingInfoTag();
        return client->SetRecordingPlayCount(*recording, recording->GetLocalPlayCount()) == PVR_ERROR_NO_ERROR;
      }
      return false;
    }
  };

  class AsyncSetRecordingLifetime : public AsyncRecordingAction
  {
  private:
    bool DoRun(const CFileItemPtr& item) override
    {
      const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
      if (client)
        return client->SetRecordingLifetime(*item->GetPVRRecordingInfoTag()) == PVR_ERROR_NO_ERROR;
      return false;
    }
  };

  CPVRGUIActions::CPVRGUIActions()
    : m_settings({CSettings::SETTING_LOOKANDFEEL_STARTUPACTION,
                  CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL,
                  CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME,
                  CSettings::SETTING_PVRRECORD_INSTANTRECORDACTION,
                  CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH,
                  CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES,
                  CSettings::SETTING_PVRPARENTAL_PIN, CSettings::SETTING_PVRPARENTAL_ENABLED,
                  CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME,
                  CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME,
                  CSettings::SETTING_PVRREMINDERS_AUTOCLOSEDELAY,
                  CSettings::SETTING_PVRREMINDERS_AUTORECORD,
                  CSettings::SETTING_PVRREMINDERS_AUTOSWITCH})
  {
  }

  bool CPVRGUIActions::ShowEPGInfo(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CPVRChannel> channel(CPVRItem(item).GetChannel());
    if (channel && CheckParentalLock(channel) != ParentalCheckResult::SUCCESS)
      return false;

    const std::shared_ptr<CPVREpgInfoTag> epgTag(CPVRItem(item).GetEpgInfoTag());
    if (!epgTag)
    {
      CLog::LogF(LOGERROR, "No epg tag!");
      return false;
    }

    CGUIDialogPVRGuideInfo* pDlgInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRGuideInfo>(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_GUIDE_INFO!");
      return false;
    }

    pDlgInfo->SetProgInfo(epgTag);
    pDlgInfo->Open();
    return true;
  }


  bool CPVRGUIActions::ShowChannelEPG(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CPVRChannel> channel(CPVRItem(item).GetChannel());
    if (channel && CheckParentalLock(channel) != ParentalCheckResult::SUCCESS)
      return false;

    CGUIDialogPVRChannelGuide* pDlgInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRChannelGuide>(WINDOW_DIALOG_PVR_CHANNEL_GUIDE);
    if (!pDlgInfo)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_CHANNEL_GUIDE!");
      return false;
    }

    pDlgInfo->Open(channel);
    return true;
  }


  bool CPVRGUIActions::ShowRecordingInfo(const CFileItemPtr& item) const
  {
    if (!item->IsPVRRecording())
    {
      CLog::LogF(LOGERROR, "No recording!");
      return false;
    }

    CGUIDialogPVRRecordingInfo* pDlgInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRRecordingInfo>(WINDOW_DIALOG_PVR_RECORDING_INFO);
    if (!pDlgInfo)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_RECORDING_INFO!");
      return false;
    }

    pDlgInfo->SetRecording(item.get());
    pDlgInfo->Open();
    return true;
  }

  bool CPVRGUIActions::FindSimilar(const std::shared_ptr<CFileItem>& item) const
  {
    CGUIWindowPVRSearchBase* windowSearch = GetSearchWindow(CPVRItem(item).IsRadio());
    if (!windowSearch)
      return false;

    //! @todo If we want dialogs to spawn program search in a clean way - without having to force-close any
    //        other dialogs - we must introduce a search dialog with functionality similar to the search window.

    for (int iId = CServiceBroker::GetGUI()->GetWindowManager().GetTopmostModalDialog(true /* ignoreClosing */);
         iId != WINDOW_INVALID;
         iId = CServiceBroker::GetGUI()->GetWindowManager().GetTopmostModalDialog(true /* ignoreClosing */))
    {
      CLog::LogF(LOGWARNING,
                 "Have to close modal dialog with id {} before search window can be opened.", iId);

      CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(iId);
      if (window)
      {
        window->Close();
      }
      else
      {
        CLog::LogF(LOGERROR, "Unable to get window instance {}! Cannot open search window.", iId);
        return false; // return, otherwise we run into an endless loop
      }
    }

    windowSearch->SetItemToSearch(item);
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(windowSearch->GetID());
    return true;
  };

  bool CPVRGUIActions::ShowTimerSettings(const std::shared_ptr<CPVRTimerInfoTag>& timer) const
  {
    CGUIDialogPVRTimerSettings* pDlgInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRTimerSettings>(WINDOW_DIALOG_PVR_TIMER_SETTING);
    if (!pDlgInfo)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_TIMER_SETTING!");
      return false;
    }

    pDlgInfo->SetTimer(timer);
    pDlgInfo->Open();

    return pDlgInfo->IsConfirmed();
  }

  bool CPVRGUIActions::AddReminder(const std::shared_ptr<CFileItem>& item) const
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

    const std::shared_ptr<CPVRTimerInfoTag> newTimer = CPVRTimerInfoTag::CreateReminderFromEpg(epgTag);
    if (!newTimer)
    {
      HELPERS::ShowOKDialogText(CVariant{19033}, // "Information"
                                CVariant{19094}); // Timer creation failed. Unsupported timer type.
      return false;
    }

    return AddTimer(newTimer);
 }

  bool CPVRGUIActions::AddTimer(bool bRadio) const
  {
    const std::shared_ptr<CPVRTimerInfoTag> newTimer(new CPVRTimerInfoTag(bRadio));
    if (ShowTimerSettings(newTimer))
    {
      return AddTimer(newTimer);
    }
    return false;
  }

  bool CPVRGUIActions::AddTimer(const CFileItemPtr& item, bool bShowTimerSettings) const
  {
    return AddTimer(item, false, bShowTimerSettings, false);
  }

  bool CPVRGUIActions::AddTimerRule(const std::shared_ptr<CFileItem>& item, bool bShowTimerSettings, bool bFallbackToOneShotTimer) const
  {
    return AddTimer(item, true, bShowTimerSettings, bFallbackToOneShotTimer);
  }

  bool CPVRGUIActions::AddTimer(const std::shared_ptr<CFileItem>& item, bool bCreateRule, bool bShowTimerSettings, bool bFallbackToOneShotTimer) const
  {
    const std::shared_ptr<CPVRChannel> channel(CPVRItem(item).GetChannel());
    if (!channel)
    {
      CLog::LogF(LOGERROR, "No channel!");
      return false;
    }

    if (CheckParentalLock(channel) != ParentalCheckResult::SUCCESS)
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

    std::shared_ptr<CPVRTimerInfoTag> timer(bCreateRule || !epgTag ? nullptr : CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(epgTag));
    std::shared_ptr<CPVRTimerInfoTag> rule (bCreateRule ? CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer) : nullptr);
    if (timer || rule)
    {
      HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19034}); // "Information", "There is already a timer set for this event"
      return false;
    }

    std::shared_ptr<CPVRTimerInfoTag> newTimer(epgTag ? CPVRTimerInfoTag::CreateFromEpg(epgTag, bCreateRule) : CPVRTimerInfoTag::CreateInstantTimerTag(channel));
    if (!newTimer)
    {
      if (bCreateRule && bFallbackToOneShotTimer)
        newTimer = CPVRTimerInfoTag::CreateFromEpg(epgTag, false);

      if (!newTimer)
      {
        HELPERS::ShowOKDialogText(CVariant{19033}, // "Information"
                                  bCreateRule
                                    ? CVariant{19095} // Timer rule creation failed. Unsupported timer type.
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

  bool CPVRGUIActions::AddTimer(const std::shared_ptr<CPVRTimerInfoTag>& item) const
  {
    if (!item->Channel() && !item->GetTimerType()->IsEpgBasedTimerRule())
    {
      CLog::LogF(LOGERROR, "No channel given");
      HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19109}); // "Error", "Could not save the timer. Check the log for more information about this message."
      return false;
    }

    if (!item->IsTimerRule() && item->GetEpgInfoTag() && !item->GetEpgInfoTag()->IsRecordable())
    {
      HELPERS::ShowOKDialogText(CVariant{19033}, CVariant{19189}); // "Information", "The PVR backend does not allow to record this event."
      return false;
    }

    if (CheckParentalLock(item->Channel()) != ParentalCheckResult::SUCCESS)
      return false;

    if (!CServiceBroker::GetPVRManager().Timers()->AddTimer(item))
    {
      HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19109}); // "Error", "Could not save the timer. Check the log for more information about this message."
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
      m_pDlgSelect(CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT))
    {
      if (m_pDlgSelect)
      {
        m_pDlgSelect->SetMultiSelection(false);
        m_pDlgSelect->SetHeading(CVariant{19086}); // Instant recording action
      }
      else
      {
        CLog::LogF(LOGERROR, "Unable to obtain WINDOW_DIALOG_SELECT instance");
      }
    }

    void InstantRecordingActionSelector::AddAction(PVRRECORD_INSTANTRECORDACTION eAction, const std::string& title)
    {
      if (m_actions.find(eAction) == m_actions.end())
      {
        switch (eAction)
        {
          case RECORD_INSTANTRECORDTIME:
            m_pDlgSelect->Add(StringUtils::Format(
                g_localizeStrings.Get(19090),
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
        for (const auto& action : m_actions)
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

  bool CPVRGUIActions::ToggleRecordingOnPlayingChannel()
  {
    const std::shared_ptr<CPVRChannel> channel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
    if (channel && channel->CanRecord())
      return SetRecordingOnChannel(channel, !CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel));

    return false;
  }

  bool CPVRGUIActions::SetRecordingOnChannel(const std::shared_ptr<CPVRChannel>& channel, bool bOnOff)
  {
    bool bReturn = false;

    if (!channel)
      return bReturn;

    if (CheckParentalLock(channel) != ParentalCheckResult::SUCCESS)
      return bReturn;

    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(channel->ClientID());
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
            const int iDurationDefault = m_settings.GetIntValue(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);
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
              const std::string currentTitle = bLocked ? g_localizeStrings.Get(19266) /* Parental locked */ : epgTag->Title();
              selector.AddAction(RECORD_CURRENT_SHOW, currentTitle);
              ePreselect = RECORD_CURRENT_SHOW;

              // "next"
              epgTagNext = channel->GetEPGNext();
              if (epgTagNext)
              {
                const std::string nextTitle = bLocked ? g_localizeStrings.Get(19266) /* Parental locked */ : epgTagNext->Title();
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

        const std::shared_ptr<CPVRTimerInfoTag> newTimer(epgTag ? CPVRTimerInfoTag::CreateFromEpg(epgTag, false) : CPVRTimerInfoTag::CreateInstantTimerTag(channel, iDuration));

        if (newTimer)
          bReturn = CServiceBroker::GetPVRManager().Timers()->AddTimer(newTimer);

        if (!bReturn)
          HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19164}); // "Error", "Could not start recording. Check the log for more information about this message."
      }
      else if (!bOnOff && CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*channel))
      {
        /* delete active timers */
        bReturn = CServiceBroker::GetPVRManager().Timers()->DeleteTimersOnChannel(channel, true, true);

        if (!bReturn)
          HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19170}); // "Error", "Could not stop recording. Check the log for more information about this message."
      }
    }

    return bReturn;
  }

  bool CPVRGUIActions::ToggleTimer(const CFileItemPtr& item) const
  {
    if (!item->HasEPGInfoTag())
      return false;

    const std::shared_ptr<CPVRTimerInfoTag> timer(CPVRItem(item).GetTimerInfoTag());
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

  bool CPVRGUIActions::ToggleTimerState(const CFileItemPtr& item) const
  {
    if (!item->HasPVRTimerInfoTag())
      return false;

    const std::shared_ptr<CPVRTimerInfoTag> timer(item->GetPVRTimerInfoTag());
    if (timer->m_state == PVR_TIMER_STATE_DISABLED)
      timer->m_state = PVR_TIMER_STATE_SCHEDULED;
    else
      timer->m_state = PVR_TIMER_STATE_DISABLED;

    if (CServiceBroker::GetPVRManager().Timers()->UpdateTimer(timer))
      return true;

    HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19263}); // "Error", "Could not update the timer. Check the log for more information about this message."
    return false;
  }

  bool CPVRGUIActions::EditTimer(const CFileItemPtr& item) const
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

    if (ShowTimerSettings(newTimer) && (!timer->GetTimerType()->IsReadOnly() || timer->GetTimerType()->SupportsEnableDisable()))
    {
      if (newTimer->GetTimerType() == timer->GetTimerType())
      {
        if (CServiceBroker::GetPVRManager().Timers()->UpdateTimer(newTimer))
          return true;

        HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19263}); // "Error", "Could not update the timer. Check the log for more information about this message."
        return false;
      }
      else
      {
        // timer type changed. delete the original timer, then create the new timer. this order is
        // important. for instance, the new timer might be a rule which schedules the original timer.
        // deleting the original timer after creating the rule would do literally this and we would
        // end up with one timer missing wrt to the rule defined by the new timer.
        if (DeleteTimer(timer, timer->IsRecording(), false))
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

  bool CPVRGUIActions::EditTimerRule(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CFileItem> parentTimer = GetTimerRule(item);
    if (parentTimer)
      return EditTimer(parentTimer);

    return false;
  }

  std::shared_ptr<CFileItem> CPVRGUIActions::GetTimerRule(const std::shared_ptr<CFileItem>& item) const
  {
    std::shared_ptr<CPVRTimerInfoTag> timer;
    if (item && item->HasEPGInfoTag())
      timer = CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(item->GetEPGInfoTag());
    else if (item && item->HasPVRTimerInfoTag())
      timer = item->GetPVRTimerInfoTag();

    if (timer)
    {
      timer = CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer);
      if (timer)
        return std::make_shared<CFileItem>(timer);
    }
    return {};
  }

  bool CPVRGUIActions::DeleteTimer(const CFileItemPtr& item) const
  {
    return DeleteTimer(item, false, false);
  }

  bool CPVRGUIActions::DeleteTimerRule(const CFileItemPtr& item) const
  {
    return DeleteTimer(item, false, true);
  }

  bool CPVRGUIActions::DeleteTimer(const CFileItemPtr& item, bool bIsRecording, bool bDeleteRule) const
  {
    std::shared_ptr<CPVRTimerInfoTag> timer;
    const std::shared_ptr<CPVRRecording> recording(CPVRItem(item).GetRecording());
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
        if (CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, true, false) == TimerOperationResult::OK)
          return true;

        HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19170}); // "Error", "Could not stop recording. Check the log for more information about this message."
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

  bool CPVRGUIActions::DeleteTimer(const std::shared_ptr<CPVRTimerInfoTag>& timer, bool bIsRecording, bool bDeleteRule) const
  {
    TimerOperationResult result = CServiceBroker::GetPVRManager().Timers()->DeleteTimer(timer, bIsRecording, bDeleteRule);
    switch (result)
    {
      case TimerOperationResult::RECORDING:
      {
        // recording running. ask the user if it should be deleted anyway
        if (HELPERS::ShowYesNoDialogText(CVariant{122},   // "Confirm delete"
                                         CVariant{19122}) // "This timer is still recording. Are you sure you want to delete this timer?"
            != HELPERS::DialogResponse::YES)
          return false;

        return DeleteTimer(timer, true, bDeleteRule);
      }
      case TimerOperationResult::OK:
      {
        return true;
      }
      case TimerOperationResult::FAILED:
      {
        HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19110}); // "Error", "Could not delete the timer. Check the log for more information about this message."
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

  bool CPVRGUIActions::ConfirmDeleteTimer(const std::shared_ptr<CPVRTimerInfoTag>& timer, bool& bDeleteRule) const
  {
    bool bConfirmed(false);
    const std::shared_ptr<CPVRTimerInfoTag> parentTimer(CServiceBroker::GetPVRManager().Timers()->GetTimerRule(timer));

    if (parentTimer && parentTimer->GetTimerType()->AllowsDelete())
    {
      // timer was scheduled by a deletable timer rule. prompt user for confirmation for deleting the timer rule, including scheduled timers.
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

  bool CPVRGUIActions::StopRecording(const CFileItemPtr& item) const
  {
    if (!DeleteTimer(item, true, false))
      return false;

    CServiceBroker::GetPVRManager().TriggerRecordingsUpdate();
    return true;
  }

  bool CPVRGUIActions::ConfirmStopRecording(const std::shared_ptr<CPVRTimerInfoTag>& timer) const
  {
    return CGUIDialogYesNo::ShowAndGetInput(CVariant{847}, // "Confirm stop recording"
                                            CVariant{848}, // "Are you sure you want to stop this recording?"
                                            CVariant{""},
                                            CVariant{timer->Title()});
  }

  bool CPVRGUIActions::EditRecording(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CPVRRecording> recording = CPVRItem(item).GetRecording();
    if (!recording)
    {
      CLog::LogF(LOGERROR, "No recording!");
      return false;
    }

    std::shared_ptr<CPVRRecording> origRecording(new CPVRRecording);
    origRecording->Update(*recording,
                          *CServiceBroker::GetPVRManager().GetClient(recording->m_iClientId));

    if (!ShowRecordingSettings(recording))
      return false;

    if (origRecording->m_strTitle != recording->m_strTitle)
    {
      if (!AsyncRenameRecording(recording->m_strTitle).Execute(item))
        CLog::LogF(LOGERROR, "Renaming recording failed!");
    }

    if (origRecording->GetLocalPlayCount() != recording->GetLocalPlayCount())
    {
      if (!AsyncSetRecordingPlayCount().Execute(item))
        CLog::LogF(LOGERROR, "Setting recording playcount failed!");
    }

    if (origRecording->m_iLifetime != recording->m_iLifetime)
    {
      if (!AsyncSetRecordingLifetime().Execute(item))
        CLog::LogF(LOGERROR, "Setting recording lifetime failed!");
    }

    return true;
  }

  bool CPVRGUIActions::CanEditRecording(const CFileItem& item) const
  {
    return CGUIDialogPVRRecordingSettings::CanEditRecording(item);
  }

  bool CPVRGUIActions::DeleteRecording(const CFileItemPtr& item) const
  {
    if ((!item->IsPVRRecording() && !item->m_bIsFolder) || item->IsParentFolder())
      return false;

    if (!ConfirmDeleteRecording(item))
      return false;

    if (!AsyncDeleteRecording().Execute(item))
    {
      HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19111}); // "Error", "PVR backend error. Check the log for more information about this message."
      return false;
    }

    return true;
  }

  bool CPVRGUIActions::ConfirmDeleteRecording(const CFileItemPtr& item) const
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

  bool CPVRGUIActions::DeleteWatchedRecordings(const std::shared_ptr<CFileItem>& item) const
  {
    if (!item->m_bIsFolder || item->IsParentFolder())
      return false;

    if (!ConfirmDeleteWatchedRecordings(item))
      return false;

    if (!AsyncDeleteRecording(true).Execute(item))
    {
      HELPERS::ShowOKDialogText(
          CVariant{257},
          CVariant{
              19111}); // "Error", "PVR backend error. Check the log for more information about this message."
      return false;
    }

    return true;
  }

  bool CPVRGUIActions::ConfirmDeleteWatchedRecordings(const std::shared_ptr<CFileItem>& item) const
  {
    return CGUIDialogYesNo::ShowAndGetInput(
        CVariant{122}, // "Confirm delete"
        CVariant{19328}, // "Delete all watched recordings in this folder?"
        CVariant{""}, CVariant{item->GetLabel()});
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

  bool CPVRGUIActions::UndeleteRecording(const CFileItemPtr& item) const
  {
    if (!item->IsDeletedPVRRecording())
      return false;

    if (!AsyncUndeleteRecording().Execute(item))
    {
      HELPERS::ShowOKDialogText(CVariant{257}, CVariant{19111}); // "Error", "PVR backend error. Check the log for more information about this message."
      return false;
    }

    return true;
  }

  bool CPVRGUIActions::ShowRecordingSettings(const std::shared_ptr<CPVRRecording>& recording) const
  {
    CGUIDialogPVRRecordingSettings* pDlgInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRRecordingSettings>(WINDOW_DIALOG_PVR_RECORDING_SETTING);
    if (!pDlgInfo)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_RECORDING_SETTING!");
      return false;
    }

    pDlgInfo->SetRecording(recording);
    pDlgInfo->Open();

    return pDlgInfo->IsConfirmed();
  }

  std::string CPVRGUIActions::GetResumeLabel(const CFileItem& item) const
  {
    std::string resumeString;

    const std::shared_ptr<CPVRRecording> recording(CPVRItem(CFileItemPtr(new CFileItem(item))).GetRecording());
    if (recording && !recording->IsDeleted())
    {
      int positionInSeconds = lrint(recording->GetResumePoint().timeInSeconds);
      if (positionInSeconds > 0)
        resumeString = StringUtils::Format(
            g_localizeStrings.Get(12022),
            StringUtils::SecondsToTimeString(positionInSeconds, TIME_FORMAT_HH_MM_SS));
    }
    return resumeString;
  }

  bool CPVRGUIActions::CheckResumeRecording(const CFileItemPtr& item) const
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

  bool CPVRGUIActions::ResumePlayRecording(const CFileItemPtr& item, bool bFallbackToPlay) const
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
    CMediaSettings::GetInstance().SetMediaStartWindowed(!bFullscreen);

    if (bFullscreen)
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
    }
  }

  void CPVRGUIActions::StartPlayback(CFileItem* item,
                                     bool bFullscreen,
                                     CPVRStreamProperties* epgProps) const
  {
    // Obtain dynamic playback url and properties from the respective pvr client
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
    if (client)
    {
      CPVRStreamProperties props;

      if (item->IsPVRChannel())
      {
        // If this was an EPG Tag to be played as live then PlayEpgTag() will create a channel
        // fileitem instead and pass the epg tags props so we use those and skip the client call
        if (epgProps)
          props = *epgProps;
        else
          client->GetChannelStreamProperties(item->GetPVRChannelInfoTag(), props);
      }
      else if (item->IsPVRRecording())
      {
        client->GetRecordingStreamProperties(item->GetPVRRecordingInfoTag(), props);
      }
      else if (item->IsEPG())
      {
        if (epgProps) // we already have props from PlayEpgTag()
          props = *epgProps;
        else
          client->GetEpgTagStreamProperties(item->GetEPGInfoTag(), props);
      }

      if (props.size())
      {
        const std::string url = props.GetStreamURL();
        if (!url.empty())
          item->SetDynPath(url);

        const std::string mime = props.GetStreamMimeType();
        if (!mime.empty())
        {
          item->SetMimeType(mime);
          item->SetContentLookup(false);
        }

        for (const auto& prop : props)
          item->SetProperty(prop.first, prop.second);
      }
    }

    CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(item));
    CheckAndSwitchToFullscreen(bFullscreen);
  }

  bool CPVRGUIActions::PlayRecording(const CFileItemPtr& item, bool bCheckResume) const
  {
    const std::shared_ptr<CPVRRecording> recording(CPVRItem(item).GetRecording());
    if (!recording)
      return false;

    if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRecording(recording))
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      return true;
    }

    if (!bCheckResume || CheckResumeRecording(item))
    {
      CFileItem* itemToPlay = new CFileItem(recording);
      itemToPlay->m_lStartOffset = item->m_lStartOffset;
      StartPlayback(itemToPlay, true);
    }
    return true;
  }

  bool CPVRGUIActions::PlayEpgTag(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CPVREpgInfoTag> epgTag(CPVRItem(item).GetEpgInfoTag());
    if (!epgTag)
      return false;

    if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingEpgTag(epgTag))
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0,
                      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      return true;
    }

    // Obtain dynamic playback url and properties from the respective pvr client
    const std::shared_ptr<CPVRClient> client =
        CServiceBroker::GetPVRManager().GetClient(epgTag->ClientID());
    if (!client)
      return false;

    CPVRStreamProperties props;
    client->GetEpgTagStreamProperties(epgTag, props);

    CFileItem* itemToPlay = nullptr;
    if (props.EPGPlaybackAsLive())
    {
      const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMember(*item);
      if (!groupMember)
        return false;

      itemToPlay = new CFileItem(groupMember);
    }
    else
    {
      itemToPlay = new CFileItem(epgTag);
    }

    StartPlayback(itemToPlay, true, &props);
    return true;
  }

  bool CPVRGUIActions::SwitchToChannel(const CFileItemPtr& item, bool bCheckResume) const
  {
    if (item->m_bIsFolder)
      return false;

    std::shared_ptr<CPVRRecording> recording;
    const std::shared_ptr<CPVRChannel> channel(CPVRItem(item).GetChannel());
    if (channel)
    {
      bool bSwitchToFullscreen = CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingChannel(channel);

      if (!bSwitchToFullscreen)
      {
        recording = CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(channel->GetEPGNow());
        bSwitchToFullscreen = recording && CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRecording(recording);
      }

      if (bSwitchToFullscreen)
      {
        CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
        return true;
      }
    }

    ParentalCheckResult result = channel ? CheckParentalLock(channel) : ParentalCheckResult::FAILED;
    if (result == ParentalCheckResult::SUCCESS)
    {
      // switch to channel or if recording present, ask whether to switch or play recording...
      if (!recording)
        recording = CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(channel->GetEPGNow());

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

      bool bFullscreen;
      switch (m_settings.GetIntValue(CSettings::SETTING_PVRPLAYBACK_SWITCHTOFULLSCREENCHANNELTYPES))
      {
        case 0: // never
          bFullscreen = false;
          break;
        case 1: // TV channels
          bFullscreen = !channel->IsRadio();
          break;
        case 2: // Radio channels
          bFullscreen = channel->IsRadio();
          break;
        case 3: // TV and radio channels
        default:
          bFullscreen = true;
          break;
      }
      const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMember(*item);
      if (!groupMember)
        return false;

      StartPlayback(new CFileItem(groupMember), bFullscreen);
      return true;
    }
    else if (result == ParentalCheckResult::FAILED)
    {
      const std::string channelName = channel ? channel->ChannelName() : g_localizeStrings.Get(19029); // Channel
      const std::string msg = StringUtils::Format(
          g_localizeStrings.Get(19035),
          channelName); // CHANNELNAME could not be played. Check the log for details.

      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(19166), msg); // PVR information
    }

    return false;
  }

  bool CPVRGUIActions::SwitchToChannel(PlaybackType type) const
  {
    std::shared_ptr<CPVRChannelGroupMember> groupMember;
    bool bIsRadio(false);

    // check if the desired PlaybackType is already playing,
    // and if not, try to grab the last played channel of this type
    switch (type)
    {
      case PlaybackTypeRadio:
      {
        if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRadio())
          return true;

        const std::shared_ptr<CPVRChannelGroup> allGroup = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllRadio();
        if (allGroup)
          groupMember = allGroup->GetLastPlayedChannelGroupMember();

        bIsRadio = true;
        break;
      }
      case PlaybackTypeTV:
      {
        if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingTV())
          return true;

        const std::shared_ptr<CPVRChannelGroup> allGroup = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV();
        if (allGroup)
          groupMember = allGroup->GetLastPlayedChannelGroupMember();

        break;
      }
      default:
        if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
          return true;

        groupMember =
            CServiceBroker::GetPVRManager().ChannelGroups()->GetLastPlayedChannelGroupMember();
        break;
    }

    // if we have a last played channel, start playback
    if (groupMember)
    {
      return SwitchToChannel(std::make_shared<CFileItem>(groupMember), true);
    }
    else
    {
      // if we don't, find the active channel group of the demanded type and play it's first channel
      const std::shared_ptr<CPVRChannelGroup> channelGroup =
          CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(bIsRadio);
      if (channelGroup)
      {
        // try to start playback of first channel in this group
        const std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers =
            channelGroup->GetMembers();
        if (!groupMembers.empty())
        {
          return SwitchToChannel(std::make_shared<CFileItem>(*groupMembers.begin()), true);
        }
      }
    }

    CLog::LogF(LOGERROR,
               "Could not determine {} channel to playback. No last played channel found, and "
               "first channel of active group could also not be determined.",
               bIsRadio ? "Radio" : "TV");

    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Error,
        g_localizeStrings.Get(19166), // PVR information
        StringUtils::Format(
            g_localizeStrings.Get(19035),
            g_localizeStrings.Get(
                bIsRadio ? 19021
                         : 19020))); // Radio/TV could not be played. Check the log for details.
    return false;
  }

  bool CPVRGUIActions::PlayChannelOnStartup() const
  {
    int iAction = m_settings.GetIntValue(CSettings::SETTING_LOOKANDFEEL_STARTUPACTION);
    if (iAction != STARTUP_ACTION_PLAY_TV &&
        iAction != STARTUP_ACTION_PLAY_RADIO)
      return false;

    bool playRadio = (iAction == STARTUP_ACTION_PLAY_RADIO);

    // get the last played channel or fallback to first channel of all channels group
    std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().PlaybackState()->GetLastPlayedChannelGroupMember(playRadio);

    if (!groupMember)
    {
      const std::shared_ptr<CPVRChannelGroup> group =
          CServiceBroker::GetPVRManager().ChannelGroups()->Get(playRadio)->GetGroupAll();
      auto channels = group->GetMembers();
      if (channels.empty())
        return false;

      groupMember = channels.front();
      if (!groupMember)
        return false;
    }

    CLog::Log(LOGINFO, "PVR is starting playback of channel '{}'",
              groupMember->Channel()->ChannelName());
    return SwitchToChannel(std::make_shared<CFileItem>(groupMember), true);
  }

  bool CPVRGUIActions::PlayMedia(const CFileItemPtr& item) const
  {
    CFileItemPtr pvrItem(item);
    if (URIUtils::IsPVRChannel(item->GetPath()) && !item->HasPVRChannelInfoTag())
    {
      const std::shared_ptr<CPVRChannelGroupMember> groupMember =
          CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelGroupMemberByPath(
              item->GetPath());
      if (groupMember)
        pvrItem = std::make_shared<CFileItem>(groupMember);
    }
    else if (URIUtils::IsPVRRecording(item->GetPath()) && !item->HasPVRRecordingInfoTag())
      pvrItem = std::make_shared<CFileItem>(CServiceBroker::GetPVRManager().Recordings()->GetByPath(item->GetPath()));

    bool bCheckResume = true;
    if (item->HasProperty("check_resume"))
      bCheckResume = item->GetProperty("check_resume").asBoolean();

    if (pvrItem && pvrItem->HasPVRChannelInfoTag())
    {
      return SwitchToChannel(pvrItem, bCheckResume);
    }
    else if (pvrItem && pvrItem->HasPVRRecordingInfoTag())
    {
      return PlayRecording(pvrItem, bCheckResume);
    }

    return false;
  }

  bool CPVRGUIActions::HideChannel(const CFileItemPtr& item) const
  {
    const std::shared_ptr<CPVRChannel> channel(item->GetPVRChannelInfoTag());

    if (!channel)
      return false;

    if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{19054}, // "Hide channel"
                                          CVariant{19039}, // "Are you sure you want to hide this channel?"
                                          CVariant{""},
                                          CVariant{channel->ChannelName()}))
      return false;

    if (!CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(channel->IsRadio())->RemoveFromGroup(channel))
      return false;

    CGUIWindowPVRBase* pvrWindow = dynamic_cast<CGUIWindowPVRBase*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow()));
    if (pvrWindow)
      pvrWindow->DoRefresh();
    else
      CLog::LogF(LOGERROR, "Called on non-pvr window. No refresh possible.");

    return true;
  }

  bool CPVRGUIActions::StartChannelScan()
  {
    return StartChannelScan(PVR_INVALID_CLIENT_ID);
  }

  bool CPVRGUIActions::StartChannelScan(int clientId)
  {
    if (!CServiceBroker::GetPVRManager().IsStarted() || IsRunningChannelScan())
      return false;

    std::shared_ptr<CPVRClient> scanClient;
    std::vector<std::shared_ptr<CPVRClient>> possibleScanClients = CServiceBroker::GetPVRManager().Clients()->GetClientsSupportingChannelScan();
    m_bChannelScanRunning = true;

    if (clientId != PVR_INVALID_CLIENT_ID)
    {
      for (const auto& client : possibleScanClients)
      {
        if (client->GetID() == clientId)
        {
          scanClient = client;
          break;
        }
      }

      if (!scanClient)
      {
        CLog::LogF(LOGERROR,
                   "Provided client id '{}' could not be found in list of possible scan clients!",
                   clientId);
        m_bChannelScanRunning = false;
        return false;
      }
    }
    /* multiple clients found */
    else if (possibleScanClients.size() > 1)
    {
      CGUIDialogSelect* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
      if (!pDialog)
      {
        CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
        m_bChannelScanRunning = false;
        return false;
      }

      pDialog->Reset();
      pDialog->SetHeading(CVariant{19119}); // "On which backend do you want to search?"

      for (const auto& client : possibleScanClients)
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
      HELPERS::ShowOKDialogText(CVariant{19033},  // "Information"
                                    CVariant{19192}); // "None of the connected PVR backends supports scanning for channels."
      m_bChannelScanRunning = false;
      return false;
    }

    /* start the channel scan */
    CLog::LogFC(LOGDEBUG, LOGPVR, "Starting to scan for channels on client {}",
                scanClient->GetFriendlyName());
    auto start = std::chrono::steady_clock::now();

    /* do the scan */
    if (scanClient->StartChannelScan() != PVR_ERROR_NO_ERROR)
      HELPERS::ShowOKDialogText(CVariant{257},    // "Error"
                                    CVariant{19193}); // "The channel scan can't be started. Check the log for more information about this message."

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    CLog::LogFC(LOGDEBUG, LOGPVR, "Channel scan finished after {} ms", duration.count());

    m_bChannelScanRunning = false;
    return true;
  }

  bool CPVRGUIActions::ProcessSettingsMenuHooks()
  {
    CPVRClientMap clients;
    CServiceBroker::GetPVRManager().Clients()->GetCreatedClients(clients);

    std::vector<std::pair<std::shared_ptr<CPVRClient>, CPVRClientMenuHook>> settingsHooks;
    for (const auto& client : clients)
    {
      for (const auto& hook : client.second->GetMenuHooks()->GetSettingsHooks())
      {
        settingsHooks.emplace_back(std::make_pair(client.second, hook));
      }
    }

    if (settingsHooks.empty())
      return true; // no settings hooks, no error

    auto selectedHook = settingsHooks.begin();

    // if there is only one settings hook, execute it directly, otherwise let the user select
    if (settingsHooks.size() > 1)
    {
      CGUIDialogSelect* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
      if (!pDialog)
      {
        CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
        return false;
      }

      pDialog->Reset();
      pDialog->SetHeading(CVariant{19196}); // "PVR client specific actions"

      for (const auto& hook : settingsHooks)
      {
        if (clients.size() == 1)
          pDialog->Add(hook.second.GetLabel());
        else
          pDialog->Add(hook.first->GetBackendName() + ": " + hook.second.GetLabel());
      }

      pDialog->Open();

      int selection = pDialog->GetSelectedItem();
      if (selection < 0)
        return true; // cancelled

      std::advance(selectedHook, selection);
    }
    return selectedHook->first->CallSettingsMenuHook(selectedHook->second) == PVR_ERROR_NO_ERROR;
  }

  namespace
  {
  class CPVRGUIDatabaseResetComponentsSelector
  {
  public:
    CPVRGUIDatabaseResetComponentsSelector() = default;
    virtual ~CPVRGUIDatabaseResetComponentsSelector() = default;

    bool Select()
    {
      CGUIDialogSelect* pDlgSelect =
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
              WINDOW_DIALOG_SELECT);
      if (!pDlgSelect)
      {
        CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
        return false;
      }

      CFileItemList options;

      const std::shared_ptr<CFileItem> itemAll =
          std::make_shared<CFileItem>(StringUtils::Format(g_localizeStrings.Get(593))); // All
      itemAll->SetPath("all");
      options.Add(itemAll);

      // if channels are cleared, groups, EPG data and providers must also be cleared
      const std::shared_ptr<CFileItem> itemChannels = std::make_shared<CFileItem>(
          StringUtils::Format("{}, {}, {}, {}",
                              g_localizeStrings.Get(19019), // Channels
                              g_localizeStrings.Get(19146), // Groups
                              g_localizeStrings.Get(19069), // Guide
                              g_localizeStrings.Get(19334))); // Providers
      itemChannels->SetPath("channels");
      itemChannels->Select(true); // preselect this item in dialog
      options.Add(itemChannels);

      const std::shared_ptr<CFileItem> itemGroups =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19146)); // Groups
      itemGroups->SetPath("groups");
      options.Add(itemGroups);

      const std::shared_ptr<CFileItem> itemGuide =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19069)); // Guide
      itemGuide->SetPath("guide");
      options.Add(itemGuide);

      const std::shared_ptr<CFileItem> itemProviders =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19334)); // Providers
      itemProviders->SetPath("providers");
      options.Add(itemProviders);

      const std::shared_ptr<CFileItem> itemReminders =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19215)); // Reminders
      itemReminders->SetPath("reminders");
      options.Add(itemReminders);

      const std::shared_ptr<CFileItem> itemRecordings =
          std::make_shared<CFileItem>(g_localizeStrings.Get(19017)); // Recordings
      itemRecordings->SetPath("recordings");
      options.Add(itemRecordings);

      const std::shared_ptr<CFileItem> itemClients =
          std::make_shared<CFileItem>(g_localizeStrings.Get(24019)); // PVR clients
      itemClients->SetPath("clients");
      options.Add(itemClients);

      pDlgSelect->Reset();
      pDlgSelect->SetHeading(CVariant{g_localizeStrings.Get(19185)}); // "Clear data"
      pDlgSelect->SetItems(options);
      pDlgSelect->SetMultiSelection(true);
      pDlgSelect->Open();

      if (!pDlgSelect->IsConfirmed())
        return false;

      for (int i : pDlgSelect->GetSelectedItems())
      {
        const std::string path = options.Get(i)->GetPath();

        m_bResetChannels |= (path == "channels" || path == "all");
        m_bResetGroups |= (path == "groups" || path == "all");
        m_bResetGuide |= (path == "guide" || path == "all");
        m_bResetProviders |= (path == "providers" || path == "all");
        m_bResetReminders |= (path == "reminders" || path == "all");
        m_bResetRecordings |= (path == "recordings" || path == "all");
        m_bResetClients |= (path == "clients" || path == "all");
      }

      m_bResetGroups |= m_bResetChannels;
      m_bResetGuide |= m_bResetChannels;
      m_bResetProviders |= m_bResetChannels;

      return (m_bResetChannels || m_bResetGroups || m_bResetGuide || m_bResetProviders ||
              m_bResetReminders || m_bResetRecordings || m_bResetClients);
    }

    bool IsResetChannelsSelected() const { return m_bResetChannels; }
    bool IsResetGroupsSelected() const { return m_bResetGroups; }
    bool IsResetGuideSelected() const { return m_bResetGuide; }
    bool IsResetProvidersSelected() const { return m_bResetProviders; }
    bool IsResetRemindersSelected() const { return m_bResetReminders; }
    bool IsResetRecordingsSelected() const { return m_bResetRecordings; }
    bool IsResetClientsSelected() const { return m_bResetClients; }

  private:
    bool m_bResetChannels = false;
    bool m_bResetGroups = false;
    bool m_bResetGuide = false;
    bool m_bResetProviders = false;
    bool m_bResetReminders = false;
    bool m_bResetRecordings = false;
    bool m_bResetClients = false;
  };

  } // unnamed namespace

  bool CPVRGUIActions::ResetPVRDatabase(bool bResetEPGOnly)
  {
    CGUIDialogProgress* pDlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    if (!pDlgProgress)
    {
      CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PROGRESS!");
      return false;
    }

    bool bResetChannels = false;
    bool bResetGroups = false;
    bool bResetGuide = false;
    bool bResetProviders = false;
    bool bResetReminders = false;
    bool bResetRecordings = false;
    bool bResetClients = false;

    if (bResetEPGOnly)
    {
      if (!CGUIDialogYesNo::ShowAndGetInput(
              CVariant{19098}, // "Warning!"
              CVariant{19188})) // "All guide data will be cleared. Are you sure?"
        return false;

      bResetGuide = true;
    }
    else
    {
      if (CheckParentalPIN() != ParentalCheckResult::SUCCESS)
        return false;

      CPVRGUIDatabaseResetComponentsSelector selector;
      if (!selector.Select())
        return false;

      if (!CGUIDialogYesNo::ShowAndGetInput(
              CVariant{19098}, // "Warning!"
              CVariant{19186})) // "All selected data will be cleared. ... Are you sure?"
        return false;

      bResetChannels = selector.IsResetChannelsSelected();
      bResetGroups = selector.IsResetGroupsSelected();
      bResetGuide = selector.IsResetGuideSelected();
      bResetProviders = selector.IsResetProvidersSelected();
      bResetReminders = selector.IsResetRemindersSelected();
      bResetRecordings = selector.IsResetRecordingsSelected();
      bResetClients = selector.IsResetClientsSelected();
    }

    CDateTime::ResetTimezoneBias();

    CLog::LogFC(LOGDEBUG, LOGPVR, "PVR clearing {} database",
                bResetEPGOnly ? "EPG" : "PVR and EPG");

    pDlgProgress->SetHeading(CVariant{313}); // "Cleaning database"
    pDlgProgress->SetLine(0, CVariant{g_localizeStrings.Get(19187)}); // "Clearing all related data."
    pDlgProgress->SetLine(1, CVariant{""});
    pDlgProgress->SetLine(2, CVariant{""});

    pDlgProgress->Open();
    pDlgProgress->Progress();

    if (CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
    {
      CLog::Log(LOGINFO, "PVR is stopping playback for {} database reset",
                bResetEPGOnly ? "EPG" : "PVR and EPG");
      CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
    }

    const std::shared_ptr<CPVRDatabase> pvrDatabase(CServiceBroker::GetPVRManager().GetTVDatabase());
    const std::shared_ptr<CPVREpgDatabase> epgDatabase(CServiceBroker::GetPVRManager().EpgContainer().GetEpgDatabase());

    // increase db open refcounts, so they don't get closed during following pvr manager shutdown
    pvrDatabase->Open();
    epgDatabase->Open();

    // stop pvr manager; close both pvr and epg databases
    CServiceBroker::GetPVRManager().Stop();

    const int iProgressStepPercentage =
        100 / ((2 * bResetChannels) + bResetGroups + bResetGuide + bResetProviders +
               bResetReminders + bResetRecordings + bResetClients + 1);
    int iProgressStepsDone = 0;

    if (bResetProviders)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all providers
      pvrDatabase->DeleteProviders();
    }

    if (bResetGuide)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // reset channel's EPG pointers
      pvrDatabase->ResetEPG();

      // delete all entries from the EPG database
      epgDatabase->DeleteEpg();
    }

    if (bResetGroups)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all channel groups (including data only available locally, like user defined groups)
      pvrDatabase->DeleteChannelGroups();
    }

    if (bResetChannels)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all channels (including data only available locally, like user set icons)
      pvrDatabase->DeleteChannels();
    }

    if (bResetReminders)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all timers data (e.g. all reminders, which are only stored locally)
      pvrDatabase->DeleteTimers();
    }

    if (bResetClients)
    {
      pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
      pDlgProgress->Progress();

      // delete all clients data (e.g priorities, which are only stored locally)
      pvrDatabase->DeleteClients();
    }

    if (bResetChannels || bResetRecordings)
    {
      CVideoDatabase videoDatabase;

      if (videoDatabase.Open())
      {
        if (bResetChannels)
        {
          pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
          pDlgProgress->Progress();

          // delete all channel's entries (e.g. settings, bookmarks, stream details)
          videoDatabase.EraseAllForPath("pvr://channels/");
        }

        if (bResetRecordings)
        {
          pDlgProgress->SetPercentage(iProgressStepPercentage * ++iProgressStepsDone);
          pDlgProgress->Progress();

          // delete all recording's entries (e.g. settings, bookmarks, stream details)
          videoDatabase.EraseAllForPath(CPVRRecordingsPath::PATH_RECORDINGS);
        }

        videoDatabase.Close();
      }
    }

    // decrease db open refcounts; this actually closes dbs because refcounts drops to zero
    pvrDatabase->Close();
    epgDatabase->Close();

    CLog::LogFC(LOGDEBUG, LOGPVR, "{} database cleared", bResetEPGOnly ? "EPG" : "PVR and EPG");

    CLog::Log(LOGINFO, "Restarting the PVR Manager after {} database reset",
              bResetEPGOnly ? "EPG" : "PVR and EPG");
    CServiceBroker::GetPVRManager().Start();

    pDlgProgress->SetPercentage(100);
    pDlgProgress->Close();
    return true;
  }

  ParentalCheckResult CPVRGUIActions::CheckParentalLock(const std::shared_ptr<CPVRChannel>& channel) const
  {
    if (!CServiceBroker::GetPVRManager().IsParentalLocked(channel))
      return ParentalCheckResult::SUCCESS;

    ParentalCheckResult ret = CheckParentalPIN();

    if (ret == ParentalCheckResult::FAILED)
      CLog::LogF(LOGERROR, "Parental lock verification failed for channel '{}': wrong PIN entered.",
                 channel->ChannelName());

    return ret;
  }

  ParentalCheckResult CPVRGUIActions::CheckParentalPIN() const
  {
    if (!m_settings.GetBoolValue(CSettings::SETTING_PVRPARENTAL_ENABLED))
      return ParentalCheckResult::SUCCESS;

    std::string pinCode = m_settings.GetStringValue(CSettings::SETTING_PVRPARENTAL_PIN);
    if (pinCode.empty())
      return ParentalCheckResult::SUCCESS;

    InputVerificationResult ret = CGUIDialogNumeric::ShowAndVerifyInput(pinCode, g_localizeStrings.Get(19262), true); // "Parental control. Enter PIN:"

    if (ret == InputVerificationResult::SUCCESS)
    {
      CServiceBroker::GetPVRManager().RestartParentalTimer();
      return ParentalCheckResult::SUCCESS;
    }
    else if (ret == InputVerificationResult::FAILED)
    {
      HELPERS::ShowOKDialogText(CVariant{19264}, CVariant{19265}); // "Incorrect PIN", "The entered PIN was incorrect."
      return ParentalCheckResult::FAILED;
    }
    else
    {
      return ParentalCheckResult::CANCELED;
    }
  }

  bool CPVRGUIActions::CanSystemPowerdown(bool bAskUser /*= true*/) const
  {
    bool bReturn(true);
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      std::shared_ptr<CPVRTimerInfoTag> cause;
      if (!AllLocalBackendsIdle(cause))
      {
        if (bAskUser)
        {
          std::string text;

          if (cause)
          {
            if (cause->IsRecording())
            {
              text = StringUtils::Format(
                  g_localizeStrings.Get(19691), // "PVR is currently recording...."
                  cause->Title(), cause->ChannelName());
            }
            else
            {
              // Next event is due to a local recording or reminder.
              const CDateTime now(CDateTime::GetUTCDateTime());
              const CDateTime start(cause->StartAsUTC());
              const CDateTimeSpan prestart(0, 0, cause->MarginStart(), 0);

              CDateTimeSpan diff(start - now);
              diff -= prestart;
              int mins = diff.GetSecondsTotal() / 60;

              std::string dueStr;
              if (mins > 1)
              {
                // "%d minutes"
                dueStr = StringUtils::Format(g_localizeStrings.Get(19694), mins);
              }
              else
              {
                // "about a minute"
                dueStr = g_localizeStrings.Get(19695);
              }

              text = StringUtils::Format(
                  cause->IsReminder()
                      ? g_localizeStrings.Get(19690) // "PVR has scheduled a reminder...."
                      : g_localizeStrings.Get(19692), // "PVR will start recording...."
                  cause->Title(), cause->ChannelName(), dueStr);
            }
          }
          else
          {
            // Next event is due to automatic daily wakeup of PVR.
            const CDateTime now(CDateTime::GetUTCDateTime());

            CDateTime dailywakeuptime;
            dailywakeuptime.SetFromDBTime(m_settings.GetStringValue(CSettings::SETTING_PVRPOWERMANAGEMENT_DAILYWAKEUPTIME));
            dailywakeuptime = dailywakeuptime.GetAsUTCDateTime();

            const CDateTimeSpan diff(dailywakeuptime - now);
            int mins = diff.GetSecondsTotal() / 60;

            std::string dueStr;
            if (mins > 1)
            {
              // "%d minutes"
              dueStr = StringUtils::Format(g_localizeStrings.Get(19694), mins);
            }
            else
            {
              // "about a minute"
              dueStr = g_localizeStrings.Get(19695);
            }

            text = StringUtils::Format(g_localizeStrings.Get(19693), // "Daily wakeup is due in...."
                                       dueStr);
          }

          // Inform user about PVR being busy. Ask if user wants to powerdown anyway.
          bReturn = HELPERS::ShowYesNoDialogText(CVariant{19685}, // "Confirm shutdown"
                                                 CVariant{text},
                                                 CVariant{222}, // "Shutdown anyway",
                                                 CVariant{19696}, // "Cancel"
                                                 10000) // timeout value before closing
                    == HELPERS::DialogResponse::YES;
        }
        else
          bReturn = false; // do not powerdown (busy, but no user interaction requested).
      }
    }
    return bReturn;
  }

  bool CPVRGUIActions::AllLocalBackendsIdle(std::shared_ptr<CPVRTimerInfoTag>& causingEvent) const
  {
    // active recording on local backend?
    const std::vector<std::shared_ptr<CPVRTimerInfoTag>> activeRecordings = CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();
    for (const auto& timer : activeRecordings)
    {
      if (EventOccursOnLocalBackend(std::make_shared<CFileItem>(timer)))
      {
        causingEvent = timer;
        return false;
      }
    }

    // soon recording on local backend?
    if (IsNextEventWithinBackendIdleTime())
    {
      const std::shared_ptr<CPVRTimerInfoTag> timer = CServiceBroker::GetPVRManager().Timers()->GetNextActiveTimer(false);
      if (!timer)
      {
        // Next event is due to automatic daily wakeup of PVR!
        causingEvent.reset();
        return false;
      }

      if (EventOccursOnLocalBackend(std::make_shared<CFileItem>(timer)))
      {
        causingEvent = timer;
        return false;
      }
    }
    return true;
  }

  bool CPVRGUIActions::EventOccursOnLocalBackend(const CFileItemPtr& item) const
  {
    if (item && item->HasPVRTimerInfoTag())
    {
      const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
      if (client)
      {
        const std::string hostname = client->GetBackendHostname();
        if (!hostname.empty() && CServiceBroker::GetNetwork().IsLocalHost(hostname))
          return true;
      }
    }
    return false;
  }

  namespace
  {
    std::string GetAnnouncerText(const std::shared_ptr<CPVRTimerInfoTag>& timer, int idEpg, int idNoEpg)
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
        text = StringUtils::Format(g_localizeStrings.Get(idNoEpg),
                                   timer->ChannelName(),
                                   timer->StartAsLocalTime().GetAsLocalizedDateTime(false, false));
      }
      return text;
    }

    void AddEventLogEntry(const std::shared_ptr<CPVRTimerInfoTag>& timer, int idEpg, int idNoEpg)
    {
      std::string name;
      std::string icon;

      const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(timer->GetTimerType()->GetClientId());
      if (client)
      {
        name = client->Name();
        icon = client->Icon();
      }
      else
      {
        name = g_sysinfo.GetAppName();
        icon = "special://xbmc/media/icon256x256.png";
      }

      CPVREventLogJob* job = new CPVREventLogJob;
      job->AddEvent(false, // do not display a toast, only log event
                    false, // info, no error
                    name,
                    GetAnnouncerText(timer, idEpg, idNoEpg),
                    icon);
      CJobManager::GetInstance().AddJob(job, nullptr);
    }
  } // unnamed namespace

  bool CPVRGUIActions::IsNextEventWithinBackendIdleTime() const
  {
    // timers going off soon?
    const CDateTime now(CDateTime::GetUTCDateTime());
    const CDateTimeSpan idle(0, 0, m_settings.GetIntValue(CSettings::SETTING_PVRPOWERMANAGEMENT_BACKENDIDLETIME), 0);
    const CDateTime next(CServiceBroker::GetPVRManager().Timers()->GetNextEventTime());
    const CDateTimeSpan delta(next - now);

    return (delta <= idle);
  }

  void CPVRGUIActions::AnnounceReminder(const std::shared_ptr<CPVRTimerInfoTag>& timer) const
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
    CGUIDialogProgress* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    if (!dialog)
      return;

    dialog->Reset();

    dialog->SetHeading(CVariant{19312}); // "PVR reminder"
    dialog->ShowChoice(0, CVariant{19165}); // "Switch"

    std::string text = GetAnnouncerText(timer, 19307, 19308); // Reminder for ...

    bool bCanRecord = false;
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(timer->m_iClientId);
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
        newTimer =
            CPVRTimerInfoTag::CreateTimerTag(timer->Channel(), timer->StartAsUTC(), iDuration);
      }

      if (newTimer)
      {
        // schedule recording
        AddTimer(std::make_shared<CFileItem>(newTimer), false);
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
          GetChannelGroupMember(timer->Channel());
      if (groupMember)
      {
        SwitchToChannel(std::make_shared<CFileItem>(groupMember), false);

        if (bAutoClosed)
        {
          AddEventLogEntry(timer, 19332,
                           19333); // Switched channel for auto-closed PVR reminder ...
        }
      }
    }
  }

  void CPVRGUIActions::AnnounceReminders() const
  {
    // Prevent multiple yesno dialogs, all on same call stack, due to gui message processing while dialog is open.
    if (m_bReminderAnnouncementRunning)
      return;

    m_bReminderAnnouncementRunning = true;
    std::shared_ptr<CPVRTimerInfoTag> timer = CServiceBroker::GetPVRManager().Timers()->GetNextReminderToAnnnounce();
    while (timer)
    {
      AnnounceReminder(timer);
      timer = CServiceBroker::GetPVRManager().Timers()->GetNextReminderToAnnnounce();
    }
    m_bReminderAnnouncementRunning = false;
  }

  void CPVRGUIActions::SetSelectedItemPath(bool bRadio, const std::string& path)
  {
    CSingleLock lock(m_critSection);
    if (bRadio)
      m_selectedItemPathRadio = path;
    else
      m_selectedItemPathTV = path;
  }

  std::string CPVRGUIActions::GetSelectedItemPath(bool bRadio) const
  {
    if (m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL))
    {
      CPVRManager& mgr = CServiceBroker::GetPVRManager();

      // if preselect playing channel is activated, return the path of the playing channel, if any.
      const std::shared_ptr<CPVRChannel> playingChannel = mgr.PlaybackState()->GetPlayingChannel();
      if (playingChannel && playingChannel->IsRadio() == bRadio)
        return GetChannelGroupMember(playingChannel)->Path();

      const std::shared_ptr<CPVREpgInfoTag> playingTag = mgr.PlaybackState()->GetPlayingEpgTag();
      if (playingTag && playingTag->IsRadio() == bRadio)
      {
        const std::shared_ptr<CPVRChannel> channel =
            mgr.ChannelGroups()->GetChannelForEpgTag(playingTag);
        if (channel)
          return GetChannelGroupMember(channel)->Path();
      }
    }

    CSingleLock lock(m_critSection);
    return bRadio ? m_selectedItemPathRadio : m_selectedItemPathTV;
  }

  void CPVRGUIActions::SeekForward()
  {
    time_t playbackStartTime = CServiceBroker::GetDataCacheCore().GetStartTime();
    if (playbackStartTime > 0)
    {
      const std::shared_ptr<CPVRChannel> playingChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
      if (playingChannel)
      {
        time_t nextTime = 0;
        std::shared_ptr<CPVREpgInfoTag> next = playingChannel->GetEPGNext();
        if (next)
        {
          next->StartAsUTC().GetAsTime(nextTime);
        }
        else
        {
          // if there is no next event, jump to end of currently playing event
          next = playingChannel->GetEPGNow();
          if (next)
            next->EndAsUTC().GetAsTime(nextTime);
        }

        int64_t seekTime = 0;
        if (nextTime != 0)
        {
          seekTime = (nextTime - playbackStartTime) * 1000;
        }
        else
        {
          // no epg; jump to end of buffer
          seekTime = CServiceBroker::GetDataCacheCore().GetMaxTime();
        }
        CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_SEEK_TIME, seekTime);
      }
    }
  }

  void CPVRGUIActions::SeekBackward(unsigned int iThreshold)
  {
    time_t playbackStartTime = CServiceBroker::GetDataCacheCore().GetStartTime();
    if (playbackStartTime > 0)
    {
      const std::shared_ptr<CPVRChannel> playingChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
      if (playingChannel)
      {
        time_t prevTime = 0;
        std::shared_ptr<CPVREpgInfoTag> prev = playingChannel->GetEPGNow();
        if (prev)
        {
          prev->StartAsUTC().GetAsTime(prevTime);

          // if playback time of current event is above threshold jump to start of current event
          int64_t playTime = CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000;
          if ((playbackStartTime + playTime - prevTime) <= iThreshold)
          {
            // jump to start of previous event
            prevTime = 0;
            prev = playingChannel->GetEPGPrevious();
            if (prev)
              prev->StartAsUTC().GetAsTime(prevTime);
          }
        }

        int64_t seekTime = 0;
        if (prevTime != 0)
        {
          seekTime = (prevTime - playbackStartTime) * 1000;
        }
        else
        {
          // no epg; jump to begin of buffer
          seekTime = CServiceBroker::GetDataCacheCore().GetMinTime();
        }
        CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_SEEK_TIME, seekTime);
      }
    }
  }

  std::shared_ptr<CPVRChannelGroupMember> CPVRGUIActions::GetChannelGroupMember(
      const std::shared_ptr<CPVRChannel>& channel) const
  {
    std::shared_ptr<CPVRChannelGroupMember> groupMember;
    if (channel)
    {
      // first, try whether the channel is contained in the active channel group
      std::shared_ptr<CPVRChannelGroup> group =
          CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(
              channel->IsRadio());
      if (group)
        groupMember = group->GetByUniqueID(channel->StorageId());

      // as fallback, obtain the member from the 'all channels' group
      if (!groupMember)
      {
        group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(channel->IsRadio());
        if (group)
          groupMember = group->GetByUniqueID(channel->StorageId());
      }
    }
    return groupMember;
  }

  std::shared_ptr<CPVRChannelGroupMember> CPVRGUIActions::GetChannelGroupMember(
      const CFileItem& item) const
  {
    std::shared_ptr<CPVRChannelGroupMember> groupMember = item.GetPVRChannelGroupMemberInfoTag();

    if (!groupMember)
      groupMember = GetChannelGroupMember(CPVRItem(std::make_shared<CFileItem>(item)).GetChannel());

    return groupMember;
  }

  CPVRChannelNumberInputHandler& CPVRGUIActions::GetChannelNumberInputHandler()
  {
    // window/dialog specific input handler
    CPVRChannelNumberInputHandler* windowInputHandler = dynamic_cast<CPVRChannelNumberInputHandler*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog()));
    if (windowInputHandler)
      return *windowInputHandler;

    // default
    return m_channelNumberInputHandler;
  }

  CPVRGUIChannelNavigator& CPVRGUIActions::GetChannelNavigator()
  {
    return m_channelNavigator;
  }

  void CPVRGUIActions::OnPlaybackStarted(const CFileItemPtr& item)
  {
    const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetChannelGroupMember(*item);
    if (groupMember)
    {
      m_channelNavigator.SetPlayingChannel(groupMember);
      SetSelectedItemPath(groupMember->Channel()->IsRadio(), groupMember->Path());
    }
  }

  void CPVRGUIActions::OnPlaybackStopped(const CFileItemPtr& item)
  {
    if (item->HasPVRChannelInfoTag() || item->HasEPGInfoTag())
    {
      m_channelNavigator.ClearPlayingChannel();
    }
  }

  bool CPVRGUIActions::OnInfo(const std::shared_ptr<CFileItem>& item)
  {
    if (item->HasPVRRecordingInfoTag())
    {
      return ShowRecordingInfo(item);
    }
    else if (item->HasPVRChannelInfoTag() || item->HasPVRTimerInfoTag())
    {
      return ShowEPGInfo(item);
    }
    else if (item->HasEPGSearchFilter())
    {
      return EditSavedSearch(item);
    }
    return false;
  }

  bool CPVRGUIActions::ExecuteSavedSearch(const std::shared_ptr<CFileItem>& item)
  {
    const auto searchFilter = item->GetEPGSearchFilter();

    if (!searchFilter)
    {
      CLog::LogF(LOGERROR, "Wrong item type. No EPG search filter present.");
      return false;
    }

    CGUIWindowPVRSearchBase* windowSearch = GetSearchWindow(searchFilter->IsRadio());
    if (!windowSearch)
      return false;

    windowSearch->SetItemToSearch(item);
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(windowSearch->GetID());
    return true;
  }

  bool CPVRGUIActions::EditSavedSearch(const std::shared_ptr<CFileItem>& item)
  {
    const auto searchFilter = item->GetEPGSearchFilter();

    if (!searchFilter)
    {
      CLog::LogF(LOGERROR, "Wrong item type. No EPG search filter present.");
      return false;
    }

    CGUIWindowPVRSearchBase* windowSearch = GetSearchWindow(searchFilter->IsRadio());
    if (!windowSearch)
      return false;

    if (windowSearch->OpenDialogSearch(item) == CGUIDialogPVRGuideSearch::Result::SEARCH)
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(windowSearch->GetID());

    return true;
  }

  bool CPVRGUIActions::RenameSavedSearch(const std::shared_ptr<CFileItem>& item)
  {
    const auto searchFilter = item->GetEPGSearchFilter();

    if (!searchFilter)
    {
      CLog::LogF(LOGERROR, "Wrong item type. No EPG search filter present.");
      return false;
    }

    std::string title = searchFilter->GetTitle();
    if (CGUIKeyboardFactory::ShowAndGetInput(title,
                                             CVariant{g_localizeStrings.Get(528)}, // "Enter title"
                                             false))
    {
      searchFilter->SetTitle(title);
      CServiceBroker::GetPVRManager().EpgContainer().PersistSavedSearch(*searchFilter);
      return true;
    }
    return false;
  }

  bool CPVRGUIActions::DeleteSavedSearch(const std::shared_ptr<CFileItem>& item)
  {
    const auto searchFilter = item->GetEPGSearchFilter();

    if (!searchFilter)
    {
      CLog::LogF(LOGERROR, "Wrong item type. No EPG search filter present.");
      return false;
    }

    if (CGUIDialogYesNo::ShowAndGetInput(CVariant{122}, // "Confirm delete"
                                         CVariant{19338}, // "Delete this saved search?"
                                         CVariant{""}, CVariant{item->GetLabel()}))
    {
      return CServiceBroker::GetPVRManager().EpgContainer().DeleteSavedSearch(*searchFilter);
    }
    return false;
  }

  void CPVRChannelSwitchingInputHandler::AppendChannelNumberCharacter(char cCharacter)
  {
    // special case. if only a single zero was typed in, switch to previously played channel.
    if (GetCurrentDigitCount() == 0 && cCharacter == '0')
    {
      SwitchToPreviousChannel();
      return;
    }

    CPVRChannelNumberInputHandler::AppendChannelNumberCharacter(cCharacter);
  }

  void CPVRChannelSwitchingInputHandler::GetChannelNumbers(std::vector<std::string>& channelNumbers)
  {
    CPVRManager& pvrMgr = CServiceBroker::GetPVRManager();
    const std::shared_ptr<CPVRChannel> playingChannel = pvrMgr.PlaybackState()->GetPlayingChannel();
    if (playingChannel)
    {
      const std::shared_ptr<CPVRChannelGroup> group = pvrMgr.ChannelGroups()->GetGroupAll(playingChannel->IsRadio());
      if (group)
        group->GetChannelNumbers(channelNumbers);
    }
  }

  void CPVRChannelSwitchingInputHandler::OnInputDone()
  {
    CPVRChannelNumber channelNumber = GetChannelNumber();
    if (channelNumber.GetChannelNumber())
      SwitchToChannel(channelNumber);
  }

  void CPVRChannelSwitchingInputHandler::SwitchToChannel(const CPVRChannelNumber& channelNumber)
  {
    if (channelNumber.IsValid() && CServiceBroker::GetPVRManager().PlaybackState()->IsPlaying())
    {
      const std::shared_ptr<CPVRChannel> playingChannel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();
      if (playingChannel)
      {
        bool bRadio = playingChannel->IsRadio();
        const std::shared_ptr<CPVRChannelGroup> group =
            CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(bRadio);

        if (channelNumber != group->GetChannelNumber(playingChannel))
        {
          // channel number present in active group?
          std::shared_ptr<CPVRChannelGroupMember> groupMember =
              group->GetByChannelNumber(channelNumber);

          if (!groupMember)
          {
            // channel number present in any group?
            const CPVRChannelGroups* groupAccess = CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio);
            const std::vector<std::shared_ptr<CPVRChannelGroup>> groups = groupAccess->GetMembers(true);
            for (const auto& currentGroup : groups)
            {
              if (currentGroup == group) // we have already checked this group
                continue;

              groupMember = currentGroup->GetByChannelNumber(channelNumber);
              if (groupMember)
                break;
            }
          }

          if (groupMember)
          {
            CApplicationMessenger::GetInstance().PostMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1,
              static_cast<void*>(new CAction(ACTION_CHANNEL_SWITCH,
                                             static_cast<float>(channelNumber.GetChannelNumber()),
                                             static_cast<float>(channelNumber.GetSubChannelNumber()))));
          }
        }
      }
    }
  }

  void CPVRChannelSwitchingInputHandler::SwitchToPreviousChannel()
  {
    const std::shared_ptr<CPVRPlaybackState> playbackState =
        CServiceBroker::GetPVRManager().PlaybackState();
    if (playbackState->IsPlaying())
    {
      const std::shared_ptr<CPVRChannel> playingChannel = playbackState->GetPlayingChannel();
      if (playingChannel)
      {
        const std::shared_ptr<CPVRChannelGroupMember> groupMember =
            playbackState->GetPreviousToLastPlayedChannelGroupMember(playingChannel->IsRadio());
        if (groupMember)
        {
          const CPVRChannelNumber channelNumber = groupMember->ChannelNumber();
          CApplicationMessenger::GetInstance().SendMsg(
              TMSG_GUI_ACTION, WINDOW_INVALID, -1,
              static_cast<void*>(new CAction(
                  ACTION_CHANNEL_SWITCH, static_cast<float>(channelNumber.GetChannelNumber()),
                  static_cast<float>(channelNumber.GetSubChannelNumber()))));
        }
      }
    }
  }

} // namespace PVR
