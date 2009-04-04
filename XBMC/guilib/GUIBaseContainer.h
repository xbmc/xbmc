/*!
\file GUIListContainer.h
\brief
*/

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
#include "boost/shared_ptr.hpp"
#include "utils/Stopwatch.h"

typedef boost::shared_ptr<CGUIListItem> CGUIListItemPtr;

/*!
 \ingroup controls
 \brief
 */

class CGUIBaseContainer : public CGUIControl
{
public:
  CGUIBaseContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime);
  virtual ~CGUIBaseContainer(void);

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

  virtual unsigned int GetRows() const;

  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

  void SetPageControl(DWORD id);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(std::vector<CControlState> &states);
  virtual int GetSelectedItem() const;

  virtual void DoRender(DWORD currentTime);
  void LoadLayout(TiXmlElement *layout);
  void LoadContent(TiXmlElement *content);

  VIEW_TYPE GetType() const { return m_type; };
  const CStdString &GetLabel() const { return m_label; };
  void SetType(VIEW_TYPE type, const CStdString &label);

  virtual bool IsContainer() const { return true; };
  CGUIListItemPtr GetListItem(int offset, unsigned int flag = 0) const;

  virtual bool GetCondition(int condition, int data) const;
  CStdString GetLabel(int info) const;

  void SetStaticContent(const std::vector<CGUIListItemPtr> &items);

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  bool OnClick(DWORD actionID);
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void MoveToItem(int item);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual void UpdateLayout(bool refreshAllItems = false);
  virtual void SetPageControlRange();
  virtual void CalculateLayout();
  virtual void SelectItem(int item) {};
  virtual void Reset();
  virtual unsigned int GetNumItems() const { return m_items.size(); };
  virtual int GetCurrentPage() const;
  bool InsideLayout(const CGUIListItemLayout *layout, const CPoint &point);

  inline float Size() const;
  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();
  CGUIListItemLayout *GetFocusedLayout() const;

  int m_offset;
  int m_cursor;
  float m_analogScrollCount;
  unsigned int m_lastHoldTime;

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  std::vector< CGUIListItemPtr > m_items;
  typedef std::vector<CGUIListItemPtr> ::iterator iItems;
  CGUIListItem *m_lastItem;

  DWORD m_pageControl;

  DWORD m_renderTime;

  std::vector<CGUIListItemLayout> m_layouts;
  std::vector<CGUIListItemLayout> m_focusedLayouts;

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;

  virtual void ScrollToOffset(int offset);
  void UpdateScrollOffset();

  DWORD m_scrollLastTime;
  int   m_scrollTime;
  float m_scrollOffset;

  VIEW_TYPE m_type;
  CStdString m_label;

  bool m_staticContent;
  std::vector<CGUIListItemPtr> m_staticItems;
  bool m_wasReset;  // true if we've received a Reset message until we've rendered once.  Allows
                    // us to make sure we don't tell the infomanager that we've been moving when
                    // the "movement" was simply due to the list being repopulated (thus cursor position
                    // changing around)

  void UpdateScrollByLetter();
  void OnNextLetter();
  void OnPrevLetter();
  void OnJumpLetter(char letter);
  void OnJumpSMS(int letter);
  std::vector< std::pair<int, CStdString> > m_letterOffsets;
private:
  float m_scrollSpeed;
  CStopWatch m_scrollTimer;
  CStopWatch m_pageChangeTimer;

  // letter match searching
  CStopWatch m_matchTimer;
  CStdString m_match;

  static const int letter_match_timeout = 1000;
};


