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
                  const CStdString& strFontName,
                  DWORD dwSpinWidth, DWORD dwSpinHeight,
                  const CStdString& strUp, const CStdString& strDown,
                  const CStdString& strUpFocus, const CStdString& strDownFocus,
                  DWORD dwSpinColor, int iSpinX, int iSpinY,
                  const CStdString& strFont, DWORD dwTextColor, DWORD dwSelectedColor,
                  const CStdString& strButton, const CStdString& strButtonFocus,
                  DWORD dwItemTextOffsetX, DWORD dwItemTextOffsetY);
  virtual ~CGUIListControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void OnRight();
  virtual void OnLeft();
  virtual void OnDown();
  virtual void OnUp();
  virtual void OnMouseOver();
  virtual void OnMouseClick(DWORD dwButton);
  virtual void OnMouseDoubleClick(DWORD dwButton);
  virtual void OnMouseWheel();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual bool HitTest(int iPosX, int iPosY) const;

  virtual bool CanFocus() const;

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  void SetScrollySuffix(const CStdString& wstrSuffix);
  void SetTextOffsets(int iXoffset, int iYOffset, int iXoffset2, int iYOffset2);
  void SetImageDimensions(int iWidth, int iHeight);
  void SetItemHeight(int iHeight);
  void SetSpace(int iHeight);
  void SetFont2(const CStdString& strFont);
  void SetColors2(DWORD dwTextColor, DWORD dwSelectedColor);
  void SetPageControlVisible(bool bVisible);
  int GetSelectedItem(CStdString& strLabel);
  int GetSelectedItem();
  bool SelectItemFromPoint(int iPosX, int iPosY);
  void GetPointFromItem(int &iPosX, int &iPosY);
  DWORD GetTextColor() const { return m_dwTextColor;};
  DWORD GetTextColor2() const { return m_dwTextColor2;};
  DWORD GetSelectedColor() const { return m_dwSelectedColor;};
  DWORD GetSelectedColor2() const { return m_dwSelectedColor2;};
  const char* GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
  const char* GetFontName2() const { if (!m_pFont2) return ""; else return m_pFont2->GetFontName().c_str(); };
DWORD GetSpinWidth() const { return m_upDown.GetWidth() / 2; };
  DWORD GetSpinHeight() const { return m_upDown.GetHeight(); };
  const CStdString& GetTextureUpName() const { return m_upDown.GetTextureUpName(); };
  const CStdString& GetTextureDownName() const { return m_upDown.GetTextureDownName(); };
  const CStdString& GetTextureUpFocusName() const { return m_upDown.GetTextureUpFocusName(); };
  const CStdString& GetTextureDownFocusName() const { return m_upDown.GetTextureDownFocusName(); };
  DWORD GetSpinTextColor() const { return m_upDown.GetTextColor();};
  int GetSpinX() const { return m_upDown.GetXPosition();};
  int GetSpinY() const { return m_upDown.GetYPosition();};
  DWORD GetSpace() const { return m_iSpaceBetweenItems;};
  DWORD GetItemHeight() const { return m_iItemHeight; };
  DWORD GetButtonTextOffsetX() const { return m_imgButton.GetTextOffsetX();};
  DWORD GetButtonTextOffsetY() const { return m_imgButton.GetTextOffsetY();};
  DWORD GetTextOffsetX() const { return m_iTextOffsetX;};
  DWORD GetTextOffsetY() const { return m_iTextOffsetY;};
  DWORD GetTextOffsetX2() const { return m_iTextOffsetX2;};
  DWORD GetTextOffsetY2() const { return m_iTextOffsetY2;};
  void SetAlignmentY(DWORD dwAlign) { m_dwTextAlign = dwAlign; };
  DWORD GetAlignmentY() const { return m_dwTextAlign;};
  DWORD GetImageWidth() const { return m_iImageWidth;};
  DWORD GetImageHeight() const { return m_iImageHeight;};
  const wstring& GetSuffix() const { return m_strSuffix;};
  const CStdString GetButtonFocusName() const { return m_imgButton.GetTextureFocusName();};
  const CStdString GetButtonNoFocusName() const { return m_imgButton.GetTextureNoFocusName();};
  int GetNumItems() const { return (int)m_vecItems.size(); };
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
  int m_iTextOffsetX;
  int m_iTextOffsetY;
  int m_iTextOffsetX2;
  int m_iTextOffsetY2;
  DWORD m_dwTextAlign;
  int m_iImageWidth;
  int m_iImageHeight;
  bool m_bUpDownVisible;
  DWORD m_dwTextColor, m_dwTextColor2;
  DWORD m_dwSelectedColor, m_dwSelectedColor2;
  CGUIFont* m_pFont;
  CGUIFont* m_pFont2;
  CGUISpinControl m_upDown;
  CGUIButtonControl m_imgButton;
  wstring m_strSuffix;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
};
#endif
