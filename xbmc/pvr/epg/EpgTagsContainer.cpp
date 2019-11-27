/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgTagsContainer.h"

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "utils/log.h"

using namespace PVR;

void CPVREpgTagsContainer::SetEpgID(int iEpgID)
{
  m_iEpgID = iEpgID;
  for (const auto& tag : m_tags)
    tag.second->SetEpgID(iEpgID);
}

void CPVREpgTagsContainer::SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data)
{
  m_channelData = data;
  for (const auto& tag : m_tags)
    tag.second->SetChannelData(data);
}

bool CPVREpgTagsContainer::UpdateEntries(const CPVREpgTagsContainer& tags, bool bUpdateDatabase)
{
  for (const auto& tag : tags.m_tags)
    UpdateEntry(tag.second, bUpdateDatabase);

  FixOverlappingEvents(bUpdateDatabase);

  return true;
}

bool CPVREpgTagsContainer::FixOverlappingEvents(bool bUpdateDatabase)
{
  bool bReturn = false;
  std::shared_ptr<CPVREpgInfoTag> previousTag, currentTag;

  for (auto it = m_tags.begin(); it != m_tags.end(); it != m_tags.end() ? it++ : it)
  {
    if (!previousTag)
    {
      previousTag = it->second;
      continue;
    }
    currentTag = it->second;

    if (previousTag->EndAsUTC() >= currentTag->EndAsUTC())
    {
      // delete the current tag. it's completely overlapped
      if (bUpdateDatabase)
        m_deletedTags.insert(std::make_pair(currentTag->UniqueBroadcastID(), currentTag));

      if (m_nowActiveStart == it->first)
        m_nowActiveStart.SetValid(false);

      m_tags.erase(it++);
    }
    else if (previousTag->EndAsUTC() > currentTag->StartAsUTC())
    {
      previousTag->SetEndFromUTC(currentTag->StartAsUTC());
      if (bUpdateDatabase)
        m_changedTags.insert(std::make_pair(previousTag->UniqueBroadcastID(), previousTag));

      previousTag = it->second;
    }
    else
    {
      previousTag = it->second;
    }
  }

  return bReturn;
}

void CPVREpgTagsContainer::AddEntry(const CPVREpgInfoTag& tag)
{
  std::shared_ptr<CPVREpgInfoTag> newTag = GetTag(tag.StartAsUTC());
  if (!newTag)
  {
    newTag.reset(new CPVREpgInfoTag());
    m_tags.insert({tag.StartAsUTC(), newTag});
  }

  newTag->Update(tag);
  newTag->SetChannelData(m_channelData);
  newTag->SetEpgID(m_iEpgID);
}

bool CPVREpgTagsContainer::UpdateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag,
                                       bool bUpdateDatabase)
{
  bool bNewTag = false;

  std::shared_ptr<CPVREpgInfoTag> infoTag = GetTag(tag->StartAsUTC());
  if (!infoTag)
  {
    infoTag.reset(new CPVREpgInfoTag());
    infoTag->SetUniqueBroadcastID(tag->UniqueBroadcastID());
    m_tags.insert({tag->StartAsUTC(), infoTag});
    bNewTag = true;
  }

  infoTag->Update(*tag, bNewTag);
  infoTag->SetChannelData(m_channelData);
  infoTag->SetEpgID(m_iEpgID);

  if (bUpdateDatabase)
    m_changedTags.insert({infoTag->UniqueBroadcastID(), infoTag});

  return true;
}

bool CPVREpgTagsContainer::DeleteEntry(const std::shared_ptr<CPVREpgInfoTag>& tag,
                                       bool bUpdateDatabase)
{
  if (m_tags.erase(tag->StartAsUTC()) > 0)
  {
    if (bUpdateDatabase)
      m_deletedTags.insert(std::make_pair(tag->UniqueBroadcastID(), tag));

    if (m_nowActiveStart == tag->StartAsUTC())
      m_nowActiveStart.SetValid(false);

    return true;
  }
  return false;
}

