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
#include <string>
using namespace std;


class CGUIListControl : public CGUIControl
{
public:
  CGUIListControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                  const string& strFontName, 
                  const string& strImageIcon,
                  DWORD dwSpinWidth,DWORD dwSpinHeight,
                  const string& strUp, const string& strDown, 
                  const string& strUpFocus, const string& strDownFocus, 
                  DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                  const string& strFont, DWORD dwTextColor,
                  const string& strButton, const string& strButtonFocus);
  virtual ~CGUIListControl(void);
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
  int                   m_iOffset;
  int                   m_iItemsPerPage;
  int                   m_iItemHeight;
  int                   m_iSelect;
	int										m_iCursorY;
  DWORD                 m_dwTextColor;
  CGUIFont*             m_pFont;
  CGUISpinControl       m_upDown;
  CGUIImage             m_imgFolder;
  CGUIButtonControl     m_imgButton;
  wstring               m_strSuffix;
  vector<CGUIListItem*> m_vecItems;
  typedef vector<CGUIListItem*> ::iterator ivecItems;
};
#endif