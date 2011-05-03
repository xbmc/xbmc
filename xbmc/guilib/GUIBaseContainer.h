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
  CGUIBaseContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime, int preloadItems);
  virtual ~CGUIBaseContainer(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetFocus(bool bOnOff);
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);

  virtual unsigned int GetRows() const;

  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

  void SetPageControl(int id);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(std::vector<CControlState> &states);
  virtual int GetSelectedItem() const;

  virtual void DoProcess(unsigned int currentTime);
  virtual void Process(unsigned int currentTime);

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
  
  /*! \brief Set the offset of the first item in the container from the container's position
   Useful for lists/panels where the focused item may be larger than the non-focused items and thus
   normally cut off from the clipping window defined by the container's position + size.
   \param offset CPoint holding the offset in skin coordinates.
   */
  void SetRenderOffset(const CPoint &offset);

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  bool OnClick(int actionID);

  virtual void ProcessItem(float posX, float posY, CGUIListItem *item, bool focused, unsigned int currentTime);

  virtual void Render();
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual void UpdateLayout(bool refreshAllItems = false);
  virtual void SetPageControlRange();
  virtual void UpdatePageControl(int offset);
  virtual void CalculateLayout();
  virtual void SelectItem(int item) {};
  virtual bool SelectItemFromPoint(const CPoint &point) { return false; };
  virtual int GetCursorFromPoint(const CPoint &point, CPoint *itemPoint = NULL) const { return -1; };
  virtual void Reset();
  virtual unsigned int GetNumItems() const { return m_items.size(); };
  virtual int GetCurrentPage() const;
  bool InsideLayout(const CGUIListItemLayout *layout, const CPoint &point) const;

  inline float Size() const;
  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();
  CGUIListItemLayout *GetFocusedLayout() const;

  CPoint m_renderOffset; ///< \brief render offset of the first item in the list \sa SetRenderOffset
    
  int m_offset;
  int m_cursor;
  float m_analogScrollCount;
  unsigned int m_lastHoldTime;

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  std::vector< CGUIListItemPtr > m_items;
  typedef std::vector<CGUIListItemPtr> ::iterator iItems;
  CGUIListItem *m_lastItem;

  int m_pageControl;

  std::vector<CGUIListItemLayout> m_layouts;
  std::vector<CGUIListItemLayout> m_focusedLayouts;

  CGUIListItemLayout *m_layout;
  CGUIListItemLayout *m_focusedLayout;

  void ScrollToOffset(int offset);
  void SetContainerMoving(int direction);
  void UpdateScrollOffset(unsigned int currentTime);

  unsigned int m_scrollLastTime;
  int          m_scrollTime;
  float        m_scrollOffset;

  VIEW_TYPE m_type;
  CStdString m_label;

  bool m_staticContent;
  unsigned int m_staticUpdateTime;
  std::vector<CGUIListItemPtr> m_staticItems;
  bool m_wasReset;  // true if we've received a Reset message until we've rendered once.  Allows
                    // us to make sure we don't tell the infomanager that we've been moving when
                    // the "movement" was simply due to the list being repopulated (thus cursor position
                    // changing around)

  void UpdateScrollByLetter();
  void GetCacheOffsets(int &cacheBefore, int &cacheAfter);
  bool ScrollingDown() const { return m_scrollSpeed > 0; };
  bool ScrollingUp() const { return m_scrollSpeed < 0; };
  void OnNextLetter();
  void OnPrevLetter();
  void OnJumpLetter(char letter);
  void OnJumpSMS(int letter);
  std::vector< std::pair<int, CStdString> > m_letterOffsets;
private:
  int m_cacheItems;
  float m_scrollSpeed;
  CStopWatch m_scrollTimer;
  CStopWatch m_pageChangeTimer;

  // letter match searching
  CStopWatch m_matchTimer;
  CStdString m_match;

  static const int letter_match_timeout = 1000;
};


