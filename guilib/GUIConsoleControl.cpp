#include "stdafx.h"
#include "guiConsoleControl.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"
#include "..\xbmc\Util.h"
#include "..\xbmc\Utils\Log.h"
#include "..\xbmc\utils\CharsetConverter.h"

#define CONSOLE_LINE_SPACING 1.0f

CGUIConsoleControl::CGUIConsoleControl(DWORD dwParentID, DWORD dwControlId, 
							   int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
							   const CStdString& strFontName,
							   D3DCOLOR dwNormalColor, D3DCOLOR dwActionColor, D3DCOLOR dwSpecialColor)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
{
	m_dwNormalColor		= dwNormalColor;
	m_dwActionColor		= dwActionColor; 
	m_dwSpecialColor	= dwSpecialColor;
	m_pFont				= g_fontManager.GetFont(strFontName);

	FLOAT fTextW;
	if (m_pFont)
	{
		m_pFont->GetTextExtent(L"X", &fTextW, &m_fFontHeight);
	}

	m_nMaxLines			= 10;
	m_dwLineCounter		= 0;
	m_dwFrameCounter	= 0;

	Line line;
	line.text	= "";
	line.colour = 0xFF00FF00;

	for(int i=0; i<m_nMaxLines; i++)
	{
		line.text.Format("Test line No. %d",i);
		m_lines.push_back(line);
	}

	ControlType = GUICONTROL_CONSOLE;
}

CGUIConsoleControl::~CGUIConsoleControl(void)
{
}

void CGUIConsoleControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();
}

void CGUIConsoleControl::AllocResources()
{
	CGUIControl::AllocResources(); 
}

void CGUIConsoleControl::FreeResources()
{
	CGUIControl::FreeResources();
}

void CGUIConsoleControl::OnAction(const CAction &action)
{
	CGUIControl::OnAction(action);
}

void CGUIConsoleControl::Render()
{
	if (!IsVisible() || !m_pFont)
	{
		return;
	}

	m_dwFrameCounter++;

	if (m_dwFrameCounter%250==0)
	{
		CStdString strDebug;
		strDebug.Format("DEBUG %u",m_dwLineCounter);
		AddLine(strDebug,0xFFFF0000);
	}

	FLOAT fTextX	= (FLOAT) m_iPosX;
	FLOAT fTextY	= (FLOAT) m_iPosY;

	CStdStringW strText;
	
	for(int nLine=0; nLine<m_nMaxLines; nLine++)
	{
		INT nIndex = (m_dwLineCounter + nLine) % m_nMaxLines;

		Line& line = m_lines[nIndex];
		g_charsetConverter.stringCharsetToFontCharset(line.text, strText);

		m_pFont->DrawText(fTextX, fTextY, line.colour, (LPWSTR) strText.c_str());

		fTextY += m_fFontHeight + CONSOLE_LINE_SPACING;
	}	
}

void CGUIConsoleControl::AddLine(CStdString& aString, DWORD aColour)
{
	Line line;
	line.text = aString;
	line.colour = aColour;

	// determine which line appears at the bottom of the console
	INT nIndex = m_dwLineCounter % m_nMaxLines;
	
	m_lines[nIndex] = line;

	m_dwLineCounter++;
}
