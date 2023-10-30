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
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <memory>

namespace PVR
{
std::shared_ptr<CPVREpgInfoTag> CPVRItem::GetEpgInfoTag() const
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
  else if (URIUtils::IsPVR(m_item->GetDynPath()))
  {
    CLog::LogF(LOGERROR, "Unsupported item type!");
  }
  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVRItem::GetNextEpgInfoTag() const
{
  if (m_item->IsEPG())
  {
    const std::shared_ptr<const CPVRChannel> channel =
        CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(
            m_item->GetEPGInfoTag());
    if (channel)
      return channel->GetEPGNext();
  }
  else if (m_item->IsPVRChannel())
  {
    return m_item->GetPVRChannelInfoTag()->GetEPGNext();
  }
  else if (m_item->IsPVRTimer())
  {
    const std::shared_ptr<const CPVRChannel> channel = m_item->GetPVRTimerInfoTag()->Channel();
    if (channel)
      return channel->GetEPGNext();
  }
  else if (URIUtils::IsPVR(m_item->GetDynPath()))
  {
    CLog::LogF(LOGERROR, "Unsupported item type!");
  }
  return {};
}

std::shared_ptr<CPVRChannel> CPVRItem::GetChannel() const
{
  if (m_item->IsPVRChannel())
  {
    return m_item->GetPVRChannelInfoTag();
  }
  else if (m_item->IsEPG())
  {
    return CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(
        m_item->GetEPGInfoTag());
  }
  else if (m_item->IsPVRTimer())
  {
    return m_item->GetPVRTimerInfoTag()->Channel();
  }
  else if (m_item->IsPVRRecording())
  {
    return m_item->GetPVRRecordingInfoTag()->Channel();
  }
  else if (URIUtils::IsPVR(m_item->GetDynPath()))
  {
    CLog::LogF(LOGERROR, "Unsupported item type!");
  }
  return {};
}

std::shared_ptr<CPVRTimerInfoTag> CPVRItem::GetTimerInfoTag() const
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
    return CServiceBroker::GetPVRManager().Timers()->GetActiveTimerForChannel(
        m_item->GetPVRChannelInfoTag());
  }
  else if (URIUtils::IsPVR(m_item->GetDynPath()))
  {
    CLog::LogF(LOGERROR, "Unsupported item type!");
  }
  return {};
}

std::shared_ptr<CPVRRecording> CPVRItem::GetRecording() const
{
  if (m_item->IsPVRRecording())
  {
    return m_item->GetPVRRecordingInfoTag();
  }
  else if (m_item->IsEPG())
  {
    return CServiceBroker::GetPVRManager().Recordings()->GetRecordingForEpgTag(
        m_item->GetEPGInfoTag());
  }
  else if (URIUtils::IsPVR(m_item->GetDynPath()))
  {
    CLog::LogF(LOGERROR, "Unsupported item type!");
  }
  return {};
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
  else if (m_item->IsPVRTimer())
  {
    return m_item->GetPVRTimerInfoTag()->IsRadio();
  }
  else if (URIUtils::IsPVR(m_item->GetDynPath()))
  {
    CLog::LogF(LOGERROR, "Unsupported item type!");
  }
  return false;
}

} // namespace PVR
