#include "stdafx.h"
#include "guilabelcontrol.h"
#include "guifontmanager.h"
#include "../xbmc/utils/CharsetConverter.h"

CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont,const wstring& strLabel, DWORD dwTextColor, DWORD dwDisabledColor, DWORD dwTextAlign, bool bHasPath)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
{
  m_strLabel=strLabel;
  m_pFont=g_fontManager.GetFont(strFont);
  m_dwTextColor=dwTextColor;
  m_dwTextAlign=dwTextAlign;
	m_bHasPath = bHasPath;
  m_dwDisabledColor = dwDisabledColor;
  m_bShowCursor = false;
  m_iCursorPos = 0;
  m_dwCounter = 0;
	ControlType = GUICONTROL_LABEL;
}

CGUILabelControl::~CGUILabelControl(void)
{
}

void CGUILabelControl::ShowCursor(bool bShow)
{
	m_bShowCursor = bShow;
}

void CGUILabelControl::SetCursorPos(int iPos)
{
	if (iPos > (int)m_strLabel.length()) iPos = m_strLabel.length();
	if (iPos < 0) iPos = 0;
	m_iCursorPos = iPos;
}

void CGUILabelControl::Render()
{
	if (!IsVisible() )
	{
		return;
	}

	if (m_pFont)
	{
		CStdStringW strLabelUnicode;
		g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);

		if (IsDisabled())
		{
			m_pFont->DrawText((float)m_iPosX, (float)m_iPosY,m_dwDisabledColor,m_strLabel.c_str(),m_dwTextAlign,(float)m_dwWidth);
		}
		else
		{
			if (m_bShowCursor)
			{	// show the cursor...
				if ((++m_dwCounter % 50) > 25)
				{
					strLabelUnicode.Insert(m_iCursorPos,L"|");
				}
			}
	
			m_pFont->DrawText((float)m_iPosX, (float)m_iPosY,m_dwTextColor,strLabelUnicode.c_str(),m_dwTextAlign, (float)m_dwWidth);
		}
	}
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}

void CGUILabelControl::SetText(CStdString aLabel)
{
  WCHAR wszText[1024];
	swprintf(wszText,L"%S",aLabel.c_str());	
	m_strLabel = wszText;
}

bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
	if ( message.GetControlId()==GetID() )
	{
		if (message.GetMessage() == GUI_MSG_LABEL_SET)
		{
			m_strLabel = message.GetLabel();
			
			if ( m_bHasPath )
			{
				ShortenPath();
			}
			return true;
		}
	}

	return CGUIControl::OnMessage(message);
}

void CGUILabelControl::ShortenPath()
{
	if (!m_pFont)
		return;
	if ( m_dwWidth <= 0 )
		return;
	if ( m_strLabel.size() <= 0 )
		return;

	CStdStringW strLabelUnicode;
	g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);

	float fTextHeight,fTextWidth;
	char cDelim = '\0';
	int nGreaterDelim, nPos;

	m_pFont->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth,&fTextHeight);

	if ( fTextWidth <= (m_dwWidth) )
		return;

	nPos = m_strLabel.find_last_of( '\\' );
	if ( nPos >= 0 )
		cDelim = '\\';
	else
	{
		int nPos = m_strLabel.find_last_of( '/' );
		if ( nPos >= 0 )
			cDelim = '/';
	}
	if ( cDelim == '\0' )
		return;

	while ( fTextWidth > m_dwWidth )
	{
		nPos = m_strLabel.find_last_of( cDelim, nPos );
		nGreaterDelim = nPos;
		if ( nPos >= 0 )
			nPos = m_strLabel.find_last_of( cDelim, nPos - 1 ); 
		else
			break;

		if ( nPos < 0 )
			break;
		
		if ( nGreaterDelim > nPos )
		{
			m_strLabel.replace( nPos+1, nGreaterDelim - nPos - 1, L"..." );
		}

		m_pFont->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth,&fTextHeight );
	}
}
