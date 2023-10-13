/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIEPGGridContainerModel.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <vector>

using namespace PVR;

static const unsigned int GRID_START_PADDING = 30; // minutes

void CGUIEPGGridContainerModel::SetInvalid()
{
  for (const auto& gridItem : m_gridIndex)
    gridItem.second.item->SetInvalid();
  for (const auto& channel : m_channelItems)
    channel->SetInvalid();
  for (const auto& ruler : m_rulerItems)
    ruler->SetInvalid();
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::CreateGapItem(int iChannel) const
{
  const std::shared_ptr<CPVRChannel> channel = m_channelItems[iChannel]->GetPVRChannelInfoTag();
  const std::shared_ptr<CPVREpgInfoTag> gapTag = channel->CreateEPGGapTag(m_gridStart, m_gridEnd);
  return std::make_shared<CFileItem>(gapTag);
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CGUIEPGGridContainerModel::GetEPGTimeline(
    int iChannel, const CDateTime& minEventEnd, const CDateTime& maxEventStart) const
{
  CDateTime min = minEventEnd - CDateTimeSpan(0, 0, MINSPERBLOCK, 0) + CDateTimeSpan(0, 0, 0, 1);
  CDateTime max = maxEventStart + CDateTimeSpan(0, 0, MINSPERBLOCK, 0);

  if (min < m_gridStart)
    min = m_gridStart;

  if (max > m_gridEnd)
    max = m_gridEnd;

  return m_channelItems[iChannel]->GetPVRChannelInfoTag()->GetEPGTimeline(m_gridStart, m_gridEnd,
                                                                          min, max);
}

void CGUIEPGGridContainerModel::Initialize(const std::unique_ptr<CFileItemList>& items,
                                           const CDateTime& gridStart,
                                           const CDateTime& gridEnd,
                                           int iFirstChannel,
                                           int iChannelsPerPage,
                                           int iFirstBlock,
                                           int iBlocksPerPage,
                                           int iRulerUnit,
                                           float fBlockSize)
{
  if (!m_channelItems.empty())
  {
    CLog::LogF(LOGERROR, "Already initialized!");
    return;
  }

  m_fBlockSize = fBlockSize;

  ////////////////////////////////////////////////////////////////////////
  // Create channel items
  std::copy(items->cbegin(), items->cend(), std::back_inserter(m_channelItems));

  /* check for invalid start and end time */
  if (gridStart >= gridEnd)
  {
    // default to start "now minus GRID_START_PADDING minutes" and end "start plus one page".
    m_gridStart = CDateTime::GetUTCDateTime() - CDateTimeSpan(0, 0, GetGridStartPadding(), 0);
    m_gridEnd = m_gridStart + CDateTimeSpan(0, 0, iBlocksPerPage * MINSPERBLOCK, 0);
  }
  else if (gridStart >
           (CDateTime::GetUTCDateTime() - CDateTimeSpan(0, 0, GetGridStartPadding(), 0)))
  {
    // adjust to start "now minus GRID_START_PADDING minutes".
    m_gridStart = CDateTime::GetUTCDateTime() - CDateTimeSpan(0, 0, GetGridStartPadding(), 0);
    m_gridEnd = gridEnd;
  }
  else
  {
    m_gridStart = gridStart;
    m_gridEnd = gridEnd;
  }

  // roundup
  m_gridStart = CDateTime(m_gridStart.GetYear(), m_gridStart.GetMonth(), m_gridStart.GetDay(),
                          m_gridStart.GetHour(), m_gridStart.GetMinute() >= 30 ? 30 : 0, 0);
  m_gridEnd = CDateTime(m_gridEnd.GetYear(), m_gridEnd.GetMonth(), m_gridEnd.GetDay(),
                        m_gridEnd.GetHour(), m_gridEnd.GetMinute() >= 30 ? 30 : 0, 0);

  m_blocks = GetBlock(m_gridEnd) + 1;

  const int iBlocksLastPage = m_blocks % iBlocksPerPage;
  if (iBlocksLastPage > 0)
  {
    m_gridEnd += CDateTimeSpan(0, 0, (iBlocksPerPage - iBlocksLastPage) * MINSPERBLOCK, 0);
    m_blocks += (iBlocksPerPage - iBlocksLastPage);
  }

  ////////////////////////////////////////////////////////////////////////
  // Create ruler items
  CDateTime ruler;
  ruler.SetFromUTCDateTime(m_gridStart);
  CDateTime rulerEnd;
  rulerEnd.SetFromUTCDateTime(m_gridEnd);
  CFileItemPtr rulerItem(new CFileItem(ruler.GetAsLocalizedDate(true)));
  rulerItem->SetProperty("DateLabel", true);
  m_rulerItems.emplace_back(rulerItem);

  const CDateTimeSpan unit(0, 0, iRulerUnit * MINSPERBLOCK, 0);
  for (; ruler < rulerEnd; ruler += unit)
  {
    rulerItem = std::make_shared<CFileItem>(ruler.GetAsLocalizedTime("", false));
    rulerItem->SetLabel2(ruler.GetAsLocalizedDate(true));
    m_rulerItems.emplace_back(rulerItem);
  }

  m_firstActiveChannel = iFirstChannel;
  m_lastActiveChannel = iFirstChannel + iChannelsPerPage - 1;
  m_firstActiveBlock = iFirstBlock;
  m_lastActiveBlock = iFirstBlock + iBlocksPerPage - 1;
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::CreateEpgTags(int iChannel, int iBlock) const
{
  std::shared_ptr<CFileItem> result;

  const int firstBlock = iBlock < m_firstActiveBlock ? iBlock : m_firstActiveBlock;
  const int lastBlock = iBlock > m_lastActiveBlock ? iBlock : m_lastActiveBlock;

  const auto tags =
      GetEPGTimeline(iChannel, GetStartTimeForBlock(firstBlock), GetStartTimeForBlock(lastBlock));

  const int firstResultBlock = GetFirstEventBlock(tags.front());
  const int lastResultBlock = GetLastEventBlock(tags.back());
  if (firstResultBlock > lastResultBlock)
    return result;

  auto it = m_epgItems.insert({iChannel, EpgTags()}).first;
  EpgTags& epgTags = (*it).second;

  epgTags.firstBlock = firstResultBlock;
  epgTags.lastBlock = lastResultBlock;

  for (const auto& tag : tags)
  {
    if (GetFirstEventBlock(tag) > GetLastEventBlock(tag))
      continue;

    const std::shared_ptr<CFileItem> item = std::make_shared<CFileItem>(tag);
    if (!result && IsEventMemberOfBlock(tag, iBlock))
      result = item;

    epgTags.tags.emplace_back(item);
  }

  return result;
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::GetEpgTags(EpgTagsMap::iterator& itEpg,
                                                                 int iChannel,
                                                                 int iBlock) const
{
  std::shared_ptr<CFileItem> result;

  EpgTags& epgTags = (*itEpg).second;

  if (iBlock < epgTags.firstBlock)
  {
    result = GetEpgTagsBefore(epgTags, iChannel, iBlock);
  }
  else if (iBlock > epgTags.lastBlock)
  {
    result = GetEpgTagsAfter(epgTags, iChannel, iBlock);
  }
  else
  {
    const auto it =
        std::find_if(epgTags.tags.cbegin(), epgTags.tags.cend(), [this, iBlock](const auto& item) {
          return IsEventMemberOfBlock(item->GetEPGInfoTag(), iBlock);
        });
    if (it != epgTags.tags.cend())
      result = (*it);
  }

  return result;
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::GetEpgTagsBefore(EpgTags& epgTags,
                                                                       int iChannel,
                                                                       int iBlock) const
{
  std::shared_ptr<CFileItem> result;

  int lastBlock = epgTags.firstBlock - 1;
  if (lastBlock < 0)
    lastBlock = 0;

  const auto tags =
      GetEPGTimeline(iChannel, GetStartTimeForBlock(iBlock), GetStartTimeForBlock(lastBlock));

  if (epgTags.lastBlock == -1)
    epgTags.lastBlock = lastBlock;

  if (tags.empty())
  {
    epgTags.firstBlock = iBlock;
  }
  else
  {
    const int firstResultBlock = GetFirstEventBlock(tags.front());
    const int lastResultBlock = GetLastEventBlock(tags.back());
    if (firstResultBlock > lastResultBlock)
      return result;

    // insert before the existing tags
    epgTags.firstBlock = firstResultBlock;

    auto it = tags.crbegin();
    if (!epgTags.tags.empty())
    {
      // ptr comp does not work for gap tags!
      // if ((*it) == epgTags.tags.front()->GetEPGInfoTag())

      const std::shared_ptr<CPVREpgInfoTag> t = epgTags.tags.front()->GetEPGInfoTag();
      if ((*it)->StartAsUTC() == t->StartAsUTC() && (*it)->EndAsUTC() == t->EndAsUTC())
      {
        if (!result && IsEventMemberOfBlock(*it, iBlock))
          result = epgTags.tags.front();

        ++it; // skip, because we already have that epg tag
      }
    }

    for (; it != tags.crend(); ++it)
    {
      if (GetFirstEventBlock(*it) > GetLastEventBlock(*it))
        continue;

      const std::shared_ptr<CFileItem> item = std::make_shared<CFileItem>(*it);
      if (!result && IsEventMemberOfBlock(*it, iBlock))
        result = item;

      epgTags.tags.insert(epgTags.tags.begin(), item);
    }
  }

  return result;
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::GetEpgTagsAfter(EpgTags& epgTags,
                                                                      int iChannel,
                                                                      int iBlock) const
{
  std::shared_ptr<CFileItem> result;

  int firstBlock = epgTags.lastBlock + 1;
  if (firstBlock >= GetLastBlock())
    firstBlock = GetLastBlock();

  const auto tags =
      GetEPGTimeline(iChannel, GetStartTimeForBlock(firstBlock), GetStartTimeForBlock(iBlock));

  if (epgTags.firstBlock == -1)
    epgTags.firstBlock = firstBlock;

  if (tags.empty())
  {
    epgTags.lastBlock = iBlock;
  }
  else
  {
    const int firstResultBlock = GetFirstEventBlock(tags.front());
    const int lastResultBlock = GetLastEventBlock(tags.back());
    if (firstResultBlock > lastResultBlock)
      return result;

    // append to the existing tags
    epgTags.lastBlock = lastResultBlock;

    auto it = tags.cbegin();
    if (!epgTags.tags.empty())
    {
      // ptr comp does not work for gap tags!
      // if ((*it) == epgTags.tags.back()->GetEPGInfoTag())

      const std::shared_ptr<CPVREpgInfoTag> t = epgTags.tags.back()->GetEPGInfoTag();
      if ((*it)->StartAsUTC() == t->StartAsUTC() && (*it)->EndAsUTC() == t->EndAsUTC())
      {
        if (!result && IsEventMemberOfBlock(*it, iBlock))
          result = epgTags.tags.back();

        ++it; // skip, because we already have that epg tag
      }
    }

    for (; it != tags.cend(); ++it)
    {
      if (GetFirstEventBlock(*it) > GetLastEventBlock(*it))
        continue;

      const std::shared_ptr<CFileItem> item = std::make_shared<CFileItem>(*it);
      if (!result && IsEventMemberOfBlock(*it, iBlock))
        result = item;

      epgTags.tags.emplace_back(item);
    }
  }

  return result;
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::GetItem(int iChannel, int iBlock) const
{
  std::shared_ptr<CFileItem> result;

  auto itEpg = m_epgItems.find(iChannel);
  if (itEpg == m_epgItems.end())
  {
    result = CreateEpgTags(iChannel, iBlock);
  }
  else
  {
    result = GetEpgTags(itEpg, iChannel, iBlock);
  }

  if (!result)
  {
    // Must never happen. if it does, fix the root cause, don't tolerate nullptr!
    CLog::LogF(LOGERROR, "EPG tag ({}, {}) not found!", iChannel, iBlock);
  }

  return result;
}

void CGUIEPGGridContainerModel::FindChannelAndBlockIndex(int channelUid,
                                                         unsigned int broadcastUid,
                                                         int eventOffset,
                                                         int& newChannelIndex,
                                                         int& newBlockIndex) const
{
  newChannelIndex = INVALID_INDEX;
  newBlockIndex = INVALID_INDEX;

  // find the new channel index
  int iCurrentChannel = 0;
  for (const auto& channel : m_channelItems)
  {
    if (channel->GetPVRChannelInfoTag()->UniqueID() == channelUid)
    {
      newChannelIndex = iCurrentChannel;

      // find the new block index
      const std::shared_ptr<CPVREpg> epg = channel->GetPVRChannelInfoTag()->GetEPG();
      if (epg)
      {
        const std::shared_ptr<CPVREpgInfoTag> tag = epg->GetTagByBroadcastId(broadcastUid);
        if (tag)
          newBlockIndex = GetFirstEventBlock(tag) + eventOffset;
      }
      break; // done
    }
    iCurrentChannel++;
  }
}

GridItem* CGUIEPGGridContainerModel::GetGridItemPtr(int iChannel, int iBlock) const
{
  auto it = m_gridIndex.find({iChannel, iBlock});
  if (it == m_gridIndex.end())
  {
    const CDateTime startTime = GetStartTimeForBlock(iBlock);
    if (startTime < m_gridStart || m_gridEnd < startTime)
    {
      CLog::LogF(LOGERROR, "Requested EPG tag ({}, {}) outside grid boundaries!", iChannel, iBlock);
      return nullptr;
    }

    const std::shared_ptr<CFileItem> item = GetItem(iChannel, iBlock);
    if (!item)
    {
      CLog::LogF(LOGERROR, "Got no EPG tag ({}, {})!", iChannel, iBlock);
      return nullptr;
    }

    const std::shared_ptr<CPVREpgInfoTag> epgTag = item->GetEPGInfoTag();

    const int startBlock = GetFirstEventBlock(epgTag);
    const int endBlock = GetLastEventBlock(epgTag);

    //! @todo it seems that this should be done somewhere else. CFileItem ctor maybe.
    item->SetProperty("GenreType", epgTag->GenreType());

    const float fItemWidth = (endBlock - startBlock + 1) * m_fBlockSize;
    it = m_gridIndex.insert({{iChannel, iBlock}, {item, fItemWidth, startBlock, endBlock}}).first;
  }

  return &(*it).second;
}

bool CGUIEPGGridContainerModel::IsSameGridItem(int iChannel, int iBlock1, int iBlock2) const
{
  if (iBlock1 == iBlock2)
    return true;

  const GridItem* item1 = GetGridItemPtr(iChannel, iBlock1);
  const GridItem* item2 = GetGridItemPtr(iChannel, iBlock2);

  // compare the instances, not instance pointers, pointers are not unique.
  return *item1 == *item2;
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::GetGridItem(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->item;
}

int CGUIEPGGridContainerModel::GetGridItemStartBlock(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->startBlock;
}

int CGUIEPGGridContainerModel::GetGridItemEndBlock(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->endBlock;
}

CDateTime CGUIEPGGridContainerModel::GetGridItemEndTime(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->item->GetEPGInfoTag()->EndAsUTC();
}

float CGUIEPGGridContainerModel::GetGridItemWidth(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->width;
}

float CGUIEPGGridContainerModel::GetGridItemOriginWidth(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->originWidth;
}

void CGUIEPGGridContainerModel::DecreaseGridItemWidth(int iChannel, int iBlock, float fSize)
{
  auto it = m_gridIndex.find({iChannel, iBlock});
  if (it != m_gridIndex.end() && (*it).second.width != ((*it).second.originWidth - fSize))
    (*it).second.width = (*it).second.originWidth - fSize;
}

unsigned int CGUIEPGGridContainerModel::GetGridStartPadding() const
{
  unsigned int iPastMinutes =
      CServiceBroker::GetPVRManager().EpgContainer().GetPastDaysToDisplay() * 24 * 60;

  if (iPastMinutes < GRID_START_PADDING)
    return iPastMinutes;

  return GRID_START_PADDING; // minutes
}

void CGUIEPGGridContainerModel::FreeChannelMemory(int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  {
    // remove before keepStart and after keepEnd
    for (int i = 0; i < keepStart && i < ChannelItemsSize(); ++i)
      m_channelItems[i]->FreeMemory();
    for (int i = keepEnd + 1; i < ChannelItemsSize(); ++i)
      m_channelItems[i]->FreeMemory();
  }
  else
  {
    // wrapping
    for (int i = keepEnd + 1; i < keepStart && i < ChannelItemsSize(); ++i)
      m_channelItems[i]->FreeMemory();
  }
}

bool CGUIEPGGridContainerModel::FreeProgrammeMemory(int firstChannel,
                                                    int lastChannel,
                                                    int firstBlock,
                                                    int lastBlock)
{
  const bool channelsChanged =
      (firstChannel != m_firstActiveChannel || lastChannel != m_lastActiveChannel);
  const bool blocksChanged = (firstBlock != m_firstActiveBlock || lastBlock != m_lastActiveBlock);
  if (!channelsChanged && !blocksChanged)
    return false;

  // clear the grid. it will be recreated on-demand.
  m_gridIndex.clear();

  bool newChannels = false;

  if (channelsChanged)
  {
    // purge epg tags for inactive channels
    for (auto it = m_epgItems.begin(); it != m_epgItems.end();)
    {
      if ((*it).first < firstChannel || (*it).first > lastChannel)
      {
        it = m_epgItems.erase(it);
        continue; // next channel
      }
      ++it;
    }

    newChannels = (firstChannel < m_firstActiveChannel) || (lastChannel > m_lastActiveChannel);
  }

  if (blocksChanged || newChannels)
  {
    // clear and refetch epg tags for active channels
    const CDateTime maxEnd = GetStartTimeForBlock(firstBlock);
    const CDateTime minStart = GetStartTimeForBlock(lastBlock);
    std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;
    for (int i = firstChannel; i <= lastChannel; ++i)
    {
      auto it = m_epgItems.find(i);
      if (it == m_epgItems.end())
        it = m_epgItems.insert({i, EpgTags()}).first;

      if (blocksChanged || i < m_firstActiveChannel || i > m_lastActiveChannel)
      {
        EpgTags& epgTags = (*it).second;

        (*it).second.tags.clear();

        tags = GetEPGTimeline(i, maxEnd, minStart);
        const int firstResultBlock = GetFirstEventBlock(tags.front());
        const int lastResultBlock = GetLastEventBlock(tags.back());
        if (firstResultBlock > lastResultBlock)
          continue;

        epgTags.firstBlock = firstResultBlock;
        epgTags.lastBlock = lastResultBlock;

        for (const auto& tag : tags)
        {
          if (GetFirstEventBlock(tag) > GetLastEventBlock(tag))
            continue;

          epgTags.tags.emplace_back(std::make_shared<CFileItem>(tag));
        }
      }
    }
  }

  m_firstActiveChannel = firstChannel;
  m_lastActiveChannel = lastChannel;
  m_firstActiveBlock = firstBlock;
  m_lastActiveBlock = lastBlock;

  return true;
}

void CGUIEPGGridContainerModel::FreeRulerMemory(int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  {
    // remove before keepStart and after keepEnd
    for (int i = 1; i < keepStart && i < RulerItemsSize(); ++i)
      m_rulerItems[i]->FreeMemory();
    for (int i = keepEnd + 1; i < RulerItemsSize(); ++i)
      m_rulerItems[i]->FreeMemory();
  }
  else
  {
    // wrapping
    for (int i = keepEnd + 1; i < keepStart && i < RulerItemsSize(); ++i)
    {
      if (i == 0)
        continue;

      m_rulerItems[i]->FreeMemory();
    }
  }
}

unsigned int CGUIEPGGridContainerModel::GetPageNowOffset() const
{
  return GetGridStartPadding() / MINSPERBLOCK; // this is the 'now' block relative to page start
}

CDateTime CGUIEPGGridContainerModel::GetStartTimeForBlock(int block) const
{
  if (block < 0)
    block = 0;
  else if (block >= GridItemsSize())
    block = GetLastBlock();

  return m_gridStart + CDateTimeSpan(0, 0, block * MINSPERBLOCK, 0);
}

int CGUIEPGGridContainerModel::GetBlock(const CDateTime& datetime) const
{
  int diff;

  if (m_gridStart == datetime)
    return 0; // block is at grid start
  else if (m_gridStart > datetime)
    diff = -1 * (m_gridStart - datetime).GetSecondsTotal(); // block is before grid start
  else
    diff = (datetime - m_gridStart).GetSecondsTotal(); // block is after grid start

  // Note: Subtract 1 second from diff to ensure that events ending exactly at block boundary
  //       are unambiguous. Example: An event ending at 5:00:00 shall be mapped to block 9 and
  //       an event starting at 5:00:00 shall be mapped to block 10, not both at block 10.
  //       Only exception is grid end, because there is no successor.
  if (datetime >= m_gridEnd)
    return diff / 60 / MINSPERBLOCK; // block is equal or after grid end
  else
    return (diff - 1) / 60 / MINSPERBLOCK;
}

int CGUIEPGGridContainerModel::GetNowBlock() const
{
  return GetBlock(CDateTime::GetUTCDateTime()) - GetPageNowOffset();
}

int CGUIEPGGridContainerModel::GetFirstEventBlock(
    const std::shared_ptr<CPVREpgInfoTag>& event) const
{
  const CDateTime eventStart = event->StartAsUTC();
  int diff;

  if (m_gridStart == eventStart)
    return 0; // block is at grid start
  else if (m_gridStart > eventStart)
    diff = -1 * (m_gridStart - eventStart).GetSecondsTotal();
  else
    diff = (eventStart - m_gridStart).GetSecondsTotal();

  // First block of a tag is always the block calculated using event's start time, rounded up.
  float fBlockIndex = diff / 60.0f / MINSPERBLOCK;
  return static_cast<int>(std::ceil(fBlockIndex));
}

int CGUIEPGGridContainerModel::GetLastEventBlock(const std::shared_ptr<CPVREpgInfoTag>& event) const
{
  // Last block of a tag is always the block calculated using event's end time, not rounded up.
  return GetBlock(event->EndAsUTC());
}

bool CGUIEPGGridContainerModel::IsEventMemberOfBlock(const std::shared_ptr<CPVREpgInfoTag>& event,
                                                     int iBlock) const
{
  const int iFirstBlock = GetFirstEventBlock(event);
  const int iLastBlock = GetLastEventBlock(event);

  if (iFirstBlock > iLastBlock)
  {
    return false;
  }
  else if (iFirstBlock == iBlock)
  {
    return true;
  }
  else if (iFirstBlock < iBlock)
  {
    return (iBlock <= iLastBlock);
  }
  return false;
}

std::unique_ptr<CFileItemList> CGUIEPGGridContainerModel::GetCurrentTimeLineItems(
    int firstChannel, int numChannels) const
{
  // Note: No need to keep this in a member. Gets generally not called multiple times for the
  //       same timeline, but content must be synced with m_epgItems, which changes quite often.

  std::unique_ptr<CFileItemList> items(new CFileItemList);

  if (numChannels > ChannelItemsSize())
    numChannels = ChannelItemsSize();

  int i = 0;
  for (int channel = firstChannel; channel < (firstChannel + numChannels); ++channel)
  {
    // m_epgItems is not sorted, fileitemlist must be sorted, so we have to 'find' the channel
    const auto itEpg = m_epgItems.find(channel);
    if (itEpg != m_epgItems.end())
    {
      // tags are sorted, so we can iterate and append
      for (const auto& tag : (*itEpg).second.tags)
      {
        tag->SetProperty("TimelineIndex", i);
        items->Add(tag);
        ++i;
      }
    }
    else
    {
      // fake empty EPG
      const std::shared_ptr<CFileItem> tag = CreateGapItem(channel);
      tag->SetProperty("TimelineIndex", i);
      items->Add(tag);
      ++i;
    }
  }
  return items;
}
