#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIControl.h"
#include "GUIListItemLayout.h"
#include "TVDatabase.h"

#define MAXCHANNELS 200
#define MAXBLOCKS   576 // 2 days of 5 minute blocks

class CGUIEPGGridContainer : public CGUIControl
{
public:
  CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, 
                       float width, float height, int scrollTime, int minutesPerPage,
                       int rulerUnit);
  virtual ~CGUIEPGGridContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();

  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetFocus(bool bOnOff);

  void UpdateItems(EPGGrid &gridData, const CDateTime &dataStart, const CDateTime &dataEnd);
  void UpdateChannels(VECFILEITEMS &channels);

  CStdString GetDescription() const;
  const int GetNumChannels()   { return m_channels; }
  virtual int GetSelectedItem() const;
  const int GetSelectedChannel() { return m_channelCursor + m_channelOffset; }

  void DoRender(DWORD currentTime);
  void Render();
  void LoadLayout(TiXmlElement *layout);
  void LoadContent(TiXmlElement *content);
  
  virtual bool IsContainer() const { return true; };

protected:
  bool OnClick(DWORD actionID);
  bool SelectItemFromPoint(const CPoint &point);

  void UpdateRuler();

  void RenderRuler(float horzDrawOffset, int blockOffset);
  void RenderChannels(float posY, int chanOffset); // render the column of channels
  void RenderItems(float horzDrawOffset, float posY, int chanOffset, int blockOffset); // render the grid of items

  void RenderChannel(float posX, float posY, CGUIListItem *item, bool focused); // render an individual channel layout
  void RenderItem(float posX, float posY, CGUIListItem *item, bool focused); // render an individual gridItem layout
 
  void RenderDebug();
  void SetChannel(int channel);
  void SetBlock(int block);
  void VerticalScroll(int amount);
  void HorizontalScroll(int amount);
  void ValidateOffset();
  void UpdateLayout(bool refreshAllItems = false);
  void CalculateLayout();
  void GenerateItemLayout(int row, int itemSize, int block);
  void Reset();
  
  CGUIListItemPtr GetItem(const int &channel);
  CGUIListItemPtr GetNextItem(const int &channel);
  CGUIListItemPtr GetPrevItem(const int &channel);
  CGUIListItemPtr GetClosestItem(const int &channel);
  
  int  GetItemSize(CGUIListItemPtr item);
  int  GetBlock(const CGUIListItemPtr &item, const int &channel);
  int  GetRealBlock(const CGUIListItemPtr &item, const int &channel);
  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();

  CGUIListItemLayout *GetFocusedLayout() const;
  
  int   m_rulerUnit; // number of blocks that makes up one element of the ruler
  int   m_channels;
  int   m_channelsPerPage;
  int   m_channelCursor;
  int   m_channelOffset;
  int   m_blocks;
  int   m_blocksPerPage;
  int   m_blockCursor;
  int   m_blockOffset;

  float m_channelPosY; // Y position of first channel row
  float m_gridPosX; // X position of first grid item
  float m_gridWidth;
  float m_gridHeight;
  float m_rulerHeight; // height of the scrolling timeline above the grid items
  float m_rulerWidth; // width of each element of the ruler
  float m_channelHeight;  // height of each channel row (& every grid item)
  float m_channelWidth; // width of the channel item
  float m_blockSize; // a block's width in pixels
  float m_analogScrollCount;

  CDateTime m_gridStart;
  CDateTime m_gridEnd;

  std::vector< CGUIListItemPtr > m_rulerItems;

  std::vector< CGUIListItemPtr > m_channelItems;
  CGUIListItemPtr m_channel;

  CFileItemPtr m_gridIndex[MAXCHANNELS][MAXBLOCKS];
  std::vector< std::vector< CGUIListItemPtr > > m_gridItems;
  typedef std::vector< std::vector< CGUIListItemPtr > >::iterator iChannels;
  typedef std::vector< CGUIListItemPtr >::iterator iShows;
  CGUIListItemPtr  m_item;
  CGUIListItem *m_lastItem;
  CGUIListItem *m_lastChannel;

  DWORD m_renderTime;

  CGUIListItemLayout *m_rulerLayout;
  CGUIListItemLayout *m_channelLayout;
  CGUIListItemLayout *m_focusedChannelLayout;
  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;
  
  int   m_scrollTime;
  bool  m_channelWrapAround; ///only when no more data available should this be true

  void ScrollToBlockOffset(int offset);
  DWORD m_horzScrollLastTime;
  float m_horzScrollSpeed;
  float m_horzScrollOffset;
  void ScrollToChannelOffset(int offset);
  DWORD m_vertScrollLastTime;
  float m_vertScrollSpeed;
  float m_vertScrollOffset;

  CStdString m_label;
  bool m_wasReset;
};
