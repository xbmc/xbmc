/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIListContainer.h
\brief
*/

#include "GUIAction.h"
#include "IGUIContainer.h"
#include "utils/Stopwatch.h"

#include <list>
#include <memory>
#include <utility>
#include <vector>

/*!
 \ingroup controls
 \brief
 */

class IListProvider;
class TiXmlNode;
class CGUIListItemLayout;

class CGUIBaseContainer : public IGUIContainer
{
public:
  CGUIBaseContainer(int parentID, int controlID, float posX, float posY, float width, float height, ORIENTATION orientation, const CScroller& scroller, int preloadItems);
  explicit CGUIBaseContainer(const CGUIBaseContainer& other);
  ~CGUIBaseContainer(void) override;

  bool OnAction(const CAction &action) override;
  void OnDown() override;
  void OnUp() override;
  void OnLeft() override;
  void OnRight() override;
  bool OnMouseOver(const CPoint &point) override;
  bool CanFocus() const override;
  bool OnMessage(CGUIMessage& message) override;
  void SetFocus(bool bOnOff) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void UpdateVisibility(const CGUIListItem *item = NULL) override;
  void AssignDepth() override;

  virtual unsigned int GetRows() const;

  virtual bool HasNextPage() const;
  virtual bool HasPreviousPage() const;

  void SetPageControl(int id);

  std::string GetDescription() const override;
  void SaveStates(std::vector<CControlState> &states) override;
  virtual int GetSelectedItem() const;

  void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;

  void LoadLayout(TiXmlElement *layout);
  void LoadListProvider(TiXmlElement *content, int defaultItem, bool defaultAlways);

  std::shared_ptr<CGUIListItem> GetListItem(int offset, unsigned int flag = 0) const override;

  bool GetCondition(int condition, int data) const override;
  std::string GetLabel(int info) const override;

  /*! \brief Set the list provider for this container (for python).
   \param provider the list provider to use for this container.
   */
  void SetListProvider(std::unique_ptr<IListProvider> provider);

  /*! \brief Set the offset of the first item in the container from the container's position
   Useful for lists/panels where the focused item may be larger than the non-focused items and thus
   normally cut off from the clipping window defined by the container's position + size.
   \param offset CPoint holding the offset in skin coordinates.
   */
  void SetRenderOffset(const CPoint &offset);

  void SetClickActions(const CGUIAction& clickActions) { m_clickActions = clickActions; }
  void SetFocusActions(const CGUIAction& focusActions) { m_focusActions = focusActions; }
  void SetUnFocusActions(const CGUIAction& unfocusActions) { m_unfocusActions = unfocusActions; }

  void SetAutoScrolling(const TiXmlNode *node);
  void ResetAutoScrolling();
  void UpdateAutoScrolling(unsigned int currentTime);

#ifdef _DEBUG
  void DumpTextureUse() override;
#endif
protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  bool OnClick(int actionID);

  virtual void ProcessItem(float posX,
                           float posY,
                           std::shared_ptr<CGUIListItem>& item,
                           bool focused,
                           unsigned int currentTime,
                           CDirtyRegionList& dirtyregions);

  void Render() override;
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(bool wrapAround);
  virtual bool MoveUp(bool wrapAround);
  virtual bool GetOffsetRange(int &minOffset, int &maxOffset) const;
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual void UpdateLayout(bool refreshAllItems = false);
  virtual void SetPageControlRange();
  virtual void UpdatePageControl(int offset);
  virtual void CalculateLayout();
  virtual void SelectItem(int item) {}
  virtual bool SelectItemFromPoint(const CPoint& point) { return false; }
  virtual int GetCursorFromPoint(const CPoint& point, CPoint* itemPoint = NULL) const { return -1; }
  virtual void Reset();
  virtual size_t GetNumItems() const { return m_items.size(); }
  virtual int GetCurrentPage() const;
  bool InsideLayout(const CGUIListItemLayout *layout, const CPoint &point) const;
  void OnFocus() override;
  void OnUnFocus() override;
  void UpdateListProvider(bool forceRefresh = false);

