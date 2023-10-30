/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgTagsCache.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "utils/log.h"

#include <algorithm>

using namespace PVR;

namespace
{
const CDateTimeSpan ONE_SECOND(0, 0, 0, 1);
}

void CPVREpgTagsCache::SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data)
{
  m_channelData = data;

  if (m_lastEndedTag)
    m_lastEndedTag->SetChannelData(data);
  if (m_nowActiveTag)
    m_nowActiveTag->SetChannelData(data);
  if (m_nextStartingTag)
    m_nextStartingTag->SetChannelData(data);
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsCache::GetLastEndedTag()
{
  Refresh();
  return m_lastEndedTag;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsCache::GetNowActiveTag()
{
  Refresh();
  return m_nowActiveTag;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsCache::GetNextStartingTag()
{
  Refresh();
  return m_nextStartingTag;
}

void CPVREpgTagsCache::Reset()
{
  m_lastEndedTag.reset();

  m_nowActiveTag.reset();
  m_nowActiveStart.Reset();
  m_nowActiveEnd.Reset();

  m_nextStartingTag.reset();
}

bool CPVREpgTagsCache::Refresh()
{
  const CDateTime activeTime =
      CServiceBroker::GetPVRManager().PlaybackState()->GetChannelPlaybackTime(
          m_channelData->ClientId(), m_channelData->UniqueClientChannelId());

  if (m_nowActiveStart.IsValid() && m_nowActiveEnd.IsValid() && m_nowActiveStart <= activeTime &&
      m_nowActiveEnd > activeTime)
    return false;

  const std::shared_ptr<const CPVREpgInfoTag> prevNowActiveTag = m_nowActiveTag;

  m_lastEndedTag.reset();
  m_nowActiveTag.reset();
  m_nextStartingTag.reset();

  const auto it =
      std::find_if(m_changedTags.cbegin(), m_changedTags.cend(), [&activeTime](const auto& tag) {
        return tag.second->StartAsUTC() <= activeTime && tag.second->EndAsUTC() > activeTime;
      });

  if (it != m_changedTags.cend())
  {
    m_nowActiveTag = (*it).second;
    m_nowActiveStart = m_nowActiveTag->StartAsUTC();
    m_nowActiveEnd = m_nowActiveTag->EndAsUTC();
  }

  if (!m_nowActiveTag && m_database)
  {
    const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags =
        m_database->GetEpgTagsByMinEndMaxStartTime(m_iEpgID, activeTime + ONE_SECOND, activeTime);
    if (!tags.empty())
    {
      if (tags.size() > 1)
        CLog::LogF(LOGWARNING, "Got multiple results. Picking up the first.");

      m_nowActiveTag = tags.front();
      m_nowActiveTag->SetChannelData(m_channelData);
      m_nowActiveStart = m_nowActiveTag->StartAsUTC();
      m_nowActiveEnd = m_nowActiveTag->EndAsUTC();
    }
  }

  RefreshLastEndedTag(activeTime);
  RefreshNextStartingTag(activeTime);

  if (!m_nowActiveTag)
  {
    // we're in a gap. remember start and end time of that gap to avoid unneeded db load.
    if (m_lastEndedTag)
      m_nowActiveStart = m_lastEndedTag->EndAsUTC();
    else
      m_nowActiveStart = activeTime - CDateTimeSpan(1000, 0, 0, 0); // fake start far in the past

    if (m_nextStartingTag)
      m_nowActiveEnd = m_nextStartingTag->StartAsUTC();
    else
      m_nowActiveEnd = activeTime + CDateTimeSpan(1000, 0, 0, 0); // fake end far in the future
  }

  const bool tagChanged =
      m_nowActiveTag && (!prevNowActiveTag || *prevNowActiveTag != *m_nowActiveTag);
  const bool tagRemoved = !m_nowActiveTag && prevNowActiveTag;

  return (tagChanged || tagRemoved);
}

void CPVREpgTagsCache::RefreshLastEndedTag(const CDateTime& activeTime)
{
  if (m_database)
  {
    m_lastEndedTag = m_database->GetEpgTagByMaxEndTime(m_iEpgID, activeTime);
    if (m_lastEndedTag)
      m_lastEndedTag->SetChannelData(m_channelData);
  }

  for (auto it = m_changedTags.rbegin(); it != m_changedTags.rend(); ++it)
  {
    if (it->second->WasActive())
    {
      if (!m_lastEndedTag || m_lastEndedTag->EndAsUTC() < it->second->EndAsUTC())
      {
        m_lastEndedTag = it->second;
        break;
      }
    }
  }
}

void CPVREpgTagsCache::RefreshNextStartingTag(const CDateTime& activeTime)
{
  if (m_database)
  {
    m_nextStartingTag = m_database->GetEpgTagByMinStartTime(m_iEpgID, activeTime + ONE_SECOND);
    if (m_nextStartingTag)
      m_nextStartingTag->SetChannelData(m_channelData);
  }

  for (const auto& tag : m_changedTags)
  {
    if (tag.second->IsUpcoming())
    {
      if (!m_nextStartingTag || m_nextStartingTag->StartAsUTC() > tag.second->StartAsUTC())
      {
        m_nextStartingTag = tag.second;
        break;
      }
    }
  }
}
