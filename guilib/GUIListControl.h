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

	DWORD									GetTextColor() const { return m_dwTextColor;};
	DWORD									GetTextColor2() const { return m_dwTextColor2;};
	DWORD									GetSelectedColor() const { return m_dwSelectedColor;};
	DWORD									GetSelectedColor2() const { return m_dwSelectedColor2;};
	const CStdString&			GetFontName() const { return m_pFont->GetFontName(); };
	const char*						GetFontName2() const { if (!m_pFont2) return ""; else return m_pFont2->GetFontName().c_str(); };
	DWORD									GetSpinWidth() const { return m_upDown.GetWidth()/2; };
	DWORD									GetSpinHeight() const { return m_upDown.GetHeight(); };
	const	CStdString&			GetTexutureUpName() const { return m_upDown.GetTexutureUpName(); };
	const	CStdString&			GetTexutureDownName() const { return m_upDown.GetTexutureDownName(); };
	const	CStdString&			GetTexutureUpFocusName() const { return m_upDown.GetTexutureUpFocusName(); };
	const	CStdString&			GetTexutureDownFocusName() const { return m_upDown.GetTexutureDownFocusName(); };
	DWORD									GetSpinTextColor() const { return m_upDown.GetTextColor();};
	DWORD									GetSpinX() const { return m_upDown.GetXPosition();};
	DWORD									GetSpinY() const { return m_upDown.GetYPosition();};
	DWORD									GetSpace() const { return m_iSpaceBetweenItems;};
	DWORD									GetItemHeight() const { return m_iItemHeight;	};
	DWORD									GetTextOffsetX() const { return m_iTextOffsetX;};
	DWORD									GetTextOffsetY() const { return m_iTextOffsetY;};
	DWORD									GetTextOffsetX2() const { return m_iTextOffsetX2;};
	DWORD									GetTextOffsetY2() const { return m_iTextOffsetY2;};
	DWORD									GetImageWidth() const { return m_iImageWidth;};
	DWORD									GetImageHeight() const { return m_iImageHeight;};
	const wstring&				GetSuffix() const { return m_strSuffix;};
	const CStdString			GetButtonFocusName() const { return m_imgButton.GetTexutureFocusName();};
	const CStdString			GetButtonNoFocusName() const { return m_imgButton.GetTexutureNoFocusName();};
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