#include "stdafx.h"
#include "guilabelcontrol.h"
#include "guifontmanager.h"


CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont,const wstring& strLabel, DWORD dwTextColor, DWORD dwTextAlign, bool bHasPath)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
{
  m_strLabel=strLabel;
  m_pFont=g_fontManager.GetFont(strFont);
  m_dwTextColor=dwTextColor;
  m_dwdwTextAlign=dwTextAlign;
	m_bHasPath = bHasPath;
}

CGUILabelControl::~CGUILabelControl(void)
{
}


void CGUILabelControl::Render()
{
	if (!IsVisible() ) return;
  if (m_pFont)
  {
    m_pFont->DrawText((float)m_dwPosX, (float)m_dwPosY,m_dwTextColor,m_strLabel.c_str(),m_dwdwTextAlign); 
  }
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}


bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId()==GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_strLabel = message.GetLabel();
			if ( m_bHasPath )
				ShortenPath();
      return true;
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUILabelControl::ShortenPath()
{
	if ( m_dwWidth <= 0 )
		return;
	if ( m_strLabel.size() <= 0 )
		return;

  WCHAR wszText[1024];
	float fTextHeight,fTextWidth;
	char cDelim = '\0';
	int nGreaterDelim, nPos;

	swprintf(wszText,L"%s", m_strLabel.c_str() );
	m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight);

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

		swprintf(wszText,L"%s", m_strLabel.c_str() );
		m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight );
	}
}
