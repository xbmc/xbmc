#include "include.h"
#include "GUILabelControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const wstring& strLabel, const CLabelInfo& labelInfo, bool bHasPath)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  SetLabel(strLabel);
  m_label = labelInfo;
  m_bHasPath = bHasPath;
  m_bShowCursor = false;
  m_iCursorPos = 0;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_ScrollInsteadOfTruncate = false;
  m_wrapMultiLine = false;
}

CGUILabelControl::~CGUILabelControl(void)
{}

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

void CGUILabelControl::SetInfo(const vector<int> &vecInfo)
{
  m_vecInfo = vecInfo;
  if (m_vecInfo.size() == 0)
    return; // no info at all
}

void CGUILabelControl::Render()
{
  if (!IsVisible()) return;

	WCHAR szLabel[1024];
	swprintf(szLabel, L"%s", m_strLabel.c_str() );
	CStdString strRenderLabel = szLabel;

	if (m_vecInfo.size())
	{ 
		strRenderLabel = g_infoManager.GetLabel(m_vecInfo[0]);
	}
	else
	{
		strRenderLabel = g_infoManager.ParseLabel(strRenderLabel);
	}
  if (m_wrapMultiLine && m_dwWidth > 0)
    WrapText(strRenderLabel);

  if (m_label.font)
  {
    CStdStringW strLabelUnicode;
    g_charsetConverter.stringCharsetToFontCharset(strRenderLabel, strLabelUnicode);

    // check for scrolling
    bool bNormalDraw = true;
    if (m_ScrollInsteadOfTruncate && m_dwWidth > 0 && !IsDisabled())
    { // ignore align center - just use align left/right
      float width, height;
      m_label.font->GetTextExtent(strLabelUnicode.c_str(), &width, &height);
      if (width > m_dwWidth)
      { // need to scroll - set the viewport.  Should be set just using the height of the text
        bNormalDraw = false;
        float fPosX = (float)m_iPosX;
        if (m_label.align & XBFONT_RIGHT)
          fPosX -= (float)m_dwWidth;

        m_label.font->DrawScrollingText(fPosX, (float)m_iPosY, m_label.angle, &m_label.textColor, 1, m_label.shadowColor, strLabelUnicode, (float)m_dwWidth, m_ScrollInfo);
      }
    }
    if (bNormalDraw)
    {
      float fPosX = (float)m_iPosX;
      if (m_label.align & XBFONT_CENTER_X)
        fPosX += (float)m_dwWidth / 2;

      float fPosY = (float)m_iPosY;
      if (m_label.align & XBFONT_CENTER_Y)
        fPosY += (float)m_dwHeight / 2;

      if (IsDisabled())
      {
        m_label.font->DrawText(fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, strLabelUnicode.c_str(), m_label.align | XBFONT_TRUNCATED, (float)m_dwWidth);
      }
      else
      {
        if (m_bShowCursor)
        { // show the cursor...
          strLabelUnicode.Insert(m_iCursorPos, L"|");
          if (m_label.align & XBFONT_CENTER_X)
            fPosX -= m_label.font->GetTextWidth(strLabelUnicode.c_str()) * 0.5f;
          BYTE *palette = new BYTE[strLabelUnicode.size()];
          for (unsigned int i=0; i < strLabelUnicode.size(); i++)
            palette[i] = 0;
          palette[m_iCursorPos] = 1;
          DWORD color[2];
          color[0] = m_label.textColor;
          if ((++m_dwCounter % 50) > 25)
            color[1] = m_label.textColor;
          else
            color[1] = 0; // transparent black
          m_label.font->DrawColourTextWidth(fPosX, fPosY, m_label.angle, color, 2, m_label.shadowColor, strLabelUnicode.c_str(), palette, (float)m_dwWidth);
          delete[] palette;
        }
        else
          m_label.font->DrawText(fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, strLabelUnicode.c_str(), m_label.align | XBFONT_TRUNCATED, (float)m_dwWidth);
      }
    }
  }
  CGUIControl::Render();
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}

