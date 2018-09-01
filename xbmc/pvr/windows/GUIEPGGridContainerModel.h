/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

#include "XBDateTime.h"

#include "pvr/PVRTypes.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

class CFileItemList;

namespace PVR
{
  struct GridItem
  {
    CFileItemPtr item;
    float originWidth = 0.0f;
    float width = 0.0f;
    int progIndex = -1;
  };

  class CGUIEPGGridContainerModel
  {
  public:
    static const int MINSPERBLOCK = 5; // minutes
    static const int MAXBLOCKS = 33 * 24 * 60 / MINSPERBLOCK; //! 33 days of 5 minute blocks (31 days for upcoming data + 1 day for past data + 1 day for fillers)

    CGUIEPGGridContainerModel() = default;
    virtual ~CGUIEPGGridContainerModel() = default;

    void Initialize(const std::unique_ptr<CFileItemList> &items, const CDateTime &gridStart, const CDateTime &gridEnd, int iRulerUnit, int iBlocksPerPage, float fBlockSize);
    void SetInvalid();

    static const int INVALID_INDEX = -1;
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
    unsigned int GetGridStartPadding() const;

    unsigned int GetPageNowOffset() const;
    int GetNowBlock() const;

    CDateTime GetStartTimeForBlock(int block) const;
    int GetBlock(const CDateTime &datetime) const;
    int GetFirstEventBlock(const CPVREpgInfoTagPtr &event) const;
    int GetLastEventBlock(const CPVREpgInfoTagPtr &event) const;

  private:
    void FreeItemsMemory();

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

    int m_blocks = 0;
  };
}
