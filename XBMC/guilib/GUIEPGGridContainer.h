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

  const int GetNumChannels()   { return m_numChannels; }
  void UpdateItems(EPGGrid &gridData);

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

  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();

  CGUIListItemLayout *GetFocusedLayout() const;
  int m_chanOffset;
  int m_itemOffset;
  int m_curChannel;
  int m_curItem;
  float m_analogScrollCount;
  bool  m_favouritesOnly;
  int m_numChannels;
  int m_channelsPerPage;
  int m_numTimeBlocks;
  
  std::vector< std::vector< int > > m_gridIndex; //todo clear
  std::vector< std::vector< CGUIListItemPtr > > m_gridItems; //todo clear
  typedef std::vector< std::vector< CGUIListItemPtr > >::iterator itChannels;
  typedef std::vector< CGUIListItemPtr >::iterator itShows;
  CGUIListItem *m_lastItem;

  DWORD m_renderTime;

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;

  std::map<CGUIListItem*, CGUIListItemLayout*> m_itemLayouts; //matches individual items with their layouts

  EPGGrid *m_pGridData;
  
  void ScrollToOffset(int offset);
  DWORD m_scrollLastTime;
  int   m_scrollTime;
  float m_scrollSpeed;
  float m_scrollOffset;
  bool  m_wrapAround;

  CStdString m_label;
  bool m_wasReset;
};