  int ScrollCorrectionRange() const;
  inline float Size() const;
  void FreeMemory(int keepStart, int keepEnd);
  void GetCurrentLayouts();
  CGUIListItemLayout *GetFocusedLayout() const;

  CPoint m_renderOffset; ///< \brief render offset of the first item in the list \sa SetRenderOffset

  float m_analogScrollCount;
  unsigned int m_lastHoldTime;

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  std::vector<std::shared_ptr<CGUIListItem>> m_items;
  typedef std::vector<std::shared_ptr<CGUIListItem>>::iterator iItems;
  std::shared_ptr<CGUIListItem> m_lastItem;

  int m_pageControl;

  std::list<CGUIListItemLayout> m_layouts;
  std::list<CGUIListItemLayout> m_focusedLayouts;

  CGUIListItemLayout* m_layout{nullptr};
  CGUIListItemLayout* m_focusedLayout{nullptr};
  bool m_layoutCondition = false;
  bool m_focusedLayoutCondition = false;

  virtual void ScrollToOffset(int offset);
  void SetContainerMoving(int direction);
  void UpdateScrollOffset(unsigned int currentTime);

  CScroller m_scroller;

  std::unique_ptr<IListProvider> m_listProvider;

  bool m_wasReset;  // true if we've received a Reset message until we've rendered once.  Allows
                    // us to make sure we don't tell the infomanager that we've been moving when
                    // the "movement" was simply due to the list being repopulated (thus cursor position
                    // changing around)

  void UpdateScrollByLetter();
  void GetCacheOffsets(int &cacheBefore, int &cacheAfter) const;
  int GetCacheCount() const { return m_cacheItems; }
  bool ScrollingDown() const { return m_scroller.IsScrollingDown(); }
  bool ScrollingUp() const { return m_scroller.IsScrollingUp(); }
  void OnNextLetter();
  void OnPrevLetter();
  void OnJumpLetter(const std::string& letter, bool skip = false);
  void OnJumpSMS(int letter);
  std::vector< std::pair<int, std::string> > m_letterOffsets;

  /*! \brief Set the cursor position
   Should be used by all base classes rather than directly setting it, as
   this also marks the control as dirty (if needed)
   */
  virtual void SetCursor(int cursor);
  inline int GetCursor() const { return m_cursor; }

  /*! \brief Set the container offset
   Should be used by all base classes rather than directly setting it, as
   this also marks the control as dirty (if needed)
   */
  void SetOffset(int offset);
  /*! \brief Returns the index of the first visible row
   returns the first row. This may be outside of the range of available items. Use GetItemOffset() to retrieve the first visible item in the list.
   \sa GetItemOffset
  */
  inline int GetOffset() const { return m_offset; }
  /*! \brief Returns the index of the first visible item
   returns the first visible item. This will always be in the range of available items. Use GetOffset() to retrieve the first visible row in the list.
   \sa GetOffset
  */
  inline int GetItemOffset() const { return CorrectOffset(GetOffset(), 0); }

  // autoscrolling
  INFO::InfoPtr m_autoScrollCondition;
  int           m_autoScrollMoveTime;   // time between to moves
  unsigned int  m_autoScrollDelayTime;  // current offset into the delay
  bool          m_autoScrollIsReversed; // scroll backwards

  unsigned int m_lastRenderTime;

  struct RENDERITEM
  {
    float posX;
    float posY;
    std::shared_ptr<CGUIListItem> item;
    bool focused;
  };

private:
  bool OnContextMenu();

  int m_cursor;
  int m_offset;
  int m_cacheItems;
  CStopWatch m_scrollTimer;
  CStopWatch m_lastScrollStartTimer;
  CStopWatch m_pageChangeTimer;

  CGUIAction m_clickActions;
  CGUIAction m_focusActions;
  CGUIAction m_unfocusActions;

  // letter match searching
  CStopWatch m_matchTimer;
  std::string m_match;
  float m_scrollItemsPerFrame;
  static const int letter_match_timeout = 1000;

  bool m_gestureActive = false;

  // early inertial scroll cancellation
  bool m_waitForScrollEnd = false;
  float m_lastScrollValue = 0.0f;
};


