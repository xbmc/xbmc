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
#include "GUIListItem.h"
#include "TVDatabase.h"

#define MAXCHANNELS 200
#define MAXBLOCKS   576 // 2 days of 5 minute blocks

class CGUIEPGGridContainer : public CGUIControl
{
public:
  CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, 
                       float width, float height, int scrollTime, int minutesPerPage);
  virtual ~CGUIEPGGridContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();

  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetFocus(bool bOnOff);

  void UpdateItems(EPGGrid &gridData, const CDateTime &dataStart, const CDateTime &dataEnd);

  CStdString GetDescription() const;
  const int GetNumChannels()   { return m_channels; }
  int GetSelectedItem() const;

  void DoRender(DWORD currentTime);
  void Render();
  void LoadLayout(TiXmlElement *layout);
  void LoadContent(TiXmlElement *content);
  
  virtual bool IsContainer() const { return true; };

protected:
  bool OnClick(DWORD actionID);
  bool SelectItemFromPoint(const CPoint &point);
  void RenderItem(float posX, float posY, CGUIListItemPtr item, bool focused);
  void RenderDebug();
  void SetChannel(int channel);
  void SetBlock(int block);
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
  
  int   m_channels;
  int   m_channelsPerPage;
  int   m_channel;
  int   m_channelOffset;
  int   m_blocks;
  int   m_blocksPerPage;
  int   m_block;
  int   m_blockOffset;

  float m_blockSize;
  float m_analogScrollCount;

  CDateTime m_gridStart;
  CDateTime m_gridEnd;

  CFileItemPtr m_gridIndex[MAXCHANNELS][MAXBLOCKS];
  std::vector< std::vector< CGUIListItemPtr > > m_gridItems;
  typedef std::vector< std::vector< CGUIListItemPtr > >::iterator iChannels;
  typedef std::vector< CGUIListItemPtr >::iterator iShows;
  CGUIListItemPtr  m_item;
  CGUIListItemPtr  m_lastItem;

  DWORD m_renderTime;

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
