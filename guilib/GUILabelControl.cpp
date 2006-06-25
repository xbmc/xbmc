#include "include.h"
#include "GUILabelControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const string& strLabel, const CLabelInfo& labelInfo, bool bHasPath)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_bHasPath = bHasPath;
  SetLabel(strLabel);
  m_label = labelInfo;
  m_bShowCursor = false;
  m_iCursorPos = 0;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_ScrollInsteadOfTruncate = false;
  m_wrapMultiLine = false;
  m_singleInfo = 0;
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

void CGUILabelControl::SetInfo(int singleInfo)
{
  m_singleInfo = singleInfo;
}

void CGUILabelControl::Render()
{
  if (!IsVisible()) return;

  CStdString renderLabel;
	if (m_singleInfo)
	{ 
		renderLabel = g_infoManager.GetLabel(m_singleInfo);
	}
	else
	{
    renderLabel = g_infoManager.GetMultiLabel(m_multiInfo);
	}
  if (m_wrapMultiLine && m_dwWidth > 0)
    WrapText(renderLabel, m_label.font, (float)m_dwWidth);

  if (m_label.font)
  {
    CStdStringW strLabelUnicode;
    g_charsetConverter.utf8ToUTF16(renderLabel, strLabelUnicode);

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

void CGUILabelControl::SetLabel(const string &strLabel)
{
  if (m_strLabel.compare(strLabel) == 0)
    return;
  m_strLabel = strLabel;

  // shorten the path label
  if ( m_bHasPath )
  {
    m_multiInfo.clear();
    m_multiInfo.push_back(CInfoPortion(0, ShortenPath(strLabel), ""));
  }
  else // parse the label for info tags
    g_infoManager.ParseLabel(strLabel, m_multiInfo);
  SetWidthControl(m_ScrollInsteadOfTruncate);
  if (m_iCursorPos > (int)m_strLabel.size())
    m_iCursorPos = m_strLabel.size();
}

void CGUILabelControl::SetWidthControl(bool bScroll)
{
  m_ScrollInsteadOfTruncate = bScroll;
  m_ScrollInfo.Reset();
}

bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUILabelControl::WrapText(CStdString &text, CGUIFont *font, float maxWidth)
{
  CStdStringW utf16Text;
  g_charsetConverter.utf8ToUTF16(text, utf16Text);
  WrapText(utf16Text, font, maxWidth);
  g_charsetConverter.utf16toUTF8(utf16Text, text);
}

void CGUILabelControl::WrapText(CStdStringW &utf16Text, CGUIFont *font, float maxWidth)
{
  // run through and force line breaks at spaces as and when necessary
  if (!font)
    return;
  unsigned int pos = 0;
  utf16Text += L"\n";
  int iLastSpaceInLine = -1;
  int iLastSpace = -1;
  CStdStringW multiLine, line;
  while (pos < utf16Text.size())
  {
    // Get the current letter in the string
    WCHAR letter = utf16Text[pos];

    // Handle the newline character
    if (letter == L'\n' )
    {
      float width = font->GetTextWidth(line.c_str());
      if (width > maxWidth && iLastSpaceInLine > 0)
      {
        multiLine += line.Left(iLastSpaceInLine) + L'\n';
        line = line.Mid(iLastSpaceInLine + 1);
        if (!line.IsEmpty())
          multiLine += line + L"\n";
      }
      else
        multiLine += line + L"\n";
      iLastSpaceInLine = -1;
      iLastSpace = -1;
      line.Empty();
    }
    else
    {
      if (letter == L' ')
      {
        float width = font->GetTextWidth(line.c_str());
        if (width > maxWidth)
        {
          if (iLastSpace > 0 && iLastSpaceInLine > 0)
          {
            multiLine += line.Left(iLastSpaceInLine) + L'\n';
            pos = iLastSpace + 1;
            line.Empty();
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
  utf16Text = multiLine;
}

CStdString CGUILabelControl::ShortenPath(const CStdString &path)
{
  if (!m_label.font || m_dwWidth == 0 || path.IsEmpty())
    return path;

  // convert to utf16 for text extent measures
  CStdStringW utf16Path;
  g_charsetConverter.utf8ToUTF16(path, utf16Path);

  float fTextHeight, fTextWidth;
  WCHAR cDelim = L'\0';
  int nGreaterDelim, nPos;

  nPos = utf16Path.find_last_of( L'\\' );
  if ( nPos >= 0 )
    cDelim = L'\\';
  else
  {
    nPos = path.find_last_of( L'/' );
    if ( nPos >= 0 )
      cDelim = L'/';
  }
  if ( cDelim == L'\0' )
    return path;

  // remove trailing slashes
  if (utf16Path.size() > 3)
    if (utf16Path.Right(3).Compare(CStdStringW("://")) != 0 && utf16Path.Right(2).Compare(CStdStringW(":\\")) != 0)
      if (nPos == utf16Path.size() - 1)
      {
        utf16Path.erase(utf16Path.size() - 1);
        nPos = utf16Path.find_last_of( cDelim );
      }

  m_label.font->GetTextExtent( utf16Path.c_str(), &fTextWidth, &fTextHeight);

  while ( fTextWidth > m_dwWidth )
  {
    nPos = utf16Path.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos >= 0 )
      nPos = utf16Path.find_last_of( cDelim, nPos - 1 );
    else
      break;

    if ( nPos < 0 )
      break;

    if ( nGreaterDelim > nPos )
      utf16Path.replace( nPos + 1, nGreaterDelim - nPos - 1, L"..." );

    m_label.font->GetTextExtent( utf16Path.c_str(), &fTextWidth, &fTextHeight );
  }
  // convert back to utf8
  CStdString utf8Path;
  g_charsetConverter.utf16toUTF8(utf16Path, utf8Path);
  return utf8Path;
}

void CGUILabelControl::SetTruncate(bool bTruncate)
{
  if (bTruncate)
    m_label.align |= XBFONT_TRUNCATED;
  else
    m_label.align &= ~XBFONT_TRUNCATED;
}
