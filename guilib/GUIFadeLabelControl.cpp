#include "include.h"
#include "GUIFadeLabelControl.h"
#include "GUIFontManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIFadeLabelControl::CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont, DWORD dwTextColor, DWORD dwTextAlign)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_pFont = g_fontManager.GetFont(strFont);
  m_dwTextColor = dwTextColor;
  m_dwTextAlign = dwTextAlign;
  m_iCurrentLabel = 0;
  m_bFadeIn = false;
  m_iCurrentFrame = 0;
  ControlType = GUICONTROL_FADELABEL;
}

CGUIFadeLabelControl::~CGUIFadeLabelControl(void)
{
}

void CGUIFadeLabelControl::SetInfo(const vector<int> &vecInfo)
{
  m_vecInfo = vecInfo;
}

void CGUIFadeLabelControl::SetLabel(const vector<wstring> &vecLabel)
{
  m_vecLabels = vecLabel;
}

void CGUIFadeLabelControl::Render()
{
	if (!UpdateFadeState())
	{
		return ;
	}
	if (!m_pFont) return ;


	if ((int)m_vecLabels.size() == 0 && (int)m_vecInfo.size() == 0) return ;

	int iTempLabelCount = m_iCurrentLabel;
	if ((int)m_vecLabels.size() > 0 && m_iCurrentLabel >= (int)m_vecLabels.size() )
	{
		m_iCurrentLabel = 0;
	}

	wstring tempLabel = L"";
	int iLabelCount = (int)m_vecLabels.size();
	if (iLabelCount > 0)
	{
		tempLabel = m_vecLabels[m_iCurrentLabel];
	}
	WCHAR szLabel[1024];
	swprintf(szLabel, L"%s", tempLabel.c_str() );
	CStdString strRenderLabel = szLabel;

	if (m_vecInfo.size())
	{ 
		iLabelCount = (int)m_vecInfo.size();
		m_iCurrentLabel = iTempLabelCount;
		if (m_iCurrentLabel >= (int)m_vecInfo.size() )
		{
			m_iCurrentLabel = 0;
		}
		strRenderLabel = g_infoManager.GetLabel(m_vecInfo[m_iCurrentLabel]);
	}
	else
	{
		strRenderLabel = ParseLabel(strRenderLabel);
	}

  CStdStringW strLabelUnicode;
  g_charsetConverter.stringCharsetToFontCharset(strRenderLabel, strLabelUnicode);

  if (iLabelCount == 1)
  {
    DWORD iWidth = (DWORD)m_pFont->GetTextWidth(strLabelUnicode.c_str());
    if (iWidth < m_dwWidth)
    {
      m_pFont->DrawText( (float)m_iPosX, (float)m_iPosY, m_dwTextColor, strLabelUnicode.c_str());
      return ;
    }
  }
  if (m_bFadeIn)
  {
    DWORD dwAlpha = (m_dwAlpha / 12) * m_iCurrentFrame;
    dwAlpha <<= 24;
    dwAlpha += ( m_dwTextColor & 0x00ffffff);
    m_pFont->DrawTextWidth((float)m_iPosX, (float)m_iPosY, dwAlpha, strLabelUnicode.c_str(), (float)m_dwWidth);

    m_iCurrentFrame++;
    if (m_iCurrentFrame >= 12)
    {
      m_bFadeIn = false;
    }
  }
  else
  {
    if (m_scrollInfo.characterPos > strLabelUnicode.size())
    { // reset to fade in again
      m_iCurrentLabel++;
      m_bFadeIn = true;
      m_iCurrentFrame = 0;
      m_scrollInfo.Reset();
    }
    else
      RenderText((float)m_iPosX, (float)m_iPosY, (float) m_dwWidth, m_dwTextColor, (WCHAR*) strLabelUnicode.c_str(), true );
  }
}


bool CGUIFadeLabelControl::CanFocus() const
{
  return false;
}


bool CGUIFadeLabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      wstring strLabel = message.GetLabel();
      m_vecLabels.push_back(strLabel);
    }
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecLabels.erase(m_vecLabels.begin(), m_vecLabels.end());
      m_scrollInfo.Reset();
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_vecLabels.erase(m_vecLabels.begin(), m_vecLabels.end());
      m_vecLabels.push_back(message.GetLabel());
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIFadeLabelControl::RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, bool bScroll )
{
  if (!m_pFont) return;

  // increase our text width to the maximum width
  float unneeded, width, spacewidth;
  m_pFont->GetTextExtent( wszText, &width, &unneeded);
  WCHAR space[2]; space[0] = L' '; space[1] = 0;
  m_pFont->GetTextExtent(space, &spacewidth, &unneeded);
  // ok, concat spaces on
  WCHAR wszOrgText[1024];
  wcscpy(wszOrgText, wszText);
  while (width < fMaxWidth)
  {
    wcscat(wszOrgText, L" ");
    width += spacewidth;
  }
  // now add enough spaces to cover the text area completely
  width = 0;
  while (width < fMaxWidth)
  {
    wcscat(wszOrgText, L" ");
    width += spacewidth;
  }

  m_pFont->DrawScrollingText(fPosX, fPosY, &m_dwTextColor, 1, wszOrgText, fMaxWidth, m_scrollInfo);
}

void CGUIFadeLabelControl::SetAlpha(DWORD dwAlpha)
{
  CGUIControl::SetAlpha(dwAlpha);
  m_dwTextColor = (dwAlpha << 24) | (m_dwTextColor & 0xFFFFFF);
}
