#include "stdafx.h"
#include "GUIFadeLabelControl.h"
#include "GUIFontManager.h"
#include "../xbmc/utils/CharsetConverter.h"

CGUIFadeLabelControl::CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont, DWORD dwTextColor, DWORD dwTextAlign)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
{
  m_pFont=g_fontManager.GetFont(strFont);
  m_dwTextColor=dwTextColor;
  m_dwdwTextAlign=dwTextAlign;
	m_iCurrentLabel=0;
	scroll_pos = 0;
	iScrollX=0;
	iLastItem=-1;
	iFrames=0;
	iStartFrame=0;
	m_bFadeIn=false;
	m_iCurrentFrame=0;
	ControlType = GUICONTROL_FADELABEL;
}

CGUIFadeLabelControl::~CGUIFadeLabelControl(void)
{
}


void CGUIFadeLabelControl::Render()
{
	if (!IsVisible())	return;
	if (!m_pFont) return;
	if (m_vecLabels.size()==0) return;
	if (m_iCurrentLabel >= (int)m_vecLabels.size() ) m_iCurrentLabel=0;

	wstring strLabel=m_vecLabels[m_iCurrentLabel];
	CStdStringW strLabelUnicode;
	g_charsetConverter.stringCharsetToFontCharset(strLabel, strLabelUnicode);

  if ((int)m_vecLabels.size()==1)
  {
    DWORD iWidth=(DWORD)m_pFont->GetTextWidth(strLabelUnicode.c_str());
    if (iWidth < m_dwWidth)
    {
      m_pFont->DrawText( (float)m_iPosX,(float)m_iPosY,m_dwTextColor,strLabelUnicode.c_str());
      return;
    }
  }
	if (m_bFadeIn)
	{
		DWORD dwAlpha = (0xff/12) * m_iCurrentFrame;
		dwAlpha <<=24;
		dwAlpha += ( m_dwTextColor &0x00ffffff);
		m_pFont->DrawTextWidth((float)m_iPosX,(float)m_iPosY,dwAlpha,strLabelUnicode.c_str(),(float)m_dwWidth);

		m_iCurrentFrame++;
		if (m_iCurrentFrame >=12)
		{
			m_bFadeIn=false;
		}
	}
	else
	{
		if ( RenderText((float)m_iPosX, (float)m_iPosY, (float) m_dwWidth,m_dwTextColor, (WCHAR*) strLabelUnicode.c_str(), true ))
		{
			m_iCurrentLabel++;
			scroll_pos = 0;
			iScrollX=0;
			iLastItem=-1;
			iFrames=0;
			iStartFrame=0;
			m_bFadeIn=true;
			m_iCurrentFrame =0;
		}
	}
}


bool CGUIFadeLabelControl::CanFocus() const
{
  return false;
}


bool CGUIFadeLabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId()==GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      wstring strLabel = message.GetLabel();
			m_vecLabels.push_back(strLabel);
    }
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
			m_vecLabels.erase(m_vecLabels.begin(), m_vecLabels.end());
    }
		if (message.GetMessage() == GUI_MSG_LABEL_SET)
		{
			m_vecLabels.erase(m_vecLabels.begin(), m_vecLabels.end());
			m_vecLabels.push_back(message.GetLabel());
		}
  }
  return CGUIControl::OnMessage(message);
}

bool CGUIFadeLabelControl::RenderText(float fPosX, float fPosY, float fMaxWidth,DWORD dwTextColor, WCHAR* wszText,bool bScroll )
{
	if (!m_pFont) return false;
	bool bResult = false;
  float fTextHeight, fTextWidth;
  m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight);

	float fPosCX=fPosX;
	float fPosCY=fPosY;
	g_graphicsContext.Correct(fPosCX, fPosCY);
	if (fPosCX <0) fPosCX=0.0f;
	if (fPosCY <0) fPosCY=0.0f;
	if (fPosCY >g_graphicsContext.GetHeight()) fPosCY=(float)g_graphicsContext.GetHeight();

	D3DVIEWPORT8 newviewport,oldviewport;
	g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);
	float fHeight=60;
	if (fHeight+fPosCY >= g_graphicsContext.GetHeight() )
		fHeight = g_graphicsContext.GetHeight() - fPosCY -1;
	if (fHeight <= 0 || fMaxWidth < 5.0f) return true;

	newviewport.X      = (DWORD)fPosCX;
	newviewport.Y			 = (DWORD)fPosCY;
	newviewport.Width  = (DWORD)(fMaxWidth-5.0f);
	newviewport.Height = (DWORD)(fHeight);
	newviewport.MinZ   = 0.0f;
	newviewport.MaxZ   = 1.0f;
	g_graphicsContext.Get3DDevice()->SetViewport(&newviewport);


	// scroll
	WCHAR wszOrgText[1024];
	wcscpy(wszOrgText, wszText);
	do{
		m_pFont->GetTextExtent( wszOrgText, &fTextWidth,&fTextHeight);
		wcscat(wszOrgText, L" ");
	} while ( fTextWidth < fMaxWidth);

//	fMaxWidth+=50.0f;
	WCHAR szText[1024];

	if (iStartFrame > 25)
	{
		WCHAR wTmp[3];
		if (scroll_pos >= (int)wcslen(wszOrgText) )
			wTmp[0]=L' ';
		else
			wTmp[0]=wszOrgText[scroll_pos];
		wTmp[1]=0;
		float fWidth,fHeight;
		m_pFont->GetTextExtent(wTmp,&fWidth,&fHeight);
		if ( iScrollX >= fWidth)
		{
			++scroll_pos;
			if (scroll_pos > (int)wcslen(wszText) )
			{
				scroll_pos = 0;
			bResult=true;
			g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);

			return true;
			}
			iFrames=0;
			iScrollX=1;
		}
		else iScrollX++;

		int ipos=0;
		int iTextLength = (int)wcslen(wszOrgText);
		for (int i=0; i < iTextLength; i++)
		{
			if (i+scroll_pos < iTextLength)
				szText[i]=wszOrgText[i+scroll_pos];
			else
			{
				szText[i]=L' ';
				ipos++;
			}
			szText[i+1]=0;
		}
		if (fPosY >=0.0)
			m_pFont->DrawTextWidth(fPosX-iScrollX,fPosY,m_dwTextColor,szText,fMaxWidth);
		
	}
	else
	{
		iStartFrame++;
		if (fPosY >=0.0)
			m_pFont->DrawTextWidth(fPosX,fPosY,m_dwTextColor,wszText,fMaxWidth);
	}
  
	g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
	return bResult;
}
