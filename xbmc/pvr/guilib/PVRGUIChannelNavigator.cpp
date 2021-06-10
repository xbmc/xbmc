/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIChannelNavigator.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/guilib/PVRGUIActions.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/Job.h"
#include "utils/JobManager.h"
#include "utils/XTimeUtils.h"

namespace
{
class CPVRChannelTimeoutJobBase : public CJob, public IJobCallback
{
public:
  CPVRChannelTimeoutJobBase() = delete;
  CPVRChannelTimeoutJobBase(PVR::CPVRGUIChannelNavigator& channelNavigator, int iTimeout)
  : m_channelNavigator(channelNavigator)
  {
    m_delayTimer.Set(iTimeout);
  }

  ~CPVRChannelTimeoutJobBase() override = default;

  virtual void OnTimeout() = 0;

  void OnJobComplete(unsigned int iJobID, bool bSuccess, CJob* job) override {}

  bool DoWork() override
  {
    while (!ShouldCancel(0, 0))
    {
      if (m_delayTimer.IsTimePast())
      {
        OnTimeout();
        return true;
      }
      KODI::TIME::Sleep(10);
    }
    return false;
  }

protected:
  PVR::CPVRGUIChannelNavigator& m_channelNavigator;

private:
  XbmcThreads::EndTime m_delayTimer;
};

class CPVRChannelEntryTimeoutJob : public CPVRChannelTimeoutJobBase
{
public:
  CPVRChannelEntryTimeoutJob(PVR::CPVRGUIChannelNavigator& channelNavigator, int iTimeout)
  : CPVRChannelTimeoutJobBase(channelNavigator, iTimeout) {}
  ~CPVRChannelEntryTimeoutJob() override = default;
  const char* GetType() const override { return "pvr-channel-entry-timeout-job"; }
  void OnTimeout() override { m_channelNavigator.SwitchToCurrentChannel(); }
};

class CPVRChannelInfoTimeoutJob : public CPVRChannelTimeoutJobBase
{
public:
  CPVRChannelInfoTimeoutJob(PVR::CPVRGUIChannelNavigator& channelNavigator, int iTimeout)
  : CPVRChannelTimeoutJobBase(channelNavigator, iTimeout) {}
  ~CPVRChannelInfoTimeoutJob() override = default;
  const char* GetType() const override { return "pvr-channel-info-timeout-job"; }
  void OnTimeout() override { m_channelNavigator.HideInfo(); }
};
} // unnamed namespace

namespace PVR
{
  void CPVRGUIChannelNavigator::SelectNextChannel(ChannelSwitchMode eSwitchMode)
  {
    if (!CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().GetShowInfo() && eSwitchMode == ChannelSwitchMode::NO_SWITCH)
    {
      // show info for current channel on first next channel selection.
      ShowInfo(false);
      return;
    }

    const std::shared_ptr<CPVRChannelGroupMember> nextMember = GetNextOrPrevChannel(true);
    if (nextMember)
      SelectChannel(nextMember, eSwitchMode);
  }

  void CPVRGUIChannelNavigator::SelectPreviousChannel(ChannelSwitchMode eSwitchMode)
  {
    if (!CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().GetShowInfo() && eSwitchMode == ChannelSwitchMode::NO_SWITCH)
    {
      // show info for current channel on first previous channel selection.
      ShowInfo(false);
      return;
    }

    const std::shared_ptr<CPVRChannelGroupMember> prevMember = GetNextOrPrevChannel(false);
    if (prevMember)
      SelectChannel(prevMember, eSwitchMode);
  }

  std::shared_ptr<CPVRChannelGroupMember> CPVRGUIChannelNavigator::GetNextOrPrevChannel(bool bNext)
  {
    const bool bPlayingRadio = CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingRadio();
    const bool bPlayingTV = CServiceBroker::GetPVRManager().PlaybackState()->IsPlayingTV();

    if (bPlayingTV || bPlayingRadio)
    {
      const std::shared_ptr<CPVRChannelGroup> group =
          CServiceBroker::GetPVRManager().PlaybackState()->GetActiveChannelGroup(bPlayingRadio);
      if (group)
      {
        CSingleLock lock(m_critSection);
        return bNext ? group->GetNextChannelGroupMember(m_currentChannel)
                     : group->GetPreviousChannelGroupMember(m_currentChannel);
      }
    }
    return {};
  }

