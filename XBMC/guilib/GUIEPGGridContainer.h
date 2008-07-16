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
#include "GUIEPGGridItemLayout.h"
#include "GUIEPGGridItem.h"
#include "TVDatabase.h"

class CGUIEPGGridContainer : public CGUIControl
{
public:
  CGUIEPGGridContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, int scrollTime);
  virtual ~CGUIEPGGridContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseDoubleClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetFocus(bool bOnOff);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);


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
  void RenderItem(float posX, float posY, CGUIEPGGridItem *item, bool focused);
  void Scroll(int amount);
  void SetCursor(int cursor);
  bool MoveDown(bool wrapAround);
  bool MoveUp(bool wrapAround);
  bool MoveLeft(bool wrapAround);
  bool MoveRight(bool wrapAround);
  //void MoveToItem(int item);
  void ValidateOffset();
  int CorrectOffset(int offset, int cursor) const;
  void UpdateLayout(bool refreshAllItems = false);
  void CalculateLayout();
  //void SelectItem(int item) {};
  //void Reset();
  unsigned int GetNumItems() const { return m_gridItems.size(); };

  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();

  CGUIListItemLayout *GetFocusedLayout() const;
  int m_offset;
  int m_cursor;
  float m_analogScrollCount;

  int m_numChannels;
  int m_channelsPerPage;

  std::vector< std::vector< CGUIEPGGridItem* > > m_gridItems;
  typedef std::vector< std::vector< CGUIEPGGridItem* > >::iterator itChannels;
  typedef std::vector< CGUIEPGGridItem* >::iterator itShows;
  CGUIEPGGridItem *m_lastItem;

  DWORD m_renderTime;

  CGUIListItemLayout m_gridLayout;
  CGUIListItemLayout m_focusedGridLayout;

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;

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
