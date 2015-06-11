#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "XBDateTime.h"
#include "FileItem.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIListItemLayout.h"
#include "guilib/IGUIContainer.h"

namespace EPG
{
  #define MAXCHANNELS 20
  #define MAXBLOCKS   (33 * 24 * 60 / 5) //! 33 days of 5 minute blocks (31 days for upcoming data + 1 day for past data + 1 day for fillers)

  struct GridItemsPtr
  {
    CGUIListItemPtr item;
    float originWidth;
    float originHeight;
    float width;
    float height;
  };

  class CGUIEPGGridContainer : public IGUIContainer
  {
  public:
    CGUIEPGGridContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                         int scrollTime, int preloadItems, int minutesPerPage,
                         int rulerUnit, const CTextureInfo& progressIndicatorTexture);
    virtual ~CGUIEPGGridContainer(void);
    virtual CGUIEPGGridContainer *Clone() const { return new CGUIEPGGridContainer(*this); };

    virtual bool OnAction(const CAction &action);
    virtual void OnDown();
    virtual void OnUp();
    virtual void OnLeft();
    virtual void OnRight();
    virtual bool OnMouseOver(const CPoint &point);
    virtual bool OnMouseClick(int dwButton, const CPoint &point);
    virtual bool OnMouseDoubleClick(int dwButton, const CPoint &point);
    virtual bool OnMouseWheel(char wheel, const CPoint &point);
    virtual bool OnMessage(CGUIMessage& message);
    virtual void SetFocus(bool focus);

