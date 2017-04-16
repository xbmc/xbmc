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

#include "FileItem.h"
#include "epg/EpgInfoTag.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/PVRManager.h"
#include "ServiceBroker.h"
#include "utils/log.h"

#include "PVRItem.h"

namespace PVR
{
  CEpgInfoTagPtr CPVRItem::GetEpgInfoTag() const
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
      CLog::Log(LOGERROR, "CPVRItem - %s - unsupported item type!", __FUNCTION__);
    }
    return CEpgInfoTagPtr();
  }

  CPVRChannelPtr CPVRItem::GetChannel() const
  {
    if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag();
    }
    else if (m_item->IsEPG())
    {
      return m_item->GetEPGInfoTag()->ChannelTag();
    }
    else if (m_item->IsPVRTimer())
    {
      const CEpgInfoTagPtr epgTag(m_item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (epgTag)
        return epgTag->ChannelTag();
    }
    else
    {
      CLog::Log(LOGERROR, "CPVRItem - %s - unsupported item type!", __FUNCTION__);
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
      return m_item->GetEPGInfoTag()->Timer();
    }
    else if (m_item->IsPVRChannel())
    {
      CPVRTimerInfoTagPtr timer;
      const CEpgInfoTagPtr epgTag(m_item->GetPVRChannelInfoTag()->GetEPGNow());
      if (epgTag)
        timer = epgTag->Timer(); // cheap method, but not reliable as timers get set at epg tags asynchronously

      if (timer)
        return timer;

      return CServiceBroker::GetPVRManager().Timers()->GetActiveTimerForChannel(m_item->GetPVRChannelInfoTag()); // more expensive, but reliable and works even for channels with no epg data
    }
    else
    {
      CLog::Log(LOGERROR, "CPVRItem - %s - unsupported item type!", __FUNCTION__);
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
      return m_item->GetEPGInfoTag()->Recording();
    }
    else
    {
      CLog::Log(LOGERROR, "CPVRItem - %s - unsupported item type!", __FUNCTION__);
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
      const CPVRChannelPtr channel(m_item->GetEPGInfoTag()->ChannelTag());
      return (channel && channel->IsRadio());
    }
    else if (m_item->IsPVRRecording())
    {
      return m_item->GetPVRRecordingInfoTag()->IsRadio();
    }
    else
    {
      CLog::Log(LOGERROR, "CPVRItem - %s - unsupported item type!", __FUNCTION__);
    }
    return false;
  }

} // namespace PVR
