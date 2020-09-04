/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgTagsContainer.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_epg.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgTagsCache.h"
#include "utils/log.h"

using namespace PVR;

namespace
{
const CDateTimeSpan ONE_SECOND(0, 0, 0, 1);
}

CPVREpgTagsContainer::CPVREpgTagsContainer(int iEpgID,
                                           const std::shared_ptr<CPVREpgChannelData>& channelData,
                                           const std::shared_ptr<CPVREpgDatabase>& database)
  : m_iEpgID(iEpgID),
    m_channelData(channelData),
    m_database(database),
    m_tagsCache(new CPVREpgTagsCache(iEpgID, channelData, database, m_changedTags))
{
}

CPVREpgTagsContainer::~CPVREpgTagsContainer() = default;

void CPVREpgTagsContainer::SetEpgID(int iEpgID)
{
  m_iEpgID = iEpgID;
  for (const auto& tag : m_changedTags)
    tag.second->SetEpgID(iEpgID);
}

void CPVREpgTagsContainer::SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data)
{
  m_channelData = data;
  m_tagsCache->SetChannelData(data);
  for (const auto& tag : m_changedTags)
    tag.second->SetChannelData(data);
}

namespace
{

void ResolveConflictingTags(const std::shared_ptr<CPVREpgInfoTag>& changedTag,
                            std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags)
{
  const CDateTime changedTagStart = changedTag->StartAsUTC();
  const CDateTime changedTagEnd = changedTag->EndAsUTC();

  for (auto it = tags.begin(); it != tags.end();)
  {
    bool bInsert = false;

    if (changedTagEnd > (*it)->StartAsUTC() && changedTagStart < (*it)->EndAsUTC())
    {
      it = tags.erase(it);

      if (it == tags.end())
      {
        bInsert = true;
      }
    }
    else if ((*it)->StartAsUTC() >= changedTagEnd)
    {
      bInsert = true;
    }
    else
    {
      ++it;
    }

    if (bInsert)
    {
      tags.emplace(it, changedTag);
      break;
    }
  }
}

bool FixOverlap(const std::shared_ptr<CPVREpgInfoTag>& previousTag,
                const std::shared_ptr<CPVREpgInfoTag>& currentTag)
{
  if (!previousTag)
    return true;

  if (previousTag->EndAsUTC() >= currentTag->EndAsUTC())
  {
    // delete the current tag. it's completely overlapped
    CLog::LogF(LOGWARNING,
               "Erasing completely overlapped event from EPG timeline "
               "(%u - %s - %s - %s) "
               "(%u - %s - %s - %s).",
               previousTag->UniqueBroadcastID(), previousTag->Title().c_str(),
               previousTag->StartAsUTC().GetAsDBDateTime(),
               previousTag->EndAsUTC().GetAsDBDateTime(), currentTag->UniqueBroadcastID(),
               currentTag->Title().c_str(), currentTag->StartAsUTC().GetAsDBDateTime(),
               currentTag->EndAsUTC().GetAsDBDateTime());

    return false;
  }
  else if (previousTag->EndAsUTC() > currentTag->StartAsUTC())
  {
    // fix the end time of the predecessor of the event
    CLog::LogF(LOGWARNING,
               "Fixing partly overlapped event in EPG timeline "
               "(%u - %s - %s - %s) "
               "(%u - %s - %s - %s).",
               previousTag->UniqueBroadcastID(), previousTag->Title().c_str(),
               previousTag->StartAsUTC().GetAsDBDateTime(),
               previousTag->EndAsUTC().GetAsDBDateTime(), currentTag->UniqueBroadcastID(),
               currentTag->Title().c_str(), currentTag->StartAsUTC().GetAsDBDateTime(),
               currentTag->EndAsUTC().GetAsDBDateTime());

    previousTag->SetEndFromUTC(currentTag->StartAsUTC());
  }
  return true;
}

} // unnamed namespace

