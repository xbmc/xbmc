/*!
\file GUIThumbnailPanel.h
\brief 
*/

#ifndef GUILIB_GUITHUMBNAILCONTROL_H
#define GUILIB_GUITHUMBNAILCONTROL_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"
#include "GUIListItem.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIThumbnailPanel :
      public CGUIControl
{
public:
  CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                     const CStdString& strImageIcon,
                     const CStdString& strImageIconFocus,
                     DWORD dwSpinWidth, DWORD dwSpinHeight,
                     const CStdString& strUp, const CStdString& strDown,
                     const CStdString& strUpFocus, const CStdString& strDownFocus,
                     DWORD dwSpinColor, int iSpinX, int iSpinY,
                     const CLabelInfo& labelInfo);
  virtual ~CGUIThumbnailPanel(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);

  virtual bool CanFocus() const;

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  virtual void SetPulseOnSelect(bool pulse);
  void SetScrollySuffix(CStdString wstrSuffix);
  void SetThumbAlign(int align);
  int GetThumbAlign();
  void SetThumbDimensions(int iXpos, int iYpos, int iWidth, int iHeight);
  void GetThumbDimensions(int& iXpos, int& iYpos, int& iWidth, int& iHeight);
  void GetThumbDimensionsBig(int& iXpos, int& iYpos, int& iWidth, int& iHeight);
  void GetThumbDimensionsLow(int& iXpos, int& iYpos, int& iWidth, int& iHeight);
  void SetSelectedItem(int iItem);
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  DWORD GetSpinWidth() const { return m_upDown.GetWidth() / 2; };
  DWORD GetSpinHeight() const { return m_upDown.GetHeight(); };
  const CStdString& GetTextureUpName() const { return m_upDown.GetTextureUpName(); };
  const CStdString& GetTextureDownName() const { return m_upDown.GetTextureDownName(); };
  const CStdString& GetTextureUpFocusName() const { return m_upDown.GetTextureUpFocusName(); };
  const CStdString& GetTextureDownFocusName() const { return m_upDown.GetTextureDownFocusName(); };
  DWORD GetSpinTextColor() const { return m_upDown.GetLabelInfo().textColor;};
  int GetSpinX() const { return m_iSpinPosX;};
  int GetSpinY() const { return m_iSpinPosY;};
  const CStdString& GetFocusName() const { return m_imgFolderFocus.GetFileName();};
  const CStdString& GetNoFocusName() const { return m_imgFolder.GetFileName();};
  void SetItemWidth(DWORD dwWidth);
  void SetItemHeight(DWORD dwHeight);
  bool IsTextureShown() const { return m_bShowTexture;};
  void ShowTexture(bool bOnoff);
  void SetTextureWidthBig(int textureWidthBig) { m_iTextureWidthBig = textureWidthBig;};
  void SetTextureHeightBig(int textureHeightBig){ m_iTextureHeightBig = textureHeightBig;};
  void SetItemWidthBig(int itemWidthBig){ m_iItemWidthBig = itemWidthBig;};
  void SetItemHeightBig(int itemHeightBig) { m_iItemHeightBig = itemHeightBig;};

  void SetTextureWidthLow(int textureWidthLow) { m_iTextureWidthLow = textureWidthLow;};
  void SetTextureHeightLow(int textureHeightLow){ m_iTextureHeightLow = textureHeightLow;};
  void SetItemWidthLow(int itemWidthLow){ m_iItemWidthLow = itemWidthLow;};
  void SetItemHeightLow(int itemHeightLow) { m_iItemHeightLow = itemHeightLow;};

  int GetTextureWidthBig() const { return m_iTextureWidthBig;};
  int GetTextureHeightBig() const { return m_iTextureHeightBig;};
  int GetItemWidthBig() const { return m_iItemWidthBig;};
  int GetItemHeightBig() const { return m_iItemHeightBig;};

  int GetTextureWidthLow() const { return m_iTextureWidthLow;};
  int GetTextureHeightLow() const { return m_iTextureHeightLow;};
  int GetItemWidthLow() const { return m_iItemWidthLow;};
  int GetItemHeightLow() const { return m_iItemHeightLow;};
  void ShowBigIcons(bool bOnOff);
  const wstring& GetSuffix() const { return m_strSuffix;};
  void SetThumbDimensionsLow(int iXpos, int iYpos, int iWidth, int iHeight) { m_iThumbXPosLow = iXpos;m_iThumbYPosLow = iYpos;m_iThumbWidthLow = iWidth;m_iThumbHeightLow = iHeight;};
  void SetThumbDimensionsBig(int iXpos, int iYpos, int iWidth, int iHeight) { m_iThumbXPosBig = iXpos;m_iThumbYPosBig = iYpos;m_iThumbWidthBig = iWidth;m_iThumbHeightBig = iHeight;};
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseDoubleClick(DWORD dwButton);
  virtual bool OnMouseWheel();
  bool ScrollDown();
  void ScrollUp();
  virtual bool HitTest(int iPosX, int iPosY) const;
  bool SelectItemFromPoint(int iPosX, int iPosY);
  void GetOffsetFromPage();
  virtual CStdString GetDescription() const;

  void HideFileNameLabel(bool bOnOff) { m_bHideFileNameLabel=bOnOff; }

protected:
  void Calculate(bool resetItem);
  void RenderItem(bool bFocus, int iPosX, int iPosY, CGUIListItem* pItem, int iStage);
  void RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText, bool bScroll );
  virtual void OnRight();
  virtual void OnLeft();
  virtual void OnDown();
  virtual void OnUp();
  void OnPageUp();
  void OnPageDown();
  bool ValidItem(int iX, int iY);

  int m_iSpinPosX;
  int m_iSpinPosY;

  int m_iItemHeightLow;
  int m_iItemWidthLow;
  int m_iTextureHeightLow;
  int m_iTextureWidthLow;

  int m_iItemHeightBig;
  int m_iItemWidthBig;
  int m_iTextureHeightBig;
  int m_iTextureWidthBig;

  bool m_bShowTexture;
  int m_iRowOffset;
  float m_fSmoothScrollOffset;
  int m_iItemHeight;
  int m_iItemWidth;
  int m_iSelect;
  int m_iCursorX;
  int m_iCursorY;
  int m_iRows;
  int m_iColumns;
  bool m_bScrollUp;
  bool m_bScrollDown;
  int m_iScrollCounter;
  wstring m_strSuffix;

  int m_iLastItem;
  int m_iTextureWidth;
  int m_iTextureHeight;
  int m_iThumbAlign;
  int m_iThumbXPos;
  int m_iThumbYPos;
  int m_iThumbWidth;
  int m_iThumbHeight;

  int m_iThumbXPosLow;
  int m_iThumbYPosLow;
  int m_iThumbWidthLow;
  int m_iThumbHeightLow;

  int m_iThumbXPosBig;
  int m_iThumbYPosBig;
  int m_iThumbWidthBig;
  int m_iThumbHeightBig;

  bool m_bHideFileNameLabel;

  CLabelInfo m_label;

  CGUISpinControl m_upDown;
  CGUIImage m_imgFolder;
  CGUIImage m_imgFolderFocus;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
  CScrollInfo m_scrollInfo;
};
#endif
