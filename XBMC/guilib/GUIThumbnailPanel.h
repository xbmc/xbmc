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
  enum LABEL_STATE { SHOW_ALL = 0, HIDE_FILES, HIDE_FOLDERS, HIDE_ALL };

  CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                     const CImage& imageIcon,
                     const CImage& imageIconFocus,
                     float spinWidth, float spinHeight,
                     const CImage& textureUp, const CImage& textureDown,
                     const CImage& textureUpFocus, const CImage& textureDownFocus,
                     const CLabelInfo& spinInfo, float iSpinX, float iSpinY,
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
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetPulseOnSelect(bool pulse);
  void SetScrollySuffix(const CStdString &strSuffix);
  void SetThumbAlign(int align);
  void SetThumbDimensions(float posX, float posY, float width, float height);
  void SetItemWidth(float width);
  void SetItemHeight(float height);
  void SetTextureWidthBig(float textureWidthBig) { m_textureWidthBig = textureWidthBig;};
  void SetTextureHeightBig(float textureHeightBig){ m_textureHeightBig = textureHeightBig;};
  void SetItemWidthBig(float itemWidthBig){ m_itemWidthBig = itemWidthBig;};
  void SetItemHeightBig(float itemHeightBig) { m_itemHeightBig = itemHeightBig;};

  void SetTextureWidthLow(float textureWidthLow) { m_textureWidthLow = textureWidthLow;};
  void SetTextureHeightLow(float textureHeightLow){ m_textureHeightLow = textureHeightLow;};
  void SetItemWidthLow(float itemWidthLow){ m_itemWidthLow = itemWidthLow;};
  void SetItemHeightLow(float itemHeightLow) { m_itemHeightLow = itemHeightLow;};

  void ShowBigIcons(bool bOnOff);
  void SetThumbDimensionsLow(float posX, float posY, float width, float height) { m_thumbXPosLow = posX; m_thumbYPosLow = posY; m_thumbWidthLow = width; m_thumbHeightLow = height;};
  void SetThumbDimensionsBig(float posX, float posY, float width, float height) { m_thumbXPosBig = posX; m_thumbYPosBig = posY; m_thumbWidthBig = width; m_thumbHeightBig = height;};
  virtual bool OnMouseOver();
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMouseDoubleClick(DWORD dwButton);
  virtual bool OnMouseWheel();
  virtual bool HitTest(float posX, float posY) const;
  virtual CStdString GetDescription() const;

  void SetLabelState(LABEL_STATE state) { m_labelState = state; };

  void SetPageControlVisible(bool visible) { m_pageControlVisible = visible; }
  void SetPageControl(DWORD id);
  void SetAspectRatio(CGUIImage::GUIIMAGE_ASPECT_RATIO ratio) { m_aspectRatio = ratio; };
  virtual void SaveStates(vector<CControlState> &states);

protected:
  bool ScrollDown();
  void ScrollUp();
  bool SelectItemFromPoint(float posX, float posY);
  void GetOffsetFromPage();
  void SetSelectedItem(int iItem);
  void Calculate(bool resetItem);
  void RenderItem(bool bFocus, float posX, float posY, CGUIListItem* pItem, int iStage);
  void RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText, bool bScroll );
  virtual void OnRight();
  virtual void OnLeft();
  virtual void OnDown();
  virtual void OnUp();
  void OnPageUp();
  void OnPageDown();
  bool ValidItem(int iX, int iY);
  void UpdatePageControl();

  float m_spinPosX;
  float m_spinPosY;

  float m_itemHeightLow;
  float m_itemWidthLow;
  float m_textureHeightLow;
  float m_textureWidthLow;

  float m_itemHeightBig;
  float m_itemWidthBig;
  float m_textureHeightBig;
  float m_textureWidthBig;

  int m_iRowOffset;
  float m_fSmoothScrollOffset;
  float m_itemHeight;
  float m_itemWidth;
  int m_iSelect;
  int m_iCursorX;
  int m_iCursorY;
  int m_iRows;
  int m_iColumns;
  bool m_bScrollUp;
  bool m_bScrollDown;
  int m_iScrollCounter;
  string m_strSuffix;

  int m_iLastItem;
  float m_textureWidth;
  float m_textureHeight;

  float m_thumbXPos;
  float m_thumbYPos;
  float m_thumbWidth;
  float m_thumbHeight;

  float m_thumbXPosLow;
  float m_thumbYPosLow;
  float m_thumbWidthLow;
  float m_thumbHeightLow;

  float m_thumbXPosBig;
  float m_thumbYPosBig;
  float m_thumbWidthBig;
  float m_thumbHeightBig;

  int m_thumbAlign;

  LABEL_STATE m_labelState;
  bool m_pageControlVisible;
  bool m_usingBigIcons;
  CGUIImage::GUIIMAGE_ASPECT_RATIO m_aspectRatio;

  CLabelInfo m_label;

  CGUISpinControl m_upDown;
  CGUIImage m_imgFolder;
  CGUIImage m_imgFolderFocus;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
  CScrollInfo m_scrollInfo;

  DWORD m_pageControl;
};
#endif
