#ifndef GUILIB_GUILISTCONTROL_H
#define GUILIB_GUILISTCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "guiimage.h"
#include "guispincontrol.h"
#include "guiButtonControl.h"
#include "guiListItem.h"
#include <vector>
#include "stdstring.h"
using namespace std;


class CGUIListControl : public CGUIControl
{
public:
  CGUIListControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                  const CStdString& strFontName, 
                  DWORD dwSpinWidth,DWORD dwSpinHeight,
                  const CStdString& strUp, const CStdString& strDown, 
                  const CStdString& strUpFocus, const CStdString& strDownFocus, 
                  DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                  const CStdString& strFont, DWORD dwTextColor,DWORD dwSelectedColor,
                  const CStdString& strButton, const CStdString& strButtonFocus);
  virtual ~CGUIListControl(void);
  virtual void 					Render();
  virtual void 					OnKey(const CKey& key) ;
  virtual bool 					OnMessage(CGUIMessage& message);

  virtual void 					AllocResources() ;
  virtual void 					FreeResources() ;
  void         					SetScrollySuffix(const CStdString& wstrSuffix);
	void				 					SetTextOffsets(int iXoffset, int iYOffset, int iXoffset2, int iYOffset2);
	void				 					SetImageDimensions(int iWidth, int iHeight);
	void									SetItemHeight(int iHeight);
	void									SetSpace(int iHeight);
	void									SetFont2(const CStdString& strFont);
	void									SetColors2(DWORD dwTextColor, DWORD dwSelectedColor);

protected:
   
  void         					RenderText(float fPosX, float fPosY,float fMaxWidth, DWORD dwTextColor, WCHAR* wszText,bool bScroll );
  void         					OnRight();
  void         					OnLeft();
  void         					OnDown();
  void         					OnUp();
  void         					OnPageUp();
  void         					OnPageDown();
	int										m_iSpaceBetweenItems;
  int                   m_iOffset;
  int                   m_iItemsPerPage;
  int                   m_iItemHeight;
  int                   m_iSelect;
	int										m_iCursorY;
	int										m_iTextOffsetX;
	int										m_iTextOffsetY;
	int										m_iTextOffsetX2;
	int										m_iTextOffsetY2;
	int										m_iImageWidth;
	int										m_iImageHeight;
  DWORD                 m_dwTextColor,m_dwTextColor2;
  DWORD                 m_dwSelectedColor,m_dwSelectedColor2;
  CGUIFont*             m_pFont;
	CGUIFont*             m_pFont2;
  CGUISpinControl       m_upDown;
  CGUIButtonControl     m_imgButton;
  wstring               m_strSuffix;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
};
#endif