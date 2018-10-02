/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIChannelNavigator.h"

#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRJobs.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroup.h"

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

    const CPVRChannelPtr nextChannel = GetNextOrPrevChannel(true);
    if (nextChannel)
      SelectChannel(nextChannel, eSwitchMode);
  }

  void CPVRGUIChannelNavigator::SelectPreviousChannel(ChannelSwitchMode eSwitchMode)
  {
    if (!CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().GetShowInfo() && eSwitchMode == ChannelSwitchMode::NO_SWITCH)
    {
      // show info for current channel on first previous channel selection.
      ShowInfo(false);
      return;
    }

    const CPVRChannelPtr prevChannel = GetNextOrPrevChannel(false);
    if (prevChannel)
      SelectChannel(prevChannel, eSwitchMode);
  }

  CPVRChannelPtr CPVRGUIChannelNavigator::GetNextOrPrevChannel(bool bNext)
  {
    const bool bPlayingRadio = CServiceBroker::GetPVRManager().IsPlayingRadio();
    const bool bPlayingTV = CServiceBroker::GetPVRManager().IsPlayingTV();

    if (bPlayingTV || bPlayingRadio)
    {
      const CPVRChannelGroupPtr group = CServiceBroker::GetPVRManager().GetPlayingGroup(bPlayingRadio);
      if (group)
      {
        CSingleLock lock(m_critSection);
        const CFileItemPtr item = bNext
          ? group->GetNextChannel(m_currentChannel)
          : group->GetPreviousChannel(m_currentChannel);;
        if (item)
          return item->GetPVRChannelInfoTag();
      }
    }
    return CPVRChannelPtr();
  }

  void CPVRGUIChannelNavigator::SelectChannel(const CPVRChannelPtr channel, ChannelSwitchMode eSwitchMode)
  {
    CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(CFileItem(channel));

    CSingleLock lock(m_critSection);
    m_currentChannel = channel;
    ShowInfo(false);

    if (IsPreview() && eSwitchMode == ChannelSwitchMode::INSTANT_OR_DELAYED_SWITCH)
    {
      int iTimeout = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_PVRPLAYBACK_CHANNELENTRYTIMEOUT);
      if (iTimeout > 0)
      {
        // delayed switch
        if (m_iChannelEntryJobId >= 0)
          CJobManager::GetInstance().CancelJob(m_iChannelEntryJobId);

        CPVRChannelEntryTimeoutJob *job = new CPVRChannelEntryTimeoutJob(iTimeout);
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
        CPVRChannelInfoTimeoutJob *job = new CPVRChannelInfoTimeoutJob(iTimeout * 1000);
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

  void CPVRGUIChannelNavigator::SetPlayingChannel(const CPVRChannelPtr channel)
  {
    CFileItemPtr item;

    if (channel)
    {
      CSingleLock lock(m_critSection);

      m_playingChannel = channel;
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
