/*!
	\file GUIConsoleControl.h
	\brief 
	*/

#ifndef GUILIB_GUIConsoleControl_H
#define GUILIB_GUIConsoleControl_H

#pragma once
#include "gui3D.h"
#include "guiControl.h"
#include "guiMessage.h"
#include "guiFont.h"
#include "stdString.h"

#include <vector>

using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUIConsoleControl : public CGUIControl
{
public:

	CGUIConsoleControl(DWORD dwParentID, DWORD dwControlId,
		int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, 
		const CStdString& strFontName,
		D3DCOLOR dwNormalColor, D3DCOLOR dwActionColor, D3DCOLOR dwSpecialColor);

	virtual ~CGUIConsoleControl(void);
  
	virtual void Render();
	virtual void OnAction(const CAction &action);

	virtual void PreAllocResources();
	virtual void AllocResources();
	virtual void FreeResources();

	DWORD	GetActionColor()	const { return m_dwActionColor;};
	DWORD	GetNormalColor()	const { return m_dwNormalColor;};
	DWORD	GetSpecialColor()	const { return m_dwSpecialColor;};

	LPCSTR	GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };

	void	Write(CStdString& aString, DWORD aColour);

protected:

	void	AddLine(CStdString& aString, DWORD aColour);

protected:

	struct Line
	{
		CStdString text;
		DWORD colour;
	};

	typedef vector<Line> LINEVECTOR;
	LINEVECTOR		m_lines;

	INT				m_nMaxLines;
	DWORD			m_dwLineCounter;
	DWORD			m_dwFrameCounter;

	FLOAT			m_fFontHeight;

	D3DCOLOR		m_dwNormalColor;
	D3DCOLOR		m_dwActionColor;
	D3DCOLOR		m_dwSpecialColor;

	CGUIFont*		m_pFont;
};
#endif
