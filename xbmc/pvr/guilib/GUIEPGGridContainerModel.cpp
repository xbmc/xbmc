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
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgInfoTag.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace PVR;

static const unsigned int GRID_START_PADDING = 30; // minutes

void CGUIEPGGridContainerModel::SetInvalid()
{
  for (const auto& programme : m_programmeItems)
    programme->SetInvalid();
  for (const auto& channel : m_channelItems)
    channel->SetInvalid();
  for (const auto& ruler : m_rulerItems)
    ruler->SetInvalid();
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::CreateGapItem(int iChannel) const
{
  const std::shared_ptr<CPVRChannel> channel = m_channelItems[iChannel]->GetPVRChannelInfoTag();

  std::shared_ptr<CPVREpgInfoTag> gapTag;
  const std::shared_ptr<CPVREpg> epg = channel->GetEPG();
  if (epg)
    gapTag = std::make_shared<CPVREpgInfoTag>(epg->GetChannelData(), epg->EpgID());
  else
    gapTag = std::make_shared<CPVREpgInfoTag>(std::make_shared<CPVREpgChannelData>(*channel), -1);

  return std::make_shared<CFileItem>(gapTag);
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::GetGapItem(int iChannel,
                                                                 int iStartBlock) const
{
  std::shared_ptr<CFileItem> item;

  auto gapsIt = m_gapItems.find({iChannel, iStartBlock});
  if (gapsIt != m_gapItems.end())
  {
    item = (*gapsIt).second;
  }
  else
  {
    item = CreateGapItem(iChannel);
    m_gapItems.insert({{iChannel, iStartBlock}, item});
  }

  return item;
}

void CGUIEPGGridContainerModel::Initialize(const std::unique_ptr<CFileItemList>& items, const CDateTime& gridStart, const CDateTime& gridEnd, int iRulerUnit, int iBlocksPerPage, float fBlockSize)
{
  if (!m_channelItems.empty())
  {
    CLog::LogF(LOGERROR, "Already initialized!");
    return;
  }

  m_fBlockSize = fBlockSize;

  ////////////////////////////////////////////////////////////////////////
  // Create programme & channel items
  m_programmeItems.reserve(items->Size());
  CFileItemPtr fileItem;
  int iLastChannelUID = -1;
  int iLastClientUID = -1;
  ItemsPtr itemsPointer;
  itemsPointer.start = 0;
  int j = 0;
  for (int i = 0; i < items->Size(); ++i)
  {
    fileItem = items->Get(i);
    if (!fileItem->HasEPGInfoTag())
      continue;

    m_programmeItems.emplace_back(fileItem);

    int iCurrentChannelUID = fileItem->GetEPGInfoTag()->UniqueChannelID();
    int iCurrentClientUID = fileItem->GetEPGInfoTag()->ClientID();
    if (iCurrentChannelUID != iLastChannelUID || iCurrentClientUID != iLastClientUID)
    {
      iLastChannelUID = iCurrentChannelUID;
      iLastClientUID = iCurrentClientUID;

      const std::shared_ptr<CPVRChannel> channel = CServiceBroker::GetPVRManager().ChannelGroups()->GetChannelForEpgTag(fileItem->GetEPGInfoTag());
      if (!channel)
        continue;

      if (j > 0)
      {
        itemsPointer.stop = j - 1;
        m_epgItemsPtr.emplace_back(itemsPointer);
        itemsPointer.start = j;
      }
      m_channelItems.emplace_back(CFileItemPtr(new CFileItem(channel)));
    }
    ++j;
  }
  if (!m_programmeItems.empty())
  {
    itemsPointer.stop = m_programmeItems.size() - 1;
    m_epgItemsPtr.emplace_back(itemsPointer);
  }

  /* check for invalid start and end time */
  if (gridStart >= gridEnd)
  {
    // default to start "now minus GRID_START_PADDING minutes" and end "start plus one page".
    m_gridStart = CDateTime::GetUTCDateTime() - CDateTimeSpan(0, 0, GetGridStartPadding(), 0);
    m_gridEnd = m_gridStart + CDateTimeSpan(0, 0, iBlocksPerPage * MINSPERBLOCK, 0);
  }
  else if (gridStart > (CDateTime::GetUTCDateTime() - CDateTimeSpan(0, 0, GetGridStartPadding(), 0)))
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
  m_gridStart = CDateTime(m_gridStart.GetYear(), m_gridStart.GetMonth(), m_gridStart.GetDay(), m_gridStart.GetHour(), m_gridStart.GetMinute() >= 30 ? 30 : 0, 0);
  m_gridEnd = CDateTime(m_gridEnd.GetYear(), m_gridEnd.GetMonth(), m_gridEnd.GetDay(), m_gridEnd.GetHour(), m_gridEnd.GetMinute() >= 30 ? 30 : 0, 0);

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
    rulerItem.reset(new CFileItem(ruler.GetAsLocalizedTime("", false)));
    rulerItem->SetLabel2(ruler.GetAsLocalizedDate(true));
    m_rulerItems.emplace_back(rulerItem);
  }

  FreeItemsMemory();

  ////////////////////////////////////////////////////////////////////////
  // Create empty epg grid (will be filled on demand)
  const CDateTimeSpan blockDuration(0, 0, MINSPERBLOCK, 0);
  const CDateTimeSpan gridDuration(m_gridEnd - m_gridStart);
  m_blocks = (gridDuration.GetDays() * 24 * 60 + gridDuration.GetHours() * 60 + gridDuration.GetMinutes()) / MINSPERBLOCK;
  if (m_blocks >= MAXBLOCKS)
    m_blocks = MAXBLOCKS;
  else if (m_blocks < iBlocksPerPage)
    m_blocks = iBlocksPerPage;

  m_gridIndex.reserve(m_channelItems.size());
  const std::vector<GridItem> blocks(m_blocks);

  for (size_t channel = 0; channel < m_channelItems.size(); ++channel)
  {
    m_gridIndex.emplace_back(blocks);
  }
}

void CGUIEPGGridContainerModel::FindChannelAndBlockIndex(int channelUid, unsigned int broadcastUid, int eventOffset, int& newChannelIndex, int& newBlockIndex) const
{
  const CDateTimeSpan blockDuration(0, 0, MINSPERBLOCK, 0);

  newChannelIndex = INVALID_INDEX;
  newBlockIndex = INVALID_INDEX;

  // find the channel
  int iCurrentChannel = 0;
  for (const auto& channel : m_channelItems)
  {
    if (channel->GetPVRChannelInfoTag()->UniqueID() == channelUid)
    {
      newChannelIndex = iCurrentChannel;
      break;
    }
    iCurrentChannel++;
  }

  if (newChannelIndex != INVALID_INDEX)
  {
    // find the block
    CDateTime gridCursor(m_gridStart); //reset cursor for new channel
    unsigned long progIdx = m_epgItemsPtr[newChannelIndex].start;
    unsigned long lastIdx = m_epgItemsPtr[newChannelIndex].stop;
    int iEpgId = m_programmeItems[progIdx]->GetEPGInfoTag()->EpgID();
    std::shared_ptr<CPVREpgInfoTag> tag;
    for (int block = 0; block < m_blocks; ++block)
    {
      while (progIdx <= lastIdx)
      {
        tag = m_programmeItems[progIdx]->GetEPGInfoTag();

        if (tag->EpgID() != iEpgId || gridCursor < tag->StartAsUTC() || m_gridEnd <= tag->StartAsUTC())
          break; // next block

        if (gridCursor < tag->EndAsUTC())
        {
          if (broadcastUid > 0 && tag->UniqueBroadcastID() == broadcastUid)
          {
            newBlockIndex = block + eventOffset;
            return; // done.
          }
          break; // next block
        }
        progIdx++;
      }
      gridCursor += blockDuration;
    }
  }
}

GridItem* CGUIEPGGridContainerModel::GetGridItemPtr(int iChannel, int iBlock) const
{
  if (!m_gridIndex[iChannel][iBlock].item)
  {
    bool bFound = false;

    std::shared_ptr<CFileItem> item;
    int startBlock = 0;
    int endBlock = 0;

    const CDateTime start = GetStartTimeForBlock(iBlock);
    std::shared_ptr<CPVREpgInfoTag> epgTag;

    int progIndex = m_epgItemsPtr[iChannel].start;
    for (; progIndex < m_epgItemsPtr[iChannel].stop; ++progIndex)
    {
      item = m_programmeItems[progIndex];
      epgTag = item->GetEPGInfoTag();

      if (start < epgTag->StartAsUTC() || m_gridEnd <= epgTag->StartAsUTC())
        break;

      if (start < epgTag->EndAsUTC())
      {
        startBlock = GetFirstEventBlock(epgTag);
        endBlock = GetLastEventBlock(epgTag);

        item->SetProperty("GenreType", epgTag->GenreType());

        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Insert a gap tag.
      if (epgTag)
      {
        if (progIndex == m_epgItemsPtr[iChannel].start)
        {
          // gap before first event
          startBlock = 0;
          endBlock = GetFirstEventBlock(epgTag) - 1;
        }
        else if (progIndex == m_epgItemsPtr[iChannel].stop)
        {
          // gap after last event
          startBlock = GetLastEventBlock(epgTag) + 1;
          endBlock = m_blocks - 1;
        }
        else
        {
          // gap between two events
          startBlock = GetLastEventBlock(m_programmeItems[progIndex - 1]->GetEPGInfoTag()) + 1;
          endBlock = GetFirstEventBlock(epgTag) - 1;
        }
      }
      else
      {
        // channel without epg
        startBlock = 0;
        endBlock = m_blocks - 1;
      }

      progIndex = -1;
      item = GetGapItem(iChannel, startBlock);
    }

    const float fItemWidth = (endBlock - startBlock + 1) * m_fBlockSize;
    m_gridIndex[iChannel][iBlock].originWidth = fItemWidth;
    m_gridIndex[iChannel][iBlock].width = fItemWidth;
    m_gridIndex[iChannel][iBlock].item = item;
    m_gridIndex[iChannel][iBlock].progIndex = progIndex;
    m_gridIndex[iChannel][iBlock].startBlock = startBlock;
  }
  return &m_gridIndex[iChannel][iBlock];
}

std::shared_ptr<CFileItem> CGUIEPGGridContainerModel::GetGridItem(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->item;
}

int CGUIEPGGridContainerModel::GetGridItemStartBlock(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->startBlock;
}

float CGUIEPGGridContainerModel::GetGridItemWidth(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->width;
}

float CGUIEPGGridContainerModel::GetGridItemOriginWidth(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->originWidth;
}

int CGUIEPGGridContainerModel::GetGridItemIndex(int iChannel, int iBlock) const
{
  return GetGridItemPtr(iChannel, iBlock)->progIndex;
}

void CGUIEPGGridContainerModel::SetGridItemWidth(int iChannel, int iBlock, float fWidth)
{
  if (m_gridIndex[iChannel][iBlock].width != fWidth)
    GetGridItemPtr(iChannel, iBlock)->width = fWidth;
}

unsigned int CGUIEPGGridContainerModel::GetGridStartPadding() const
{
  unsigned int iPastMinutes = CServiceBroker::GetPVRManager().EpgContainer().GetPastDaysToDisplay() * 24 * 60;

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

void CGUIEPGGridContainerModel::FreeProgrammeMemory(int channel, int keepStart, int keepEnd)
{
  if (keepStart < keepEnd)
  {
    // remove before keepStart and after keepEnd
    if (keepStart > 0 && keepStart < m_blocks)
    {
      // if item exist and block is not part of visible item
      std::shared_ptr<CGUIListItem> last = m_gridIndex[channel][keepStart].item;
      for (int i = keepStart - 1; i > 0; --i)
      {
        const std::shared_ptr<CGUIListItem> current = m_gridIndex[channel][i].item;
        if (current && current != last)
        {
          current->FreeMemory();
          // FreeMemory() is smart enough to not cause any problems when called multiple times on same item
          // but we can make use of condition needed to not call FreeMemory() on item that is partially visible
          // to avoid calling FreeMemory() multiple times on item that occupy few blocks in a row
          last = current;
        }
      }
    }

    if (keepEnd > 0 && keepEnd < m_blocks)
    {
      std::shared_ptr<CGUIListItem> last = m_gridIndex[channel][keepEnd].item;
      for (int i = keepEnd + 1; i < m_blocks; ++i)
      {
        // if item exist and block is not part of visible item
        std::shared_ptr<CGUIListItem> current = m_gridIndex[channel][i].item;
        if (current && current != last)
        {
          current->FreeMemory();
          // FreeMemory() is smart enough to not cause any problems when called multiple times on same item
          // but we can make use of condition needed to not call FreeMemory() on item that is partially visible
          // to avoid calling FreeMemory() multiple times on item that occupy few blocks in a row
          last = current;
        }
      }
    }
  }
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

void CGUIEPGGridContainerModel::FreeItemsMemory()
{
  for (const auto& programme : m_programmeItems)
    programme->FreeMemory();
  for (const auto& channel : m_channelItems)
    channel->FreeMemory();
  for (const auto& ruler : m_rulerItems)
    ruler->FreeMemory();
}

unsigned int CGUIEPGGridContainerModel::GetPageNowOffset() const
{
  return GetGridStartPadding() / MINSPERBLOCK; // this is the 'now' block relative to page start
}

CDateTime CGUIEPGGridContainerModel::GetStartTimeForBlock(int block) const
{
  if (block < 0)
    block = 0;
  else if (block >= m_blocks)
    block = m_blocks - 1;

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
  //       are unambigious. Example: An event ending at 5:00:00 shall be mapped to block 9 and
  //       an event starting at 5:00:00 shall be mapped to block 10, not both at block 10.
  return (diff - 1) / 60 / MINSPERBLOCK;
}

int CGUIEPGGridContainerModel::GetNowBlock() const
{
  return GetBlock(CDateTime::GetUTCDateTime()) - GetPageNowOffset();
}

int CGUIEPGGridContainerModel::GetFirstEventBlock(const std::shared_ptr<CPVREpgInfoTag>& event) const
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
  // Refer to CGUIEPGGridContainerModel::Refresh, where the model is created, for details!
  float fBlockIndex = diff / 60.0f / MINSPERBLOCK;
  return std::ceil(fBlockIndex);
}

int CGUIEPGGridContainerModel::GetLastEventBlock(const std::shared_ptr<CPVREpgInfoTag>& event) const
{
  // Last block of a tag is always the block calculated using event's end time, not rounded up.
  // Refer to CGUIEPGGridContainerModel::Refresh, where the model is created, for details!
  return GetBlock(event->EndAsUTC());
}
