/*!
\file GUIListControl.h
\brief 
*/

#ifndef GUILIB_GUILISTCONTROL_H
#define GUILIB_GUILISTCONTROL_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"
#include "GUIListItem.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIListControl : public CGUIControl
{
public:
  CGUIListControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                  float spinWidth, float spinHeight,
                  const CImage& textureUp, const CImage& textureDown,
                  const CImage& textureUpFocus, const CImage& textureDownFocus,
                  const CLabelInfo& spinInfo, float spinX, float spinY,
                  const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                  const CImage& textureButton, const CImage& textureButtonFocus);
  virtual ~CGUIListControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnRight();
  virtual void OnLeft();
  virtual void OnDown();
  virtual void OnUp();
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseDoubleClick(DWORD dwButton);
  virtual bool OnMouseWheel();
  virtual bool HitTest(float posX, float posY) const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  virtual void SetPosition(float posX, float posY);
  virtual void SetPulseOnSelect(bool pulse);

  virtual bool CanFocus() const;

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  void SetScrollySuffix(const CStdString& wstrSuffix);
  void SetImageDimensions(float width, float height);
  void SetItemHeight(float height);
  void SetSpaceBetweenItems(float spaceBetweenItems);
  void SetPageControlVisible(bool bVisible);
  void SetPageControl(DWORD id);
  int GetSelectedItem() const;
  bool SelectItemFromPoint(float posX, float posY);
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  virtual CStdString GetDescription() const;
  virtual void SaveStates(vector<CControlState> &states);

protected:
  class CListText {
  public:
    CListText() { x = y = width = maxwidth = height = 0.0f; selected = highlighted = false; };
    CStdStringW text;
    float x;
    float y;
    float width;
    float maxwidth;
    float height;
    bool selected;
    bool highlighted;
  };

  void RenderText(const CListText &text, const CLabelInfo &label, CScrollInfo &scroll);
  void Scroll(int iAmount);
  int GetPage();

  float m_spaceBetweenItems;
  int m_iOffset;
  float m_smoothScrollOffset;
  int m_iItemsPerPage;
  float m_itemHeight;
  int m_iSelect;
  int m_iCursorY;
  float m_spinPosX;
  float m_spinPosY;
  float m_imageWidth;
  float m_imageHeight;
  bool m_bUpDownVisible;
  CLabelInfo m_label;
  CLabelInfo m_label2;
  CGUISpinControl m_upDown;
  CGUIButtonControl m_imgButton;
  string m_strSuffix;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
  CScrollInfo m_scrollInfo;
  CScrollInfo m_scrollInfo2;

  DWORD m_pageControl;
};
#endif
