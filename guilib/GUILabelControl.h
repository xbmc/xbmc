#ifndef GUILIB_GUILABELCONTROL_H
#define GUILIB_GUILABELCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "stdstring.h"
using namespace std;

class CGUILabelControl :
  public CGUIControl
{
public:
  CGUILabelControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont,const wstring& strLabel, DWORD dwTextColor,DWORD dwTextAlign);
  virtual ~CGUILabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
	DWORD							GetTextColor() const { return m_dwTextColor;};
	DWORD							GetAlignment() const { return m_dwdwTextAlign;};
	const CStdString& GetFontName() const { return m_pFont->GetFontName(); };
	const wstring			GetLabel() const { return m_strLabel; };
protected:
  CGUIFont*								m_pFont;
  wstring                 m_strLabel;
  DWORD                   m_dwTextColor;
  DWORD                   m_dwdwTextAlign;
};
#endif