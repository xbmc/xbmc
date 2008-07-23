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

class CGUIEPGGridContainer : public CGUIControl
{
public:
  CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, int scrollTime, int minutesPerPage);
  virtual ~CGUIEPGGridContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMessage(CGUIMessage& message);

  const int GetNumChannels()   { return m_channels; }
  void UpdateItems(EPGGrid &gridData, const CDateTime &dataStart, const CDateTime &dataEnd);

  CStdString GetDescription() const;
  int GetSelectedItem() const;

  void DoRender(DWORD currentTime);
  void Render();
  void LoadLayout(TiXmlElement *layout);
  void LoadContent(TiXmlElement *content);
  
  virtual bool IsContainer() const { return true; };

protected:
  bool OnClick(DWORD actionID);
  bool SelectItemFromPoint(const CPoint &point);
  void RenderItem(float posX, float posY, CGUIListItem  *item, bool focused);
  void RenderRuler();
  void Scroll(int amount);
  void SetChannel(int cursor);
  bool MoveDown(bool wrapAround);
  bool MoveUp(bool wrapAround);
  bool MoveLeft(bool wrapAround);
  bool MoveRight(bool wrapAround);
  void ValidateOffset();
  int  CorrectOffset(int offset, int cursor) const;
  void UpdateLayout(bool refreshAllItems = false);
  void CalculateLayout();

  //int  GetItemAbove(int channel, int item);
  //int  GetItemBelow(int channel, int item);
  //int  GetItemLeft (int channel, int item);
  //int  GetItemRight(int channel, int item);
  int  GetBlock(const int &item, const int &channel);
  int  GetItem(const int &channel);
  int  GetClosestItem(const int &channel);

  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();

  CGUIListItemLayout *GetFocusedLayout() const;
  
  int m_item;
  int m_channels;
  int m_channelsPerPage;
  int m_channel;
  int m_chanOffset;
  int m_blocks;
  int m_blocksPerPage;
  int m_block;
  int m_blockOffset; 

  float m_analogScrollCount;
  bool  m_favouritesOnly;

  std::vector< std::vector< int > > m_gridIndex;
  std::vector< std::vector< CGUIListItemPtr > > m_gridItems;
  typedef std::vector< std::vector< CGUIListItemPtr > >::iterator itChannels;
  typedef std::vector< CGUIListItemPtr >::iterator itShows;
  CGUIListItem *m_lastItem;

  DWORD m_renderTime;

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;
  
  void ScrollToBlockOffset(int offset);
  void ScrollToChannelOffset(int offset);
  DWORD m_scrollLastTime;
  int   m_scrollTime;
  float m_scrollSpeed;
  float m_scrollOffset;
  bool  m_wrapAround;

  CStdString m_label;
  bool m_wasReset;
};
