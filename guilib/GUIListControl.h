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
                  const CStdString& strImageIcon,
                  DWORD dwSpinWidth,DWORD dwSpinHeight,
                  const CStdString& strUp, const CStdString& strDown, 
                  const CStdString& strUpFocus, const CStdString& strDownFocus, 
                  DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                  const CStdString& strFont, DWORD dwTextColor,
                  const CStdString& strButton, const CStdString& strButtonFocus);
  virtual ~CGUIListControl(void);
  virtual void Render();
  virtual void OnKey(const CKey& key) ;
  virtual bool OnMessage(CGUIMessage& message);

  virtual void AllocResources() ;
  virtual void FreeResources() ;
  void         SetScrollySuffix(CStdString wstrSuffix);

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