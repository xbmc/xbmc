#include "stdafx.h"
#include "guiEditControl.h"
#include "guifontmanager.h"
#include "../xbmc/utils/CharsetConverter.h"

CGUIEditControl::CGUIEditControl(DWORD dwParentID, DWORD dwControlId,
	int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, 
	const CStdString& strFont, const wstring& strLabel, 
	DWORD dwTextColor, DWORD dwDisabledColor)
:CGUILabelControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight, strFont, strLabel, dwTextColor, dwDisabledColor, 0, false)
{
	ControlType = GUICONTROL_EDIT;
	ShowCursor(true);
}

CGUIEditControl::~CGUIEditControl(void)
{
}

void CGUIEditControl::OnKeyPress(WORD wKeyId)
{
	if (wKeyId >= KEY_VKEY && wKeyId < KEY_ASCII)
	{	
		// input from the keyboard (vkey, not ascii)
		BYTE b = wKeyId & 0xFF;
		if (b == 0x25 && m_iCursorPos>0)
		{
			// left
			m_iCursorPos--;
		}
		if (b == 0x27 && m_iCursorPos<(int)m_strLabel.length())
		{
			// right
			m_iCursorPos++;
		}
	}
	else if (wKeyId >= KEY_ASCII)
	{
		// input from the keyboard
		char ch = wKeyId & 0xFF;
		switch (ch)
		{
		case 27:	// escape
			break;
		case 10:	// enter
			break;
		case 8:		// backspace or delete??
			if (m_iCursorPos>0)
			{
				m_strLabel.erase(m_iCursorPos-1,1);
				m_iCursorPos--;
			}
			break;
		default:	// use character input
			m_strLabel.insert( m_strLabel.begin() + m_iCursorPos, (wchar_t)ch);
			m_iCursorPos++;
			break;
		}
	}
}
