/*!
\file GUIListContainer.h
\brief 
*/

#pragma once

#include "GUIControl.h"
#include "GUIListItemLayout.h"
/*!
 \ingroup controls
 \brief 
 */
class CGUIBaseContainer : public CGUIControl
{
public:
  CGUIBaseContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation, int scrollTime);
  virtual ~CGUIBaseContainer(void);

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
  virtual void SetFocus(bool bOnOff);
  virtual void AllocResources();
  virtual unsigned int GetRows() const;

  void SetPageControl(DWORD id);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(vector<CControlState> &states);
  int GetSelectedItem() const;

  virtual void Animate(DWORD currentTime);
  virtual void UpdateEffectState(DWORD currentTime);
  void LoadLayout(TiXmlElement *layout);

protected:
  virtual bool SelectItemFromPoint(float posX, float posY);
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(DWORD nextControl);
  virtual bool MoveUp(DWORD nextControl);
  virtual void MoveToItem(int item);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset) const;
  virtual void UpdateLayout();
  virtual void CalculateLayout();

  inline float Size() const;
  void MoveToRow(int row);
  void FreeMemory(int keepStart, int keepEnd);

  int m_offset;
  int m_cursor;
  float m_analogScrollCount;

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  vector<CGUIListItem*> m_items;
  typedef vector<CGUIListItem*> ::iterator iItems;

  DWORD m_pageControl;

  DWORD m_renderTime;

  CGUIListItemLayout m_layout;
  CGUIListItemLayout m_focusedLayout;

  virtual void ScrollToOffset(int offset);
  DWORD m_scrollLastTime;
  int   m_scrollTime;
  float m_scrollSpeed;
  float m_scrollOffset;
};

