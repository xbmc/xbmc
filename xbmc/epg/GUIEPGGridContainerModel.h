#pragma once
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

#include <memory>
#include <vector>

#include "XBDateTime.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

class CFileItemList;

namespace EPG
{
  struct GridItem
  {
    CFileItemPtr item;
    float originWidth;
    float width;
    int progIndex;

    GridItem() : originWidth(0.0f), width(0.0f), progIndex(-1) {}
  };

  class CGUIEPGGridContainerModel
  {
  public:
    static const int MINSPERBLOCK       = 5; // minutes
    static const int MAXBLOCKS          = 33 * 24 * 60 / MINSPERBLOCK; //! 33 days of 5 minute blocks (31 days for upcoming data + 1 day for past data + 1 day for fillers)
    static const int GRID_START_PADDING = 30; // minutes; latest grid start 'now - GRID_START_PADDING', will be adjusted to this value if shall be set to later

    CGUIEPGGridContainerModel() : m_blocks(0) {}
    virtual ~CGUIEPGGridContainerModel() { Reset(); }

    void Refresh(const std::unique_ptr<CFileItemList> &items, const CDateTime &gridStart, const CDateTime &gridEnd, int iRulerUnit, int iBlocksPerPage, float fBlockSize);
    void SetInvalid();

    void FindChannelAndBlockIndex(int channelUid, unsigned int broadcastUid, int eventOffset, int &newChannelIndex, int &newBlockIndex) const;

    void FreeChannelMemory(int keepStart, int keepEnd);
    void FreeProgrammeMemory(int channel, int keepStart, int keepEnd);
    void FreeRulerMemory(int keepStart, int keepEnd);

    CFileItemPtr GetProgrammeItem(int iIndex) const { return m_programmeItems[iIndex]; }
    bool HasProgrammeItems() const { return !m_programmeItems.empty(); }
    int ProgrammeItemsSize() const { return static_cast<int>(m_programmeItems.size()); }

    CFileItemPtr GetChannelItem(int iIndex) const { return m_channelItems[iIndex]; }
    bool HasChannelItems() const { return !m_channelItems.empty(); }
    int ChannelItemsSize() const { return static_cast<int>(m_channelItems.size()); }

    CFileItemPtr GetRulerItem(int iIndex) const { return m_rulerItems[iIndex]; }
    int RulerItemsSize() const { return static_cast<int>(m_rulerItems.size()); }

    int GetBlockCount() const { return m_blocks; }
    bool HasGridItems() const { return !m_gridIndex.empty(); }
    GridItem *GetGridItemPtr(int iChannel, int iBlock) { return &m_gridIndex[iChannel][iBlock]; }
    CFileItemPtr GetGridItem(int iChannel, int iBlock) const { return m_gridIndex[iChannel][iBlock].item; }
    float GetGridItemWidth(int iChannel, int iBlock) const { return m_gridIndex[iChannel][iBlock].width; }
    float GetGridItemOriginWidth(int iChannel, int iBlock) const { return m_gridIndex[iChannel][iBlock].originWidth; }
    int GetGridItemIndex(int iChannel, int iBlock) const { return m_gridIndex[iChannel][iBlock].progIndex; }
    void SetGridItemWidth(int iChannel, int iBlock, float fWidth) { m_gridIndex[iChannel][iBlock].width = fWidth; }

    bool IsZeroGridDuration() const { return (m_gridEnd - m_gridStart) == CDateTimeSpan(0, 0, 0, 0); }
    const CDateTime &GetGridStart() const { return m_gridStart; }
    const CDateTime &GetGridEnd() const { return m_gridEnd; }

  private:
    void FreeItemsMemory();
    void Reset();

    struct ItemsPtr
    {
      long start;
      long stop;
    };

    CDateTime m_gridStart;
    CDateTime m_gridEnd;

    std::vector<CFileItemPtr> m_programmeItems;
    std::vector<CFileItemPtr> m_channelItems;
    std::vector<CFileItemPtr> m_rulerItems;
    std::vector<ItemsPtr> m_epgItemsPtr;
    std::vector<std::vector<GridItem> > m_gridIndex;

    int m_blocks;
  };
}
