/*!
	\file GUILabelControl.h
	\brief 
	*/

#ifndef GUILIB_GUILABELCONTROL_H
#define GUILIB_GUILABELCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "stdstring.h"
using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUILabelControl :
  public CGUIControl
{
public:
  CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont,const wstring& strLabel, DWORD dwTextColor, DWORD dwDisabledColor, DWORD dwTextAlign, bool bHasPath);
  virtual ~CGUILabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);

	DWORD							GetTextColor() const { return m_dwTextColor;}
	DWORD							GetDisabledColor() const { return m_dwDisabledColor;}
	const CStdString& GetFontName() const { return m_pFont->GetFontName(); }
	const wstring			GetLabel() const { return m_strLabel; }
	void	SetText(CStdString aLabel);
	void	ShowCursor(bool bShow = true);
	void	SetCursorPos(int iPos);
	int		GetCursorPos() const { return m_iCursorPos;};

	DWORD             m_dwTextAlign;

protected:
	void							ShortenPath();
protected:
  CGUIFont*								m_pFont;
  wstring		              m_strLabel;
  DWORD                   m_dwTextColor;
	bool										m_bHasPath;
  DWORD		m_dwDisabledColor;
  bool		m_bShowCursor;
  int m_iCursorPos;
  DWORD m_dwCounter;
};
#endif
