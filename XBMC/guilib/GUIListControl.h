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
  CGUIListControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                  DWORD dwSpinWidth, DWORD dwSpinHeight,
                  const CStdString& strUp, const CStdString& strDown,
                  const CStdString& strUpFocus, const CStdString& strDownFocus,
                  DWORD dwSpinColor, int iSpinX, int iSpinY,
                  const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                  const CStdString& strButton, const CStdString& strButtonFocus);
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
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetPulseOnSelect(bool pulse);
  virtual bool HitTest(int iPosX, int iPosY) const;

  virtual bool CanFocus() const;

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  void SetScrollySuffix(const CStdString& wstrSuffix);
  void SetImageDimensions(int iWidth, int iHeight);
  void SetItemHeight(int iHeight);
  void SetSpace(int iHeight);
  void SetPageControlVisible(bool bVisible);
  int GetSelectedItem() const;
  bool SelectItemFromPoint(int iPosX, int iPosY);
  void GetPointFromItem(int &iPosX, int &iPosY);
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  const CLabelInfo& GetLabelInfo2() const { return m_label2; };
  DWORD GetSpinWidth() const { return m_upDown.GetWidth() / 2; };
  DWORD GetSpinHeight() const { return m_upDown.GetHeight(); };
  const CStdString& GetTextureUpName() const { return m_upDown.GetTextureUpName(); };
  const CStdString& GetTextureDownName() const { return m_upDown.GetTextureDownName(); };
  const CStdString& GetTextureUpFocusName() const { return m_upDown.GetTextureUpFocusName(); };
  const CStdString& GetTextureDownFocusName() const { return m_upDown.GetTextureDownFocusName(); };
  DWORD GetSpinTextColor() const { return m_upDown.GetLabelInfo().textColor;};
  int GetSpinX() const { return m_iSpinPosX;};
  int GetSpinY() const { return m_iSpinPosY;};
  DWORD GetSpace() const { return m_iSpaceBetweenItems;};
  DWORD GetItemHeight() const { return m_iItemHeight; };
  DWORD GetImageWidth() const { return m_iImageWidth;};
  DWORD GetImageHeight() const { return m_iImageHeight;};
  const wstring& GetSuffix() const { return m_strSuffix;};
  const CStdString GetButtonFocusName() const { return m_imgButton.GetTextureFocusName();};
  const CStdString GetButtonNoFocusName() const { return m_imgButton.GetTextureNoFocusName();};
  int GetNumItems() const { return (int)m_vecItems.size(); };
  virtual CStdString GetDescription() const;

protected:

  void RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, bool bScroll );
  void Scroll(int iAmount);
  int GetPage();
  int m_iSpaceBetweenItems;
  int m_iOffset;
  float m_fSmoothScrollOffset;
  int m_iItemsPerPage;
  int m_iItemHeight;
  int m_iSelect;
  int m_iCursorY;
  int m_iSpinPosX;
  int m_iSpinPosY;
  int m_iImageWidth;
  int m_iImageHeight;
  bool m_bUpDownVisible;
  CLabelInfo m_label;
  CLabelInfo m_label2;
  CGUISpinControl m_upDown;
  CGUIButtonControl m_imgButton;
  wstring m_strSuffix;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
  CScrollInfo m_scrollInfo;
};
#endif
