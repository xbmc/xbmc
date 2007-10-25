/*!
\file GUIListContainer.h
\brief 
*/

#pragma once

#include "GUIControl.h"
#include "GUIListItemLayout.h"

enum VIEW_TYPE { VIEW_TYPE_NONE = 0,
                 VIEW_TYPE_LIST,
                 VIEW_TYPE_ICON,
                 VIEW_TYPE_BIG_LIST,
                 VIEW_TYPE_BIG_ICON,
                 VIEW_TYPE_WIDE,
                 VIEW_TYPE_BIG_WIDE,
                 VIEW_TYPE_WRAP,
                 VIEW_TYPE_BIG_WRAP,
                 VIEW_TYPE_AUTO,
                 VIEW_TYPE_MAX };
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
  virtual void UpdateVisibility();

  virtual unsigned int GetRows() const;

  void SetPageControl(DWORD id);

  virtual CStdString GetDescription() const;
  virtual void SaveStates(vector<CControlState> &states);
  int GetSelectedItem() const;

  virtual void DoRender(DWORD currentTime);
  void LoadLayout(TiXmlElement *layout);
  void LoadContent(TiXmlElement *content);

  VIEW_TYPE GetType() const { return m_type; };
  const CStdString &GetLabel() const { return m_label; };
  void SetType(VIEW_TYPE type, const CStdString &label);

  virtual bool IsContainer() const { return true; };
  CGUIListItem *GetListItem(int offset) const;

  virtual bool GetCondition(int condition, int data) const;

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  bool OnClick(DWORD actionID);
  virtual bool SelectItemFromPoint(const CPoint &point);
  virtual void RenderItem(float posX, float posY, CGUIListItem *item, bool focused);
  virtual void Scroll(int amount);
  virtual bool MoveDown(DWORD nextControl);
  virtual bool MoveUp(DWORD nextControl);
  virtual void MoveToItem(int item);
  virtual void ValidateOffset();
  virtual int  CorrectOffset(int offset, int cursor) const;
  virtual void UpdateLayout();
  virtual void CalculateLayout();
  bool InsideLayout(const CGUIListItemLayout &layout, const CPoint &point);

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
  CGUIListItem *m_lastItem;

  DWORD m_pageControl;

  DWORD m_renderTime;

  CGUIListItemLayout m_layout;
  CGUIListItemLayout m_focusedLayout;

  virtual void ScrollToOffset(int offset);
  DWORD m_scrollLastTime;
  int   m_scrollTime;
  float m_scrollSpeed;
  float m_scrollOffset;

  VIEW_TYPE m_type;
  CStdString m_label;

  bool m_staticContent;
  vector<CGUIListItem*> m_staticItems;
};