bool CPVREpgTagsContainer::UpdateEntries(const CPVREpgTagsContainer& tags)
{
  if (tags.m_changedTags.empty())
    return false;

  if (m_database)
  {
    const CDateTime minEventEnd = (*tags.m_changedTags.cbegin()).second->StartAsUTC() + ONE_SECOND;
    const CDateTime maxEventStart = (*tags.m_changedTags.crbegin()).second->EndAsUTC();

    std::vector<std::shared_ptr<CPVREpgInfoTag>> existingTags =
        m_database->GetEpgTagsByMinEndMaxStartTime(m_iEpgID, minEventEnd, maxEventStart);

    if (!m_changedTags.empty())
    {
      // Fix data inconsistencies
      for (const auto& changedTagsEntry : m_changedTags)
      {
        const auto& changedTag = changedTagsEntry.second;

        if (changedTag->EndAsUTC() > minEventEnd && changedTag->StartAsUTC() < maxEventStart)
        {
          // tag is in queried range, thus it could cause inconsistencies...
          ResolveConflictingTags(changedTag, existingTags);
        }
      }
    }

    bool bResetCache = false;
    for (const auto& tagsEntry : tags.m_changedTags)
    {
      const auto& tag = tagsEntry.second;

      tag->SetChannelData(m_channelData);
      tag->SetEpgID(m_iEpgID);

      std::shared_ptr<CPVREpgInfoTag> existingTag;
      for (const auto& t : existingTags)
      {
        if (t->StartAsUTC() == tag->StartAsUTC())
        {
          existingTag = t;
          break;
        }
      }

      if (existingTag)
      {
        existingTag->SetChannelData(m_channelData);
        existingTag->SetEpgID(m_iEpgID);

        if (existingTag->Update(*tag, false))
        {
          // tag differs from existing tag and must be persisted
          m_changedTags.insert({existingTag->StartAsUTC(), existingTag});
          bResetCache = true;
        }
      }
      else
      {
        // new tags must always be persisted
        m_changedTags.insert({tag->StartAsUTC(), tag});
        bResetCache = true;
      }
    }

    if (bResetCache)
      m_tagsCache->Reset();
  }
  else
  {
    for (const auto& tag : tags.m_changedTags)
      UpdateEntry(tag.second);
  }

  return true;
}

void CPVREpgTagsContainer::FixOverlappingEvents(
    std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags) const
{
  std::shared_ptr<CPVREpgInfoTag> previousTag;
  for (auto it = tags.begin(); it != tags.end();)
  {
    const std::shared_ptr<CPVREpgInfoTag> currentTag = *it;
    if (FixOverlap(previousTag, currentTag))
    {
      previousTag = currentTag;
      ++it;
    }
    else
    {
      it = tags.erase(it);
      m_tagsCache->Reset();
    }
  }
}

