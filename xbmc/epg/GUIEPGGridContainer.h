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

#include <string>
#include <vector>

#include "XBDateTime.h"
#include "FileItem.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIListItemLayout.h"
#include "guilib/IGUIContainer.h"

namespace EPG
{
  struct GridItem;
  class CGUIEPGGridContainerModel;

  class CGUIEPGGridContainer : public IGUIContainer
  {
  public:
    CGUIEPGGridContainer(int parentID, int controlID, float posX, float posY, float width, float height,
                         int scrollTime, int preloadItems, int minutesPerPage,
                         int rulerUnit, const CTextureInfo& progressIndicatorTexture);
    CGUIEPGGridContainer(const CGUIEPGGridContainer &other);

    virtual CGUIEPGGridContainer *Clone() const { return new CGUIEPGGridContainer(*this); }

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
    virtual int GetSelectedItem() const;
    CFileItemPtr GetSelectedChannelItem() const;
    PVR::CPVRChannelPtr GetSelectedChannel();
    virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);

    virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
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
    void SetTimelineItems(const std::unique_ptr<CFileItemList> &items, const CDateTime &gridStart, const CDateTime &gridEnd);
    void SetChannel(const PVR::CPVRChannelPtr &channel);
    void SetChannel(const std::string &channel);
    void ResetCoordinates();

  protected:
    bool OnClick(int actionID);
    bool SelectItemFromPoint(const CPoint &point, bool justGrid = true);

    void SetChannel(int channel, bool bFindClosestItem = true);
    void SetBlock(int block);
    void ChannelScroll(int amount);
    void ProgrammesScroll(int amount);
    void ValidateOffset();
    void UpdateLayout();

    GridItem *GetItem(int channel);
    GridItem *GetNextItem(int channel);
    GridItem *GetPrevItem(int channel);
    GridItem *GetClosestItem(int channel);

    int GetItemSize(GridItem *item);
    int GetBlock(const CGUIListItemPtr &item, int channel);
    int GetRealBlock(const CGUIListItemPtr &item, int channel);
    void MoveToRow(int row);

    CGUIListItemLayout *GetFocusedLayout() const;

    void ScrollToBlockOffset(int offset);
    void ScrollToChannelOffset(int offset);
    void GoToBlock(int blockIndex);
    void GoToChannel(int channelIndex);
    void UpdateScrollOffset(unsigned int currentTime);
    void ProcessItem(float posX, float posY, const CFileItemPtr &item, CFileItemPtr &lastitem, bool focused, CGUIListItemLayout* normallayout, CGUIListItemLayout* focusedlayout, unsigned int currentTime, CDirtyRegionList &dirtyregions, float resize = -1.0f);
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

    void GetChannelCacheOffsets(int &cacheBefore, int &cacheAfter);
    void GetProgrammeCacheOffsets(int &cacheBefore, int &cacheAfter);

  private:
    void HandleChannels(bool bRender, unsigned int currentTime, CDirtyRegionList &dirtyregions);
    void HandleRuler(bool bRender, unsigned int currentTime, CDirtyRegionList &dirtyregions);
    void HandleProgrammeGrid(bool bRender, unsigned int currentTime, CDirtyRegionList &dirtyregions);

    void UpdateItems();

    EPG::CEpgInfoTagPtr GetSelectedEpgInfoTag() const;

    int m_rulerUnit; //! number of blocks that makes up one element of the ruler
    int m_channelsPerPage;
    int m_programmesPerPage;
    int m_channelCursor;
    int m_channelOffset;
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

    CGUITexture m_guiProgressIndicatorTexture;

    CFileItemPtr m_lastItem;
    CFileItemPtr m_lastChannel;

    int m_scrollTime;

    int m_programmeScrollLastTime;
    float m_programmeScrollSpeed;
    float m_programmeScrollOffset;

    int m_channelScrollLastTime;
    float m_channelScrollSpeed;
    float m_channelScrollOffset;

    CCriticalSection m_critSection;
    std::unique_ptr<CGUIEPGGridContainerModel> m_gridModel;
    std::unique_ptr<CGUIEPGGridContainerModel> m_updatedGridModel;
    std::unique_ptr<CGUIEPGGridContainerModel> m_outdatedGridModel;

    GridItem *m_item;
  };
}
