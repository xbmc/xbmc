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
		D3DCOLOR dwPenColor1, D3DCOLOR dwPenColor2, D3DCOLOR dwPenColor3, D3DCOLOR dwPenColor4);

	virtual ~CGUIConsoleControl(void);
  
	virtual void Render();
	virtual void OnAction(const CAction &action);

	virtual void PreAllocResources();
	virtual void AllocResources();
	virtual void FreeResources();

	DWORD	GetPenColor(INT nPaletteIndex)
	{
		return nPaletteIndex<(INT)m_palette.size() ? m_palette[nPaletteIndex] : 0xFF808080;
	};

	LPCSTR	GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };

	void	Clear();
	void	Write(CStdString& aString, INT nPaletteIndex = 0);

protected:

	void	AddLine(CStdString& aString, DWORD aColour);
	void	WriteString(CStdString& aString, DWORD aColour);

protected:

	struct Line
	{
		CStdString text;
		DWORD colour;
	};

	typedef vector<Line> LINEVECTOR;
	LINEVECTOR		m_lines;

	typedef vector<D3DCOLOR> PALETTE;
	PALETTE			m_palette;

	INT				m_nMaxLines;
	DWORD			m_dwLineCounter;
	DWORD			m_dwFrameCounter;

	FLOAT			m_fFontHeight;
	CGUIFont*		m_pFont;
};
#endif