  void CPVRGUIChannelNavigator::SelectChannel(
      const std::shared_ptr<CPVRChannelGroupMember>& groupMember, ChannelSwitchMode eSwitchMode)
  {
    CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(CFileItem(groupMember));

    CSingleLock lock(m_critSection);
    m_currentChannel = groupMember;
    ShowInfo(false);

    if (IsPreview() && eSwitchMode == ChannelSwitchMode::INSTANT_OR_DELAYED_SWITCH)
    {
      int iTimeout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT);
      if (iTimeout > 0)
      {
        // delayed switch
        if (m_iChannelEntryJobId >= 0)
          CJobManager::GetInstance().CancelJob(m_iChannelEntryJobId);

        CPVRChannelEntryTimeoutJob* job = new CPVRChannelEntryTimeoutJob(*this, iTimeout);
        m_iChannelEntryJobId = CJobManager::GetInstance().AddJob(job, dynamic_cast<IJobCallback*>(job));
      }
      else
      {
        // instant switch
        SwitchToCurrentChannel();
      }
    }
  }

  void CPVRGUIChannelNavigator::SwitchToCurrentChannel()
  {
    CFileItemPtr item;

    {
      CSingleLock lock(m_critSection);

      if (m_iChannelEntryJobId >= 0)
      {
        CJobManager::GetInstance().CancelJob(m_iChannelEntryJobId);
        m_iChannelEntryJobId = -1;
      }

      item.reset(new CFileItem(m_currentChannel));
    }

    if (item)
      CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(item, false);
  }

  bool CPVRGUIChannelNavigator::IsPreview() const
  {
    CSingleLock lock(m_critSection);
    return m_currentChannel != m_playingChannel;
  }

  bool CPVRGUIChannelNavigator::IsPreviewAndShowInfo() const
  {
    return IsPreview() && CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().GetShowInfo();
  }

  void CPVRGUIChannelNavigator::ShowInfo()
  {
    ShowInfo(true);
  }

  void CPVRGUIChannelNavigator::ShowInfo(bool bForce)
  {
    int iTimeout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_PVRMENU_DISPLAYCHANNELINFO);

    if (bForce || iTimeout > 0)
    {
      CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(true);

      CSingleLock lock(m_critSection);

      if (m_iChannelInfoJobId >= 0)
      {
        CJobManager::GetInstance().CancelJob(m_iChannelInfoJobId);
        m_iChannelInfoJobId = -1;
      }

      if (!bForce && iTimeout > 0)
      {
        CPVRChannelInfoTimeoutJob* job = new CPVRChannelInfoTimeoutJob(*this, iTimeout * 1000);
        m_iChannelInfoJobId = CJobManager::GetInstance().AddJob(job, dynamic_cast<IJobCallback*>(job));
      }
    }
  }

  void CPVRGUIChannelNavigator::HideInfo()
  {
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(false);

    CFileItemPtr item;

    {
      CSingleLock lock(m_critSection);

      if (m_iChannelInfoJobId >= 0)
      {
        CJobManager::GetInstance().CancelJob(m_iChannelInfoJobId);
        m_iChannelInfoJobId = -1;
      }

      if (m_currentChannel != m_playingChannel)
      {
        m_currentChannel = m_playingChannel;
        if (m_playingChannel)
          item.reset(new CFileItem(m_playingChannel));
      }
    }

    if (item)
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*item);
  }

  void CPVRGUIChannelNavigator::ToggleInfo()
  {
    if (CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().GetShowInfo())
      HideInfo();
    else
      ShowInfo();
  }

  void CPVRGUIChannelNavigator::SetPlayingChannel(
      const std::shared_ptr<CPVRChannelGroupMember>& groupMember)
  {
    CFileItemPtr item;

    if (groupMember)
    {
      CSingleLock lock(m_critSection);

      m_playingChannel = groupMember;
      if (m_currentChannel != m_playingChannel)
      {
        m_currentChannel = m_playingChannel;
        if (m_playingChannel)
          item.reset(new CFileItem(m_playingChannel));
      }
    }

    if (item)
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*item);

    ShowInfo(false);
  }

  void CPVRGUIChannelNavigator::ClearPlayingChannel()
  {
    CSingleLock lock(m_critSection);
    m_playingChannel.reset();
    HideInfo();
  }

} // namespace PVR