void CGUILabelControl::SetAlpha(DWORD dwAlpha)
{
  CGUIControl::SetAlpha(dwAlpha);
  m_label.textColor = (dwAlpha << 24) | (m_label.textColor & 0xFFFFFF);
  m_label.disabledColor = (dwAlpha << 24) | (m_label.disabledColor & 0xFFFFFF);
}

void CGUILabelControl::SetLabel(const wstring &strLabel)
{
  if (m_strLabel.compare(strLabel) == 0)
    return;
  m_strLabel = strLabel;
  SetWidthControl(m_ScrollInsteadOfTruncate);
  if (m_iCursorPos > (int)m_strLabel.size())
    m_iCursorPos = m_strLabel.size();
}

void CGUILabelControl::SetWidthControl(bool bScroll)
{
  m_ScrollInsteadOfTruncate = bScroll;
  m_ScrollInfo.Reset();
}

void CGUILabelControl::SetText(CStdString aLabel)
{
  WCHAR wszText[1024];
  swprintf(wszText, L"%S", aLabel.c_str());
  SetLabel(wszText);
}

bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());

      if ( m_bHasPath )
      {
        ShortenPath();
      }
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUILabelControl::WrapText(CStdString &text)
{
  // run through and force line breaks at spaces as and when necessary
  if (!m_label.font)
    return;
  BOOL bStartingNewLine = TRUE;
  BOOL bBreakAtSpace = FALSE;
  int pos = 0;
  text += "\n";
  int iTextSize = (int)text.size();
  int lpos = 0;
  int iLastSpace = -1;
  int iLastSpaceInLine = -1;
  CStdString multiLine, line;
  while (pos < iTextSize)
  {
    // Get the current letter in the string
    char letter = text[pos];

    // Handle the newline character
    if (letter == '\n' )
    {
      if (!multiLine.IsEmpty())
        multiLine += "\n";
      multiLine += line;
      iLastSpace = -1;
      iLastSpaceInLine = -1;
      lpos = 0;
      line.Empty();
    }
    else
    {
      if (letter == ' ')
      {
        CStdStringW wLine = line;
        float width = m_label.font->GetTextWidth(wLine.c_str());
        if (width > m_dwWidth)
        {
          if (iLastSpace > 0 && iLastSpaceInLine > 0)
          {
            if (!multiLine.IsEmpty())
              multiLine += "\n";
            multiLine += line.Left(iLastSpaceInLine);
            line.Empty();
            pos = iLastSpace + 1;
            iLastSpaceInLine = -1;
            iLastSpace = -1;
            continue;
          }
        }
        // only add spaces if we're not empty
        if (!line.IsEmpty())
        {
          iLastSpace = pos;
          iLastSpaceInLine = line.size();
          line += letter;
        }
      }
      else
        line += letter;
    }
    pos++;
  }
  text = multiLine;
}

void CGUILabelControl::ShortenPath()
{
  if (!m_label.font)
    return ;
  if ( m_dwWidth <= 0 )
    return ;
  if ( m_strLabel.size() <= 0 )
    return ;

  float fTextHeight, fTextWidth;
  char cDelim = '\0';
  int nGreaterDelim, nPos;

  nPos = m_strLabel.find_last_of( '\\' );
  if ( nPos >= 0 )
    cDelim = '\\';
  else
  {
    nPos = m_strLabel.find_last_of( '/' );
    if ( nPos >= 0 )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return ;

  // remove trailing slashes
  if (nPos == m_strLabel.size() - 1)
  {
    m_strLabel.erase(m_strLabel.size() - 1);
    nPos = m_strLabel.find_last_of( cDelim );
  }

  CStdStringW strLabelUnicode;
  g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);

  m_label.font->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth, &fTextHeight);

  if ( fTextWidth <= (m_dwWidth) )
    return ;

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
      m_strLabel.replace( nPos + 1, nGreaterDelim - nPos - 1, L"..." );
      g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);
    }

    m_label.font->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth, &fTextHeight );
  }
}

void CGUILabelControl::SetTruncate(bool bTruncate)
{
  if (bTruncate)
    m_label.align |= XBFONT_TRUNCATED;
  else
    m_label.align &= ~XBFONT_TRUNCATED;
}
