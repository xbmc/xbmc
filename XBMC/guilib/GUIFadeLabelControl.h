#ifndef GUILIB_GUIFADELABELCONTROL_H
#define GUILIB_GUIFADELABELCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "stdstring.h"
#include <vector>
using namespace std;

class CGUIFadeLabelControl :  public CGUIControl
{
public:
  CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont, DWORD dwTextColor,DWORD dwTextAlign);
  virtual ~CGUIFadeLabelControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
protected:
	bool	RenderText(float fPosX, float fPosY, float fMaxWidth,DWORD dwTextColor, WCHAR* wszText,bool bScroll );
  CGUIFont*								m_pFont;
  vector<wstring>         m_vecLabels;
  DWORD                   m_dwTextColor;
  DWORD                   m_dwdwTextAlign;
	int											m_iCurrentLabel;
	int scroll_pos;
	int iScrollX;
	int iLastItem;
	int iFrames;
	int iStartFrame;
	bool	m_bFadeIn;
	int		m_iCurrentFrame;
};
#endif