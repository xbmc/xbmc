#include "stdafx.h"
#include "GUIEditControl.h"
#include "../xbmc/util.h"

CGUIEditControl::CGUIEditControl(DWORD dwParentID, DWORD dwControlId,
	int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, 
	const CStdString& strFont, const wstring& strLabel, 
	DWORD dwTextColor, DWORD dwDisabledColor)
:CGUILabelControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight, strFont, strLabel, dwTextColor, dwDisabledColor, 0, false)
{
	ControlType = GUICONTROL_EDIT;
	m_pObserver = NULL;
	m_iOriginalPosX = iPosX;
	ShowCursor(true);
}

CGUIEditControl::~CGUIEditControl(void)
{
}

void CGUIEditControl::SetObserver(IEditControlObserver* aObserver)
{
	m_pObserver = aObserver;
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
			case 27:
			{	// escape
				m_strLabel.clear();
				m_iCursorPos = 0;
				break;
			}
			case 10:
			{
				// enter
				if (m_pObserver)
				{
					CStdString strLineOfText;
					CUtil::Unicode2Ansi(m_strLabel,strLineOfText);
					m_strLabel.clear();
					m_iCursorPos = 0;
					m_pObserver->OnEditTextComplete(strLineOfText);
				}
				break;
			}
			case 8:
			{
				// backspace or delete??
				if (m_iCursorPos>0)
				{
					m_strLabel.erase(m_iCursorPos-1,1);
					m_iCursorPos--;
				}
				break;
			}
			default:
			{
				// use character input
				m_strLabel.insert( m_strLabel.begin() + m_iCursorPos, (wchar_t)ch);
				m_iCursorPos++;
				break;
			}
		}
	}

	RecalcLabelPosition();
}

void CGUIEditControl::RecalcLabelPosition()
{
	INT nMaxWidth = (INT) m_dwWidth - 8;

	FLOAT fTextWidth, fTextHeight;

	CStdStringW strTempLabel = m_strLabel;
	CStdStringW strTempPart  = strTempLabel.Mid(0,m_iCursorPos);

	m_pFont->GetTextExtent( strTempPart.c_str(), &fTextWidth, &fTextHeight );

	// if skinner forgot to set height :p
	if (m_dwHeight == 0)
	{
		// store font height
		m_dwHeight = (DWORD) fTextHeight;
	}

	// if text accumulated is greater than width allowed
	if ((INT)fTextWidth>nMaxWidth)
	{
		// move the position of the label to the left (outside of the viewport)
		m_iPosX = (m_iOriginalPosX+nMaxWidth)-(INT)fTextWidth;
	}
	else
	{
		// otherwise use original position
		m_iPosX = m_iOriginalPosX;
	}
}

void CGUIEditControl::Render()
{
	if (IsVisible())
	{
		// we can only perform view port operations if we have an area to display
		if (m_dwHeight>0 && m_dwWidth>0)
		{
			D3DVIEWPORT8 newviewport,oldviewport;
			g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);

			newviewport.X		= (DWORD)m_iOriginalPosX;
			newviewport.Y		= (DWORD)m_iPosY;
			newviewport.Width	= m_dwWidth;
			newviewport.Height	= m_dwHeight;
			newviewport.MinZ	= 0.0f;
			newviewport.MaxZ	= 1.0f;
			
			g_graphicsContext.Get3DDevice()->SetViewport(&newviewport);

			CGUILabelControl::Render();

			g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
		}
		else
		{
			// use default rendering until we have recalculated label position
			CGUILabelControl::Render();
		}
	}
}