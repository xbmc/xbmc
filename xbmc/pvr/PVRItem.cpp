/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRItem.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"

namespace PVR
{
  CPVREpgInfoTagPtr CPVRItem::GetEpgInfoTag() const
  {
    if (m_item->IsEPG())
    {
      return m_item->GetEPGInfoTag();
    }
    else if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag()->GetEPGNow();
    }
    else if (m_item->IsPVRTimer())
    {
      return m_item->GetPVRTimerInfoTag()->GetEpgInfoTag();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return CPVREpgInfoTagPtr();
  }

  CPVREpgInfoTagPtr CPVRItem::GetNextEpgInfoTag() const
  {
    if (m_item->IsEPG())
    {
      const std::shared_ptr<CPVRChannel> channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(m_item->GetEPGInfoTag());
      if (channel)
        return channel->GetEPGNext();
    }
    else if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag()->GetEPGNext();
    }
    else if (m_item->IsPVRTimer())
    {
      const CPVRChannelPtr channel =m_item->GetPVRTimerInfoTag()->Channel();
      if (channel)
        return channel->GetEPGNext();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return CPVREpgInfoTagPtr();
  }

  CPVRChannelPtr CPVRItem::GetChannel() const
  {
    if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag();
    }
    else if (m_item->IsEPG())
    {
      return CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(m_item->GetEPGInfoTag());
    }
    else if (m_item->IsPVRTimer())
    {
      return m_item->GetPVRTimerInfoTag()->Channel();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return CPVRChannelPtr();
  }

  CPVRTimerInfoTagPtr CPVRItem::GetTimerInfoTag() const
  {
    if (m_item->IsPVRTimer())
    {
      return m_item->GetPVRTimerInfoTag();
    }
    else if (m_item->IsEPG())
    {
      return CServiceBroker::GetPVRManager().Timers()->GetTimerForEpgTag(m_item->GetEPGInfoTag());
    }
    else if (m_item->IsPVRChannel())
    {
      return CServiceBroker::GetPVRManager().Timers()->GetActiveTimerForChannel(m_item->GetPVRChannelInfoTag());
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return CPVRTimerInfoTagPtr();
  }

  CPVRRecordingPtr CPVRItem::GetRecording() const
  {
    if (m_item->IsPVRRecording())
    {
      return m_item->GetPVRRecordingInfoTag();
    }
    else if (m_item->IsEPG())
    {
      return CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(m_item->GetEPGInfoTag());
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return CPVRRecordingPtr();
  }

  bool CPVRItem::IsRadio() const
  {
    if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag()->IsRadio();
    }
    else if (m_item->IsEPG())
    {
      return m_item->GetEPGInfoTag()->IsRadio();
    }
    else if (m_item->IsPVRRecording())
    {
      return m_item->GetPVRRecordingInfoTag()->IsRadio();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return false;
  }

} // namespace PVR
