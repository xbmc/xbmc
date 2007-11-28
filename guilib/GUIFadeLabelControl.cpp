#include "include.h"
#include "GUIFadeLabelControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIFadeLabelControl::CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool scrollOut, DWORD timeToPauseAtEnd)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height), m_scrollInfo(50, labelInfo.offsetX)
{
  m_label = labelInfo;
  m_iCurrentLabel = 0;
  m_bFadeIn = false;
  m_bFadeOut = false;
  m_bPaused = false;
  m_iCurrentFrame = 0;
  m_bScrollOut = scrollOut;
  m_iTimeToPauseAtEnd = timeToPauseAtEnd;
  ControlType = GUICONTROL_FADELABEL;
}

CGUIFadeLabelControl::~CGUIFadeLabelControl(void)
{
}

void CGUIFadeLabelControl::SetInfo(const vector<int> &vecInfo)
{
  m_vecInfo = vecInfo;
}

void CGUIFadeLabelControl::SetLabel(const vector<string> &vecLabel)
{
  m_stringLabels = vecLabel;
  m_infoLabels.clear();
  for (unsigned int i = 0; i < vecLabel.size(); i++)
    AddLabel(vecLabel[i]);
}

void CGUIFadeLabelControl::AddLabel(const string &label)
{
  vector<CInfoPortion> info;
  g_infoManager.ParseLabel(label, info);
  m_infoLabels.push_back(info);
}

void CGUIFadeLabelControl::Render()
{
	if (!m_label.font || (m_infoLabels.size() == 0 && m_vecInfo.size() == 0))
  {
    CGUIControl::Render();
    return ;
  }

	int iTempLabelCount = m_iCurrentLabel;
	if ((int)m_infoLabels.size() > 0 && m_iCurrentLabel >= (int)m_infoLabels.size() )
	{
		m_iCurrentLabel = 0;
	}

  CStdString strRenderLabel;
	int iLabelCount = (int)m_infoLabels.size();
	if (iLabelCount > 0)
	{
		strRenderLabel = g_infoManager.GetMultiInfo(m_infoLabels[m_iCurrentLabel], m_dwParentID);
	}

	if (m_vecInfo.size())
	{ 
		iLabelCount = (int)m_vecInfo.size();
		m_iCurrentLabel = iTempLabelCount;
		if (m_iCurrentLabel >= (int)m_vecInfo.size() )
		{
			m_iCurrentLabel = 0;
		}
		strRenderLabel = g_infoManager.GetLabel(m_vecInfo[m_iCurrentLabel], m_dwParentID);
	}

  CStdStringW strLabelUnicode;
  g_charsetConverter.utf8ToW(strRenderLabel, strLabelUnicode);
    
  if (iLabelCount == 1)
  {
    float width = m_label.font->GetTextWidth(strLabelUnicode.c_str());
    if (width + m_label.offsetX < m_width)
    {
      m_label.font->DrawText(m_posX + m_label.offsetX, m_posY, m_label.textColor, m_label.shadowColor, strLabelUnicode.c_str());
      CGUIControl::Render();
      return ;
    }
  }
  
  if (!m_bFadeIn && !m_bFadeOut && !m_bPaused)
  {
    bool moveToNextLabel = false;
    
    if (!m_bScrollOut)
    {
       CStdStringW remainingString = strLabelUnicode.Mid(m_scrollInfo.characterPos);
       float width = m_label.font->GetTextWidth(remainingString.c_str());
       moveToNextLabel = (width < m_width - m_label.offsetX);
    } 
    else
    {
       moveToNextLabel = (m_scrollInfo.characterPos > strLabelUnicode.size());
    }
    
    if (moveToNextLabel)
    { // reset to fade in again
      if (!m_bScrollOut)
      {
         m_bPaused = true;
      }
      else
      {
         m_iCurrentLabel++;
         m_bFadeIn = true;
         m_scrollInfo.Reset();
      }
      m_iCurrentFrame = 0;      
    }
    else
      RenderText(m_posX, m_posY, m_width, m_label.textColor, (WCHAR*) strLabelUnicode.c_str(), true );
  }
  
  if (m_bFadeIn || m_bFadeOut)
  {
    DWORD dwAlpha = 21 * m_iCurrentFrame;
    if (m_bFadeOut) dwAlpha = 255 - dwAlpha;
    dwAlpha <<= 24;
    dwAlpha += ( m_label.textColor & 0x00ffffff);
    m_label.font->DrawTextWidth(m_posX + m_label.offsetX, m_posY, dwAlpha, m_label.shadowColor, strLabelUnicode.c_str(), m_width - m_label.offsetX);

    m_iCurrentFrame++;
    if (m_iCurrentFrame >= 12)
    {
      if (m_bFadeOut)
         m_iCurrentLabel++;
      m_bFadeIn = false;
      m_bFadeOut = false;
    }
  }
  else if (m_bPaused)
  {
     CStdStringW remainingString = strLabelUnicode.Mid(m_scrollInfo.characterPos);
     m_label.font->DrawTextWidth(m_posX + m_label.offsetX, m_posY, m_label.textColor, m_label.shadowColor, remainingString.c_str(), m_width - m_label.offsetX);
     
     m_iCurrentFrame++;
     if (m_iCurrentFrame >= (int) m_iTimeToPauseAtEnd)
     {
        m_iCurrentFrame = 0;
        m_scrollInfo.Reset();
        m_bPaused = false;
        m_bFadeOut = true;
     }
  }
    
  CGUIControl::Render();
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
      AddLabel(message.GetLabel());
    }
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_infoLabels.clear();
      m_stringLabels.clear();
      m_scrollInfo.Reset();
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_infoLabels.clear();
      m_stringLabels.clear();
      m_scrollInfo.Reset();
      AddLabel(message.GetLabel());
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIFadeLabelControl::RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, bool bScroll )
{
  if (!m_label.font) return;

  // increase our text width to the maximum width
  float unneeded, width, spacewidth;
  m_label.font->GetTextExtent( wszText, &width, &unneeded);
  WCHAR space[2]; space[0] = L' '; space[1] = 0;
  m_label.font->GetTextExtent(space, &spacewidth, &unneeded);
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

  m_label.font->DrawScrollingText(fPosX, fPosY, &m_label.textColor, 1, m_label.shadowColor, wszOrgText, fMaxWidth, m_scrollInfo);
}

