/*!
	\file GUITextBox.h
	\brief 
	*/

#ifndef GUILIB_GUITEXTBOX_H
#define GUILIB_GUITEXTBOX_H

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


/*!
	\ingroup controls
	\brief 
	*/
class CGUITextBox : public CGUIControl
{
public:
  CGUITextBox(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                  const CStdString& strFontName, 
                  DWORD dwSpinWidth,DWORD dwSpinHeight,
                  const CStdString& strUp, const CStdString& strDown, 
                  const CStdString& strUpFocus, const CStdString& strDownFocus, 
                  DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                  const CStdString& strFont, DWORD dwTextColor);
  virtual ~CGUITextBox(void);
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);

	virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
	DWORD									GetTextColor() const { return m_dwTextColor;};
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
	void				 SetText(const wstring &strText);
protected:
  void         OnRight();
  void         OnLeft();
  void         OnDown();
  void         OnUp();
  void         OnPageUp();
  void         OnPageDown();

  int                   m_iOffset;
  int                   m_iItemsPerPage;
  int                   m_iItemHeight;
	int                   m_iMaxPages;
  DWORD                 m_dwTextColor;
  CGUIFont*             m_pFont;
  CGUISpinControl       m_upDown;
  vector<CGUIListItem> m_vecItems;
  typedef vector<CGUIListItem> ::iterator ivecItems;
};
#endif