void CPVREpgTagsContainer::Cleanup(const CDateTime& time)
{
  for (auto it = m_tags.begin(); it != m_tags.end();)
  {
    if (it->second->EndAsUTC() < time)
    {
      if (m_nowActiveStart == it->first)
        m_nowActiveStart.SetValid(false);

      it = m_tags.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void CPVREpgTagsContainer::Clear()
{
  m_tags.clear();
}

bool CPVREpgTagsContainer::IsEmpty() const
{
  return m_tags.empty();
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTag(const CDateTime& time) const
{
  const auto it = m_tags.find(time);
  if (it == m_tags.cend())
    return {};

  return (*it).second;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTag(unsigned int iUniqueBroadcastId) const
{
  if (iUniqueBroadcastId != EPG_TAG_INVALID_UID)
  {
    for (const auto& tag : m_tags)
    {
      if (tag.second->UniqueBroadcastID() == iUniqueBroadcastId)
        return tag.second;
    }
  }
  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTagBetween(const CDateTime& start,
                                                                    const CDateTime& end) const
{
  for (const auto& tag : m_tags)
  {
    if (tag.second->StartAsUTC() >= start)
    {
      if (tag.second->EndAsUTC() <= end)
        return tag.second;
      else
        break;
    }
  }
  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetActiveTag(bool bUpdateIfNeeded) const
{
  if (m_nowActiveStart.IsValid())
  {
    const auto it = m_tags.find(m_nowActiveStart);
    if (it != m_tags.end() && it->second->IsActive())
      return it->second;
  }

  if (bUpdateIfNeeded)
  {
    std::shared_ptr<CPVREpgInfoTag> lastActiveTag;

    // one of the first items will always match if the list is sorted
    for (const auto& tag : m_tags)
    {
      if (tag.second->IsActive())
      {
        m_nowActiveStart = tag.first;
        return tag.second;
      }
      else if (tag.second->WasActive())
        lastActiveTag = tag.second;
    }

    // there might be a gap between the last and next event. return the last if found and it
    // ended not more than 5 minutes ago
    if (lastActiveTag &&
        lastActiveTag->EndAsUTC() + CDateTimeSpan(0, 0, 5, 0) >= CDateTime::GetUTCDateTime())
      return lastActiveTag;
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetLastEndedTag() const
{
  for (auto it = m_tags.rbegin(); it != m_tags.rend(); ++it)
  {
    if (it->second->WasActive())
      return it->second;
  }
  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetFirstUpcomingTag() const
{
  for (const auto& tag : m_tags)
  {
    if (tag.second->IsUpcoming())
      return tag.second;
  }
  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetPredecessor(
    const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  auto it = m_tags.find(tag->StartAsUTC());
  if (it != m_tags.end() && it != m_tags.begin())
  {
    --it;
    return it->second;
  }
  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetSuccessor(
    const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  auto it = m_tags.find(tag->StartAsUTC());
  if (it != m_tags.end() && ++it != m_tags.end())
    return it->second;

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetFirstTag() const
{
  if (m_tags.empty())
    return {};

  return m_tags.cbegin()->second;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetLastTag() const
{
  if (m_tags.empty())
    return {};

  return m_tags.crbegin()->second;
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
  static const CDateTimeSpan ONE_SECOND(0, 0, 0, 1);

  std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;

  CDateTime lastEnd = minEventEnd;
  for (const auto& epgTag : m_tags)
  {
    if (epgTag.second->EndAsUTC() > minEventEnd)
    {
      const CDateTime start = epgTag.second->StartAsUTC();

      if (start <= maxEventStart)
      {
        if (start > minEventEnd && (start - lastEnd) > ONE_SECOND &&
            epgTag.second != (*m_tags.cbegin()).second)
        {
          // insert gap tag between two events
          tags.emplace_back(CreateGapTag(lastEnd, start - ONE_SECOND));
        }

        tags.emplace_back(epgTag.second);
      }
      else
      {
        if (tags.empty())
        {
          if (epgTag.second != (*m_tags.cbegin()).second)
          {
            // insert gap tag spanning pred of last checked event end to next event start
            tags.emplace_back(CreateGapTag(lastEnd, start - ONE_SECOND));
          }
        }
        else
        {
          if (maxEventStart >= tags.back()->EndAsUTC() && (start - ONE_SECOND) <= timelineEnd)
          {
            // append gap tag spanning last found event end to next event start
            tags.emplace_back(CreateGapTag(tags.back()->EndAsUTC(), start - ONE_SECOND));
          }

          if (minEventEnd <= tags.front()->StartAsUTC() - ONE_SECOND &&
              tags.front() != (*m_tags.cbegin()).second)
          {
            // prepend gap tag spanning pred of first found event end to first found event start
            tags.insert(tags.begin(),
                        CreateGapTag(lastEnd, tags.front()->StartAsUTC() - ONE_SECOND));
          }
        }
        break; // done. m_tags is sorted by date, ascending
      }
    }
    lastEnd = epgTag.second->EndAsUTC();
  }

  if (tags.empty())
  {
    if (m_tags.empty())
    {
      // insert gap tag spanning whole timeline
      tags.emplace_back(CreateGapTag(timelineStart, timelineEnd));
    }
    else if (maxEventStart <= (*m_tags.cbegin()).second->StartAsUTC())
    {
      // insert gap tag spanning timeline start to very first event start
      tags.emplace_back(
          CreateGapTag(timelineStart, (*m_tags.cbegin()).second->StartAsUTC() - ONE_SECOND));
    }
    else if (minEventEnd >= (*m_tags.crbegin()).second->EndAsUTC())
    {
      // insert gap tag spanning very last event end to timeline end
      tags.emplace_back(CreateGapTag((*m_tags.crbegin()).second->EndAsUTC(), timelineEnd));
    }
    else
    {
      CLog::LogF(LOGERROR, "Could not find any epgtag for requested timeline");
    }
  }
  else
  {
    if (tags.front() == (*m_tags.cbegin()).second && tags.front()->StartAsUTC() >= minEventEnd)
    {
      // prepend gap tag spanning timeline start to very first event start
      tags.insert(tags.begin(), CreateGapTag(timelineStart,
                                             (*m_tags.cbegin()).second->StartAsUTC() - ONE_SECOND));
    }

    if (tags.back() == (*m_tags.crbegin()).second && tags.back()->EndAsUTC() <= maxEventStart)
    {
      // append gap tag spanning very last event end to timeline end
      tags.emplace_back(CreateGapTag((*m_tags.crbegin()).second->EndAsUTC(), timelineEnd));
    }
  }

  return tags;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgTagsContainer::GetAllTags() const
{
  std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;

  for (const auto& tag : m_tags)
    tags.emplace_back(tag.second);

  return tags;
}

bool CPVREpgTagsContainer::NeedsSave() const
{
  return !m_changedTags.empty() || !m_deletedTags.empty();
}

void CPVREpgTagsContainer::Persist(const std::shared_ptr<CPVREpgDatabase>& database)
{
  for (const auto& tag : m_deletedTags)
    database->Delete(*tag.second);

  for (const auto& tag : m_changedTags)
    tag.second->Persist(database, false);

  m_deletedTags.clear();
  m_changedTags.clear();
}
