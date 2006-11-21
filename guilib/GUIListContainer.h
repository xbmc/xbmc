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
class CGUIListContainer : public CGUIControl
{
public:
  CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, ORIENTATION orientation);
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  CGUIListContainer(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                         const CImage& textureButton, const CImage& textureButtonFocus,
                         float textureHeight, float itemWidth, float itemHeight, float spaceBetweenItems, CGUIControl *pSpin);
  CGUIControl *m_spinControl;
//#endif
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
  virtual void SetFocus(bool bOnOff);

  void SetPageControl(DWORD id);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(vector<CControlState> &states);
  int GetSelectedItem() const;

  virtual void Animate(DWORD currentTime);
  virtual void UpdateEffectState(DWORD currentTime);
  void LoadLayout(TiXmlElement *layout);

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

  ORIENTATION m_orientation;
  int m_itemsPerPage;

  string m_strSuffix;
  vector<CGUIListItem*> m_items;
  typedef vector<CGUIListItem*> ::iterator iItems;

  DWORD m_pageControl;

  DWORD m_renderTime;

  CGUIListItemLayout m_layout;
  CGUIListItemLayout m_focusedLayout;

  float m_analogScrollCount;

  void ScrollToOffset();
  DWORD m_scrollTime;
  float m_scrollSpeed;
  float m_scrollOffset;
};