    virtual std::string GetDescription() const;
    const int GetNumChannels()   { return m_channels; };
    virtual int GetSelectedItem() const;
    const int GetSelectedChannel() { return m_channelCursor + m_channelOffset; }
    void SetSelectedChannel(int channelIndex);
    PVR::CPVRChannelPtr GetChannel(int iIndex);
    virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);

    virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
    virtual void DoRender();
    virtual void Render();
    void LoadLayout(TiXmlElement *layout);
    void LoadContent(TiXmlElement *content);

    virtual CGUIListItemPtr GetListItem(int offset, unsigned int flag = 0) const;
    virtual std::string GetLabel(int info) const;

    /*! \brief Set the offset of the first item in the container from the container's position
     Useful for lists/panels where the focused item may be larger than the non-focused items and thus
     normally cut off from the clipping window defined by the container's position + size.
     \param offset CPoint holding the offset in skin coordinates.
     */
    void SetRenderOffset(const CPoint &offset);

    void GoToBegin();
    void GoToEnd();
    void GoToNow();
    void SetStartEnd(CDateTime start, CDateTime end);
    void SetChannel(const PVR::CPVRChannelPtr &channel);
    void SetChannel(const std::string &channel);

  protected:
    bool OnClick(int actionID);
    bool SelectItemFromPoint(const CPoint &point, bool justGrid = true);

    void UpdateItems();

    void SetChannel(int channel);
    void SetBlock(int block);
    void ChannelScroll(int amount);
    void ProgrammesScroll(int amount);
    void ValidateOffset();
    void UpdateLayout();
    void Reset();
    void ClearGridIndex(void);

    GridItemsPtr *GetItem(const int &channel);
    GridItemsPtr *GetNextItem(const int &channel);
    GridItemsPtr *GetPrevItem(const int &channel);
    GridItemsPtr *GetClosestItem(const int &channel);

    int GetItemSize(GridItemsPtr *item);
    int GetBlock(const CGUIListItemPtr &item, const int &channel);
    int GetRealBlock(const CGUIListItemPtr &item, const int &channel);
    void MoveToRow(int row);

    CGUIListItemLayout *GetFocusedLayout() const;

    void ScrollToBlockOffset(int offset);
    void ScrollToChannelOffset(int offset);
    void UpdateScrollOffset(unsigned int currentTime);
    void ProcessItem(float posX, float posY, CGUIListItem *item, CGUIListItem *&lastitem, bool focused, CGUIListItemLayout* normallayout, CGUIListItemLayout* focusedlayout, unsigned int currentTime, CDirtyRegionList &dirtyregions, float resize = -1.0f);
    void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
    void GetCurrentLayouts();

    void ProcessChannels(unsigned int currentTime, CDirtyRegionList &dirtyregions);
    void ProcessRuler(unsigned int currentTime, CDirtyRegionList &dirtyregions);
    void ProcessProgrammeGrid(unsigned int currentTime, CDirtyRegionList &dirtyregions);
    void ProcessProgressIndicator(unsigned int currentTime, CDirtyRegionList &dirtyregions);
    void RenderChannels();
    void RenderRuler();
    void RenderProgrammeGrid();
    void RenderProgressIndicator();

    CPoint m_renderOffset; ///< \brief render offset of the first item in the list \sa SetRenderOffset

    struct ItemsPtr
    {
      long start;
      long stop;
    };
    std::vector<ItemsPtr> m_epgItemsPtr;
    std::vector<CGUIListItemPtr> m_channelItems;
    std::vector<CGUIListItemPtr> m_rulerItems;
    std::vector<CGUIListItemPtr> m_programmeItems;
    std::vector<CGUIListItemLayout> m_channelLayouts;
    std::vector<CGUIListItemLayout> m_focusedChannelLayouts;
    std::vector<CGUIListItemLayout> m_focusedProgrammeLayouts;
    std::vector<CGUIListItemLayout> m_programmeLayouts;
    std::vector<CGUIListItemLayout> m_rulerLayouts;

    CGUIListItemLayout *m_channelLayout;
    CGUIListItemLayout *m_focusedChannelLayout;
    CGUIListItemLayout *m_programmeLayout;
    CGUIListItemLayout *m_focusedProgrammeLayout;
    CGUIListItemLayout *m_rulerLayout;

    bool m_wasReset;  // true if we've received a Reset message until we've rendered once.  Allows
                      // us to make sure we don't tell the infomanager that we've been moving when
                      // the "movement" was simply due to the list being repopulated (thus cursor position
                      // changing around)

    void FreeItemsMemory();
    void FreeChannelMemory(int keepStart, int keepEnd);
    void FreeProgrammeMemory(int channel, int keepStart, int keepEnd);
    void FreeRulerMemory(int keepStart, int keepEnd);

    void GetChannelCacheOffsets(int &cacheBefore, int &cacheAfter);
    void GetProgrammeCacheOffsets(int &cacheBefore, int &cacheAfter);

  private:
    int m_rulerUnit; //! number of blocks that makes up one element of the ruler
    int m_channels;
    int m_channelsPerPage;
    int m_programmesPerPage;
    int m_channelCursor;
    int m_channelOffset;
    int m_blocks;
    int m_blocksPerPage;
    int m_blockCursor;
    int m_blockOffset;
    int m_cacheChannelItems;
    int m_cacheProgrammeItems;
    int m_cacheRulerItems;

    float m_rulerPosX;      //! X position of first ruler item
    float m_rulerPosY;      //! Y position of first ruler item
    float m_rulerHeight;    //! height of the scrolling timeline above the ruler items
    float m_rulerWidth;     //! width of each element of the ruler
    float m_channelPosX;    //! X position of first channel row
    float m_channelPosY;    //! Y position of first channel row
    float m_channelHeight;  //! height of the channel item
    float m_channelWidth;   //! width of the channel item
    float m_gridPosX;       //! X position of first grid item
    float m_gridPosY;       //! Y position of first grid item
    float m_gridWidth;      //! width of the epg grid control
    float m_gridHeight;     //! height of the epg grid control
    float m_blockSize;      //! a block's width in pixels
    float m_analogScrollCount;

    CDateTime m_gridStart;
    CDateTime m_gridEnd;

    CGUITexture m_guiProgressIndicatorTexture;

    std::vector<std::vector<GridItemsPtr> > m_gridIndex;
    GridItemsPtr *m_item;
    CGUIListItem *m_lastItem;
    CGUIListItem *m_lastChannel;

    int m_scrollTime;

    int m_programmeScrollLastTime;
    float m_programmeScrollSpeed;
    float m_programmeScrollOffset;

    int m_channelScrollLastTime;
    float m_channelScrollSpeed;
    float m_channelScrollOffset;
  };
}