void CPVREpgTagsContainer::FixOverlappingEvents(
    std::map<CDateTime, std::shared_ptr<CPVREpgInfoTag>>& tags) const
{
  std::shared_ptr<CPVREpgInfoTag> previousTag;
  for (auto it = tags.begin(); it != tags.end();)
  {
    const std::shared_ptr<CPVREpgInfoTag> currentTag = (*it).second;
    if (FixOverlap(previousTag, currentTag))
    {
      previousTag = currentTag;
      ++it;
    }
    else
    {
      it = tags.erase(it);
      m_tagsCache->Reset();
    }
  }
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::CreateEntry(
    const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  if (tag)
  {
    tag->SetChannelData(m_channelData);
  }
  return tag;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgTagsContainer::CreateEntries(
    const std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags) const
{
  for (auto& tag : tags)
  {
    tag->SetChannelData(m_channelData);
  }
  return tags;
}

bool CPVREpgTagsContainer::UpdateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  tag->SetChannelData(m_channelData);
  tag->SetEpgID(m_iEpgID);

  std::shared_ptr<CPVREpgInfoTag> existingTag = GetTag(tag->StartAsUTC());
  if (existingTag)
  {
    if (existingTag->Update(*tag, false))
    {
      // tag differs from existing tag and must be persisted
      m_changedTags.insert({existingTag->StartAsUTC(), existingTag});
      m_tagsCache->Reset();
    }
  }
  else
  {
    // new tags must always be persisted
    m_changedTags.insert({tag->StartAsUTC(), tag});
    m_tagsCache->Reset();
  }

  return true;
}

bool CPVREpgTagsContainer::DeleteEntry(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  m_changedTags.erase(tag->StartAsUTC());
  m_deletedTags.insert({tag->StartAsUTC(), tag});
  m_tagsCache->Reset();
  return true;
}

void CPVREpgTagsContainer::Cleanup(const CDateTime& time)
{
  for (auto it = m_changedTags.begin(); it != m_changedTags.end();)
  {
    if (it->second->EndAsUTC() < time)
    {
      m_tagsCache->Reset();

      const auto it1 = m_deletedTags.find(it->first);
      if (it1 != m_deletedTags.end())
        m_deletedTags.erase(it1);

      it = m_changedTags.erase(it);
    }
    else
    {
      ++it;
    }
  }

  if (m_database)
    m_database->DeleteEpgTags(m_iEpgID, time);
}

void CPVREpgTagsContainer::Clear()
{
  m_changedTags.clear();
}

bool CPVREpgTagsContainer::IsEmpty() const
{
  if (!m_changedTags.empty())
    return false;

  if (m_database)
    return !m_database->GetFirstStartTime(m_iEpgID).IsValid();

  return true;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTag(const CDateTime& startTime) const
{
  const auto it = m_changedTags.find(startTime);
  if (it != m_changedTags.cend())
    return (*it).second;

  if (m_database)
    return CreateEntry(m_database->GetEpgTagByStartTime(m_iEpgID, startTime));

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTag(unsigned int iUniqueBroadcastID) const
{
  if (iUniqueBroadcastID == EPG_TAG_INVALID_UID)
    return {};

  for (const auto& tag : m_changedTags)
  {
    if (tag.second->UniqueBroadcastID() == iUniqueBroadcastID)
      return tag.second;
  }

  if (m_database)
    return CreateEntry(m_database->GetEpgTagByUniqueBroadcastID(m_iEpgID, iUniqueBroadcastID));

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTagBetween(const CDateTime& start,
                                                                    const CDateTime& end) const
{
  for (const auto& tag : m_changedTags)
  {
    if (tag.second->StartAsUTC() >= start)
    {
      if (tag.second->EndAsUTC() <= end)
        return tag.second;
      else
        break;
    }
  }

  if (m_database)
  {
    const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags =
        CreateEntries(m_database->GetEpgTagsByMinStartMaxEndTime(m_iEpgID, start, end));
    if (!tags.empty())
    {
      if (tags.size() > 1)
        CLog::LogF(LOGWARNING, "Got multiple tags. Picking up the first.");

      return tags.front();
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetActiveTag(bool bUpdateIfNeeded) const
{
  return m_tagsCache->GetNowActiveTag(bUpdateIfNeeded);
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetLastEndedTag() const
{
  return m_tagsCache->GetLastEndedTag();
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetNextStartingTag() const
{
  return m_tagsCache->GetNextStartingTag();
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::CreateGapTag(const CDateTime& start,
                                                                   const CDateTime& end) const
{
  return std::make_shared<CPVREpgInfoTag>(m_channelData, m_iEpgID, start, end, true);
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgTagsContainer::GetTimeline(
    const CDateTime& timelineStart,
    const CDateTime& timelineEnd,
    const CDateTime& minEventEnd,
    const CDateTime& maxEventStart) const
{
  if (m_database)
  {
    std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;

    if (!m_changedTags.empty() && !m_database->GetFirstStartTime(m_iEpgID).IsValid())
    {
      // nothing in the db yet. take what we have in memory.
      for (const auto& tag : m_changedTags)
      {
        if (tag.second->EndAsUTC() > minEventEnd && tag.second->StartAsUTC() < maxEventStart)
          tags.emplace_back(tag.second);
      }

      if (!tags.empty())
        FixOverlappingEvents(tags);
    }
    else
    {
      tags = m_database->GetEpgTagsByMinEndMaxStartTime(m_iEpgID, minEventEnd, maxEventStart);

      if (!m_changedTags.empty())
      {
        // Fix data inconsistencies
        for (const auto& changedTagsEntry : m_changedTags)
        {
          const auto& changedTag = changedTagsEntry.second;

          if (changedTag->EndAsUTC() > minEventEnd && changedTag->StartAsUTC() < maxEventStart)
          {
            // tag is in queried range, thus it could cause inconsistencies...
            ResolveConflictingTags(changedTag, tags);
          }
        }
      }
    }

    tags = CreateEntries(tags);

    std::vector<std::shared_ptr<CPVREpgInfoTag>> result;

    for (const auto& epgTag : tags)
    {
      if (!result.empty())
      {
        const CDateTime currStart = epgTag->StartAsUTC();
        const CDateTime prevEnd = result.back()->EndAsUTC();
        if ((currStart - prevEnd) >= ONE_SECOND)
        {
          // insert gap tag before current tag
          result.emplace_back(CreateGapTag(prevEnd, currStart));
        }
      }

      result.emplace_back(epgTag);
    }

    if (result.empty())
    {
      // create single gap tag
      CDateTime maxEnd = m_database->GetMaxEndTime(m_iEpgID, minEventEnd);
      if (!maxEnd.IsValid() || maxEnd < timelineStart)
        maxEnd = timelineStart;

      CDateTime minStart = m_database->GetMinStartTime(m_iEpgID, maxEventStart);
      if (!minStart.IsValid() || minStart > timelineEnd)
        minStart = timelineEnd;

      result.emplace_back(CreateGapTag(maxEnd, minStart));
    }
    else
    {
      if (result.front()->StartAsUTC() > minEventEnd)
      {
        // prepend gap tag
        CDateTime maxEnd = m_database->GetMaxEndTime(m_iEpgID, minEventEnd);
        if (!maxEnd.IsValid() || maxEnd < timelineStart)
          maxEnd = timelineStart;

        result.insert(result.begin(), CreateGapTag(maxEnd, result.front()->StartAsUTC()));
      }

      if (result.back()->EndAsUTC() < maxEventStart)
      {
        // append gap tag
        CDateTime minStart = m_database->GetMinStartTime(m_iEpgID, maxEventStart);
        if (!minStart.IsValid() || minStart > timelineEnd)
          minStart = timelineEnd;

        result.emplace_back(CreateGapTag(result.back()->EndAsUTC(), minStart));
      }
    }

    return result;
  }

  return {};
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgTagsContainer::GetAllTags() const
{
  if (m_database)
  {
    std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;
    if (!m_changedTags.empty() && !m_database->GetFirstStartTime(m_iEpgID).IsValid())
    {
      // nothing in the db yet. take what we have in memory.
      for (const auto& tag : m_changedTags)
        tags.emplace_back(tag.second);

      FixOverlappingEvents(tags);
    }
    else
    {
      tags = m_database->GetAllEpgTags(m_iEpgID);

      if (!m_changedTags.empty())
      {
        // Fix data inconsistencies
        for (const auto& changedTagsEntry : m_changedTags)
        {
          ResolveConflictingTags(changedTagsEntry.second, tags);
        }
      }
    }

    return CreateEntries(tags);
  }

  return {};
}

CDateTime CPVREpgTagsContainer::GetFirstStartTime() const
{
  CDateTime result;

  if (!m_changedTags.empty())
    result = (*m_changedTags.cbegin()).second->StartAsUTC();

  if (m_database)
  {
    const CDateTime dbResult = m_database->GetFirstStartTime(m_iEpgID);
    if (!result.IsValid() || (dbResult.IsValid() && dbResult < result))
      result = dbResult;
  }

  return result;
}

CDateTime CPVREpgTagsContainer::GetLastEndTime() const
{
  CDateTime result;

  if (!m_changedTags.empty())
    result = (*m_changedTags.crbegin()).second->EndAsUTC();

  if (m_database)
  {
    const CDateTime dbResult = m_database->GetLastEndTime(m_iEpgID);
    if (result.IsValid() || (dbResult.IsValid() && dbResult > result))
      result = dbResult;
  }

  return result;
}

bool CPVREpgTagsContainer::NeedsSave() const
{
  return !m_changedTags.empty() || !m_deletedTags.empty();
}

void CPVREpgTagsContainer::Persist(bool bQueueWrite)
{
  if (m_database)
  {
    m_database->Lock();

    CLog::Log(LOGDEBUG, "EPG Tags Container: Updating %d, deleting %d events...",
              m_changedTags.size(), m_deletedTags.size());

    for (const auto& tag : m_deletedTags)
      m_database->Delete(*tag.second, !bQueueWrite);

    m_deletedTags.clear();

    FixOverlappingEvents(m_changedTags);

    for (const auto& tag : m_changedTags)
    {
      // remove any conflicting events from database before persisting the new event
      m_database->DeleteEpgTagsByMinEndMaxStartTime(m_iEpgID, tag.second->StartAsUTC() + ONE_SECOND,
                                                    tag.second->EndAsUTC() - ONE_SECOND,
                                                    !bQueueWrite);

      tag.second->Persist(m_database, !bQueueWrite);
    }

    m_changedTags.clear();

    m_database->Unlock();
  }
}

void CPVREpgTagsContainer::Delete()
{
  if (m_database)
    m_database->DeleteEpgTags(m_iEpgID);

  Clear();
}
