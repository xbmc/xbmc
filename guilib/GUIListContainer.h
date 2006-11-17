/*!
\file GUIListContainer.h
\brief 
*/

#pragma once

#include "GUIControl.h"

class CGUIListItem;

/*!
 \ingroup controls
 \brief 
 */
class CGUIListContainer : public CGUIControl
{
public:
  CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation);
  virtual ~CGUIListContainer(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnLeft();
  virtual void OnRight();
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseDoubleClick(DWORD dwButton);
  virtual bool OnMouseWheel();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetPulseOnSelect(bool pulse);
  virtual void SetFocus(bool bOnOff);

  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);

  void SetPageControl(DWORD id);

  void SetItemSize(float itemWidth, float itemHeight, float focusedWidth, float focusedHeight);
  void SetItemLayout(const vector<CGUIControl*> &itemLayout, const vector<CGUIControl*> &focusedLayout);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(vector<CControlState> &states);
  int GetSelectedItem() const;

  virtual void Animate(DWORD currentTime);
  virtual void UpdateEffectState(DWORD currentTime);

protected:
  bool SelectItemFromPoint(float posX, float posY);
  void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  void Scroll(int amount);
  bool MoveDown(DWORD nextControl);
  bool MoveUp(DWORD nextControl);
  void ValidateOffset();
  void UpdateLayout();
  inline float Size() const;

  int m_offset;
  int m_cursor;

  float m_itemSize;
  float m_focusedSize;
  ORIENTATION m_orientation;
  int m_itemsPerPage;

  string m_strSuffix;
  vector<CGUIListItem*> m_items;
  typedef vector<CGUIListItem*> ::iterator iItems;

  DWORD m_pageControl;

  vector<CGUIControl*> m_controls;
  vector<CGUIControl*> m_focusedControls;
  typedef vector<CGUIControl*>::iterator iControls;

  DWORD m_renderTime;

  void ScrollToOffset();
  DWORD m_scrollTime;
  float m_scrollSpeed;
  float m_scrollOffset;
};

