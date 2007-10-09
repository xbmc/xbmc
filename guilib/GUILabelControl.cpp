#include "include.h"
#include "GUILabelControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const string& strLabel, const CLabelInfo& labelInfo, bool bHasPath)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_bHasPath = bHasPath;
  m_iCursorPos = 0; 
  SetLabel(strLabel);
  m_label = labelInfo;
  m_bShowCursor = false;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_ScrollInsteadOfTruncate = false;
  m_wrapMultiLine = false;
  m_singleInfo = 0;
  m_startHighlight = m_endHighlight = 0;
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

void CGUILabelControl::SetInfo(int singleInfo)
{
  m_singleInfo = singleInfo;
}

void CGUILabelControl::Render()
{
  CStdString lastLabel(m_renderLabel);
	if (m_singleInfo)
	{ 
		m_renderLabel = g_infoManager.GetLabel(m_singleInfo);
	}
	else
	{
    m_renderLabel = g_infoManager.GetMultiInfo(m_multiInfo, m_dwParentID);
	}
  // reset scrolling if we have a new label
  if (m_renderLabel != lastLabel)
    m_ScrollInfo.Reset();

  if (m_label.font)
  {
    CStdStringW strLabelUnicode;
    g_charsetConverter.utf8ToW(m_renderLabel, strLabelUnicode);

    if (m_wrapMultiLine && m_width > 0)
      WrapText(strLabelUnicode, m_label.font, m_width);

    // check for scrolling
    bool bNormalDraw = true;
    if (m_ScrollInsteadOfTruncate && m_width > 0 && !IsDisabled())
    { // ignore align center - just use align left/right
      float width, height;
      m_label.font->GetTextExtent(strLabelUnicode.c_str(), &width, &height);
      if (width > m_width)
      { // need to scroll - set the viewport.  Should be set just using the height of the text
        bNormalDraw = false;
        float fPosX = m_posX;
        if (m_label.align & XBFONT_RIGHT)
          fPosX -= m_width;

        float fPosY = m_posY;
        if (m_label.align & XBFONT_CENTER_Y)
          fPosY += (m_height - height) * 0.5f;  // adjust for height of text as well, as DrawScrollingText doesn't do it for us

        m_label.font->DrawScrollingText(fPosX, fPosY, m_label.angle, &m_label.textColor, 1, m_label.shadowColor, strLabelUnicode, m_width, m_ScrollInfo);
      }
    }
    if (bNormalDraw)
    {
      float fPosX = m_posX;
      if (m_label.align & XBFONT_CENTER_X)
        fPosX += m_width * 0.5f;

      float fPosY = m_posY;
      if (m_label.align & XBFONT_CENTER_Y)
        fPosY += m_height * 0.5f;

      if (IsDisabled())
      {
        m_label.font->DrawText(fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, strLabelUnicode.c_str(), m_label.align | XBFONT_TRUNCATED, m_width);
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
          m_label.font->DrawColourTextWidth(fPosX, fPosY, m_label.angle, color, 2, m_label.shadowColor, strLabelUnicode.c_str(), palette, m_width);
          delete[] palette;
        }
        else if (m_startHighlight || m_endHighlight)
        {
          DWORD palette[2] = { m_label.disabledColor, m_label.textColor };
          BYTE *colors = new BYTE[strLabelUnicode.size()];
          for (unsigned int i = 0; i < strLabelUnicode.size(); i++)
            colors[i] = 0;
          for (unsigned int i = m_startHighlight; i < m_endHighlight && i < strLabelUnicode.size(); i++)
            colors[i] = 1;
          m_label.font->DrawColourTextWidth(fPosX, fPosY, m_label.angle, palette, 2, m_label.shadowColor, strLabelUnicode.c_str(), colors, m_width);
          delete[] colors;
        }
        else
          m_label.font->DrawText(fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, strLabelUnicode.c_str(), m_label.align | XBFONT_TRUNCATED, m_width);
      }
    }
  }
  CGUIControl::Render();
}


bool CGUILabelControl::CanFocus() const
{
  return false;
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

void CGUILabelControl::SetAlignment(DWORD align)
{
  m_label.align = align;
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
  g_charsetConverter.utf8ToW(text, utf16Text);
  WrapText(utf16Text, font, maxWidth);
  g_charsetConverter.wToUTF8(utf16Text, text);
}

void CGUILabelControl::WrapText(CStdStringW &utf16Text, CGUIFont *font, float maxWidth)
{
  vector<CStdStringW> multiLine;
  WrapText(utf16Text, font, maxWidth, multiLine);
  utf16Text.Empty();
  for (unsigned int i = 0; i < multiLine.size(); i++)
    utf16Text += multiLine[i] + L'\n';
  utf16Text.Trim();
}

void CGUILabelControl::WrapText(CStdStringW &utf16Text, CGUIFont *font, float maxWidth, vector<CStdStringW> &multiLine)
{
  // run through and force line breaks at spaces as and when necessary
  if (!font)
    return;
  unsigned int pos = 0;
  utf16Text += L"\n";
  int iLastSpaceInLine = -1;
  int iLastSpace = -1;
  CStdStringW line;
  multiLine.clear();
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
        multiLine.push_back(line.Left(iLastSpaceInLine));
        line = line.Mid(iLastSpaceInLine + 1);
        if (!line.IsEmpty())
          multiLine.push_back(line);
      }
      else
        multiLine.push_back(line);
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
            multiLine.push_back(line.Left(iLastSpaceInLine));
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
}

CStdString CGUILabelControl::ShortenPath(const CStdString &path)
{
  if (!m_label.font || m_width == 0 || path.IsEmpty())
    return path;

  // convert to utf16 for text extent measures
  CStdStringW utf16Path;
  g_charsetConverter.utf8ToW(path, utf16Path);

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
      if (nPos == (int) utf16Path.size() - 1)
      {
        utf16Path.erase(utf16Path.size() - 1);
        nPos = utf16Path.find_last_of( cDelim );
      }

  m_label.font->GetTextExtent( utf16Path.c_str(), &fTextWidth, &fTextHeight);

  while ( fTextWidth > m_width )
  {
    nPos = utf16Path.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    if ( nPos >= 0 )
      nPos = utf16Path.find_last_of( cDelim, nPos - 1 );
    else
      break;

    if ( nPos <= 0 )
      break;

    if ( nGreaterDelim > nPos )
      utf16Path.replace( nPos + 1, nGreaterDelim - nPos - 1, L"..." );

    m_label.font->GetTextExtent( utf16Path.c_str(), &fTextWidth, &fTextHeight );
  }
  // convert back to utf8
  CStdString utf8Path;
  g_charsetConverter.wToUTF8(utf16Path, utf8Path);
  return utf8Path;
}

void CGUILabelControl::SetTruncate(bool bTruncate)
{
  if (bTruncate)
    m_label.align |= XBFONT_TRUNCATED;
  else
    m_label.align &= ~XBFONT_TRUNCATED;
}

void CGUILabelControl::SetHighlight(unsigned int start, unsigned int end)
{
  m_startHighlight = start;
  m_endHighlight = end;
}

