#ifndef GUILIB_GUITHUMBNAILCONTROL_H
#define GUILIB_GUITHUMBNAILCONTROL_H

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

class CGUIThumbnailPanel :
  public CGUIControl
{
public:
  CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                    const CStdString& strFontName, 
                    const CStdString& strImageIcon,
                    const CStdString& strImageIconFocus,
                    DWORD dwitemWidth, DWORD dwitemHeight,
                    DWORD dwSpinWidth,DWORD dwSpinHeight,
                    const CStdString& strUp, const CStdString& strDown, 
                    const CStdString& strUpFocus, const CStdString& strDownFocus, 
                    DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                    const CStdString& strFont, DWORD dwTextColor,DWORD dwSelectedColor);
  virtual ~CGUIThumbnailPanel(void);
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);

  virtual void AllocResources() ;
  virtual void FreeResources() ;
  void         SetScrollySuffix(CStdString wstrSuffix);
	void				 SetTextureDimensions(int iWidth, int iHeight);
  void         SetThumbDimensions(int iXpos, int iYpos,int iWidth, int iHeight);
  void         GetThumbDimensions(int& iXpos, int& iYpos,int& iWidth, int& iHeight);

	DWORD									GetTextColor() const { return m_dwTextColor;};
	DWORD									GetSelectedColor() const { return m_dwSelectedColor;};
	const CStdString&			GetFontName() const { return m_pFont->GetFontName(); };
	DWORD									GetSpinWidth() const { return m_upDown.GetWidth()/2; };
	DWORD									GetSpinHeight() const { return m_upDown.GetHeight(); };
	const	CStdString&			GetTexutureUpName() const { return m_upDown.GetTexutureUpName(); };
	const	CStdString&			GetTexutureDownName() const { return m_upDown.GetTexutureDownName(); };
	const	CStdString&			GetTexutureUpFocusName() const { return m_upDown.GetTexutureUpFocusName(); };
	const	CStdString&			GetTexutureDownFocusName() const { return m_upDown.GetTexutureDownFocusName(); };
	DWORD									GetSpinTextColor() const { return m_upDown.GetTextColor();};
	DWORD									GetSpinX() const { return m_upDown.GetXPosition();};
	DWORD									GetSpinY() const { return m_upDown.GetYPosition();};
	DWORD									GetTextureWidth() const { return m_iTextureWidth;};
	DWORD									GetTextureHeight() const { return m_iTextureHeight;};
	const CStdString			GetFocusName() const { return m_imgFolderFocus.GetFileName();};
	const CStdString			GetNoFocusName() const { return m_imgFolder.GetFileName();};
	DWORD									GetItemWidth() const { return m_iItemWidth;};
	DWORD									GetItemHeight() const { return m_iItemHeight;};
	void                  SetItemWidth(DWORD dwWidth);
	void                  SetItemHeight(DWORD dwHeight);
  bool                  IsTextureShown() const { return m_bShowTexture;};
  void                  ShowTexture(bool bOnoff);
protected:
	void				 RenderItem(bool bFocus,DWORD dwPosX, DWORD dwPosY, CGUIListItem* pItem);
  void         RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText,bool bScroll );
  void         OnRight();
  void         OnLeft();
  void         OnDown();
  void         OnUp();
  void         OnPageUp();
  void         OnPageDown();
  bool         ValidItem(int iX, int iY);
  bool                  m_bShowTexture;
  int                   m_iOffset;
  int                   m_iItemHeight;
  int                   m_iItemWidth;
  int                   m_iSelect;
	int										m_iCursorX;
	int										m_iCursorY;
  int                   m_iRows;
  int                   m_iColumns;
	bool									m_bScrollUp;
	bool									m_bScrollDown;
	int										m_iScrollCounter;
  wstring               m_strSuffix;
  DWORD                 m_dwTextColor;
  DWORD                 m_dwSelectedColor;
	int										m_iLastItem;
	int										m_iTextureWidth;
	int										m_iTextureHeight;
  int                   m_iThumbXPos;
  int                   m_iThumbYPos;
  int                   m_iThumbWidth;
  int                   m_iThumbHeight;
  CGUIFont*             m_pFont;
  CGUISpinControl       m_upDown;
  CGUIImage             m_imgFolder;
  CGUIImage             m_imgFolderFocus;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
};
#endif