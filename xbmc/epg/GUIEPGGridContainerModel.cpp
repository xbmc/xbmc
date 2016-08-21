/*
 *      Copyright (C) 2012-2016 Team Kodi
 *      http://xbmc.org
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
#include "utils/Variant.h"
#include "pvr/channels/PVRChannel.h"

#include "GUIEPGGridContainerModel.h"

class CGUIListItem;
typedef std::shared_ptr<CGUIListItem> CGUIListItemPtr;

using namespace EPG;
using namespace PVR;

void CGUIEPGGridContainerModel::SetInvalid()
{
  for (const auto &programme : m_programmeItems)
    programme->SetInvalid();
  for (const auto &channel : m_channelItems)
    channel->SetInvalid();
  for (const auto &ruler : m_rulerItems)
    ruler->SetInvalid();
}

void CGUIEPGGridContainerModel::Reset()
{
  for (auto &channel : m_gridIndex)
  {
    for (const auto &block : channel)
    {
      if (block.item)
        block.item->ClearProperties();
    }
    channel.clear();
  }
  m_gridIndex.clear();

  m_channelItems.clear();
  m_programmeItems.clear();
  m_rulerItems.clear();
  m_epgItemsPtr.clear();
}

void CGUIEPGGridContainerModel::Refresh(const std::unique_ptr<CFileItemList> &items, const CDateTime &gridStart, const CDateTime &gridEnd, int iRulerUnit, int iBlocksPerPage, float fBlockSize)
{
  Reset();

  ////////////////////////////////////////////////////////////////////////
  // Create programme & channel items
  m_programmeItems.reserve(items->Size());
  CFileItemPtr fileItem;
  int iLastChannelID = -1;
  ItemsPtr itemsPointer;
  itemsPointer.start = 0;
  CPVRChannelPtr channel;
  int j = 0;
  for (int i = 0; i < items->Size(); ++i)
  {
    fileItem = items->Get(i);
    if (!fileItem->HasEPGInfoTag() || !fileItem->GetEPGInfoTag()->HasPVRChannel())
      continue;

    m_programmeItems.emplace_back(fileItem);

    channel = fileItem->GetEPGInfoTag()->ChannelTag();
    if (!channel)
      continue;

    int iCurrentChannelID = channel->ChannelID();
    if (iCurrentChannelID != iLastChannelID)
    {
      if (j > 0)
      {
        itemsPointer.stop = j - 1;
        m_epgItemsPtr.emplace_back(itemsPointer);
        itemsPointer.start = j;
      }
      iLastChannelID = iCurrentChannelID;
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
    m_gridStart = CDateTime::GetCurrentDateTime().GetAsUTCDateTime() - CDateTimeSpan(0, 0, GRID_START_PADDING, 0);
    m_gridEnd = m_gridStart + CDateTimeSpan(0, 0, iBlocksPerPage * MINSPERBLOCK, 0);
  }
  else if (gridStart > (CDateTime::GetCurrentDateTime().GetAsUTCDateTime() - CDateTimeSpan(0, 0, GRID_START_PADDING, 0)))
  {
    // adjust to start "now minus GRID_START_PADDING minutes".
    m_gridStart = CDateTime::GetCurrentDateTime().GetAsUTCDateTime() - CDateTimeSpan(0, 0, GRID_START_PADDING, 0);
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
  // Create epg grid
  const CDateTimeSpan blockDuration(0, 0, MINSPERBLOCK, 0);
  const CDateTimeSpan gridDuration(m_gridEnd - m_gridStart);
  m_blocks = (gridDuration.GetDays() * 24 * 60 + gridDuration.GetHours() * 60 + gridDuration.GetMinutes()) / MINSPERBLOCK;
  if (m_blocks >= MAXBLOCKS)
    m_blocks = MAXBLOCKS;

  m_gridIndex.reserve(m_channelItems.size());
  const std::vector<GridItem> blocks(m_blocks);

  for (size_t channel = 0; channel < m_channelItems.size(); ++channel)
  {
    m_gridIndex.emplace_back(blocks);

    CDateTime gridCursor(m_gridStart); //reset cursor for new channel
    unsigned long progIdx = m_epgItemsPtr[channel].start;
    unsigned long lastIdx = m_epgItemsPtr[channel].stop;
    int iEpgId            = m_programmeItems[progIdx]->GetEPGInfoTag()->EpgID();
    int itemSize          = 1; // size of the programme in blocks
    int savedBlock        = 0;
    CFileItemPtr item;
    CEpgInfoTagPtr tag;

    for (int block = 0; block < m_blocks; ++block)
    {
      while (progIdx <= lastIdx)
      {
        item = m_programmeItems[progIdx];
        tag = item->GetEPGInfoTag();

        if (tag->EpgID() != iEpgId || gridCursor < tag->StartAsUTC() || m_gridEnd <= tag->StartAsUTC())
          break;

        if (gridCursor < tag->EndAsUTC())
        {
          m_gridIndex[channel][block].item = item;
          m_gridIndex[channel][block].progIndex = progIdx;
          break;
        }

        progIdx++;
      }

      gridCursor += blockDuration;

      if (block == 0)
        continue;

      const CFileItemPtr prevItem(m_gridIndex[channel][block - 1].item);
      const CFileItemPtr currItem(m_gridIndex[channel][block].item);

      if (block == m_blocks - 1 || prevItem != currItem)
      {
        // special handling for last block.
        int blockDelta = -1;
        int sizeDelta = 0;
        if (block == m_blocks - 1 && prevItem == currItem)
        {
          itemSize++;
          blockDelta = 0;
          sizeDelta = 1;
        }

        if (prevItem)
        {
          m_gridIndex[channel][savedBlock].item->SetProperty("GenreType", prevItem->GetEPGInfoTag()->GenreType());
        }
        else
        {
          CEpgInfoTagPtr gapTag(CEpgInfoTag::CreateDefaultTag());
          gapTag->SetPVRChannel(m_channelItems[channel]->GetPVRChannelInfoTag());
          CFileItemPtr gapItem(new CFileItem(gapTag));
          for (int i = block + blockDelta; i >= block - itemSize + sizeDelta; --i)
          {
            m_gridIndex[channel][i].item = gapItem;
          }
        }

        float fItemWidth = itemSize * fBlockSize;
        m_gridIndex[channel][savedBlock].originWidth = fItemWidth;
        m_gridIndex[channel][savedBlock].width = fItemWidth;

        itemSize = 1;
        savedBlock = block;

        // special handling for last block.
        if (block == m_blocks - 1 && prevItem != currItem)
        {
          if (currItem)
          {
            m_gridIndex[channel][savedBlock].item->SetProperty("GenreType", currItem->GetEPGInfoTag()->GenreType());
          }
          else
          {
            CEpgInfoTagPtr gapTag(CEpgInfoTag::CreateDefaultTag());
            gapTag->SetPVRChannel(m_channelItems[channel]->GetPVRChannelInfoTag());
            CFileItemPtr gapItem(new CFileItem(gapTag));
            m_gridIndex[channel][block].item = gapItem;
          }

          m_gridIndex[channel][savedBlock].originWidth = fBlockSize; // size always 1 block here
          m_gridIndex[channel][savedBlock].width = fBlockSize;
        }
      }
      else
      {
        itemSize++;
      }
    }
  }
}

void CGUIEPGGridContainerModel::FindChannelAndBlockIndex(int channelUid, unsigned int broadcastUid, int eventOffset, int &newChannelIndex, int &newBlockIndex) const
{
  const CDateTimeSpan blockDuration(0, 0, MINSPERBLOCK, 0);
  bool bFoundPrevChannel = false;

  for (size_t channel = 0; channel < m_channelItems.size(); ++channel)
  {
    CDateTime gridCursor(m_gridStart); //reset cursor for new channel
    unsigned long progIdx = m_epgItemsPtr[channel].start;
    unsigned long lastIdx = m_epgItemsPtr[channel].stop;
    int iEpgId = m_programmeItems[progIdx]->GetEPGInfoTag()->EpgID();
    CEpgInfoTagPtr tag;
    CPVRChannelPtr chan;

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
            newChannelIndex = channel;
            newBlockIndex   = block + eventOffset;
            return; // both found. done.
          }
          if (!bFoundPrevChannel && channelUid > -1)
          {
            chan = tag->ChannelTag();
            if (chan && chan->UniqueID() == channelUid)
            {
              newChannelIndex = channel;
              bFoundPrevChannel = true;
            }
          }
          break; // next block
        }
        progIdx++;
      }
      gridCursor += blockDuration;
    }
  }
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
      CGUIListItemPtr last(m_gridIndex[channel][keepStart].item);
      for (int i = keepStart - 1; i > 0; --i)
      {
        if (m_gridIndex[channel][i].item && m_gridIndex[channel][i].item != last)
        {
          m_gridIndex[channel][i].item->FreeMemory();
          // FreeMemory() is smart enough to not cause any problems when called multiple times on same item
          // but we can make use of condition needed to not call FreeMemory() on item that is partially visible
          // to avoid calling FreeMemory() multiple times on item that ocupy few blocks in a row
          last = m_gridIndex[channel][i].item;
        }
      }
    }

    if (keepEnd > 0 && keepEnd < m_blocks)
    {
      CGUIListItemPtr last(m_gridIndex[channel][keepEnd].item);
      for (int i = keepEnd + 1; i < m_blocks; ++i)
      {
        // if item exist and block is not part of visible item
        if (m_gridIndex[channel][i].item && m_gridIndex[channel][i].item != last)
        {
          m_gridIndex[channel][i].item->FreeMemory();
          // FreeMemory() is smart enough to not cause any problems when called multiple times on same item
          // but we can make use of condition needed to not call FreeMemory() on item that is partially visible
          // to avoid calling FreeMemory() multiple times on item that ocupy few blocks in a row
          last = m_gridIndex[channel][i].item;
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
  for (const auto &programme : m_programmeItems)
    programme->FreeMemory();
  for (const auto &channel : m_channelItems)
    channel->FreeMemory();
  for (const auto &ruler : m_rulerItems)
    ruler->FreeMemory();
}
