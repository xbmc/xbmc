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
#include <string>
using namespace std;

class CGUIThumbnailPanel :
  public CGUIControl
{
public:
  CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                    const string& strFontName, 
                    const string& strImageIcon,
                    const string& strImageIconFocus,
                    DWORD dwitemWidth, DWORD dwitemHeight,
                    DWORD dwSpinWidth,DWORD dwSpinHeight,
                    const string& strUp, const string& strDown, 
                    const string& strUpFocus, const string& strDownFocus, 
                    DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                    const string& strFont, DWORD dwTextColor);
  virtual ~CGUIThumbnailPanel(void);
  virtual void Render();
  virtual void OnKey(const CKey& key) ;
  virtual bool OnMessage(CGUIMessage& message);

  virtual void AllocResources() ;
  virtual void FreeResources() ;
  void         SetScrollySuffix(string wstrSuffix);
protected:
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