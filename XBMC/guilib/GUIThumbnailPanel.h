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
                    const CStdString& strFont, DWORD dwTextColor);
  virtual ~CGUIThumbnailPanel(void);
  virtual void Render();
  virtual void OnKey(const CKey& key) ;
  virtual bool OnMessage(CGUIMessage& message);

  virtual void AllocResources() ;
  virtual void FreeResources() ;
  void         SetScrollySuffix(CStdString wstrSuffix);
protected:
	void				 RenderItem(bool bFocus,DWORD dwPosX, DWORD dwPosY, CGUIListItem* pItem);
  void         RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText,bool bScroll );
  void         OnRight();
  void         OnLeft();
  void         OnDown();
  void         OnUp();
  bool                  ValidItem(int iX, int iY);
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
  CGUIFont*             m_pFont;
  CGUISpinControl       m_upDown;
  CGUIImage             m_imgFolder;
  CGUIImage             m_imgFolderFocus;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
};
#endif