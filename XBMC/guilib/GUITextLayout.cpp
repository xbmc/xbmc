/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUITextLayout.h"
#include "GUIFont.h"
#include "GUIControl.h"
#include "GUIColorManager.h"
#include "utils/CharsetConverter.h"
#include "StringUtils.h"

using namespace std;

#define WORK_AROUND_NEEDED_FOR_LINE_BREAKS

CGUIString::CGUIString(iString start, iString end, bool carriageReturn)
{
  m_text.assign(start, end);
  m_carriageReturn = carriageReturn;
}

CStdString CGUIString::GetAsString() const
{
  CStdString text;
  for (unsigned int i = 0; i < m_text.size(); i++)
    text += (char)(m_text[i] & 0xff);
  return text;
}

CGUITextLayout::CGUITextLayout(CGUIFont *font, bool wrap, float fHeight)
{
  m_font = font;
  m_textColor = 0;
  m_wrap = wrap;
  m_maxHeight = fHeight;
}

void CGUITextLayout::SetWrap(bool bWrap)
{
  m_wrap = bWrap;
}

void CGUITextLayout::Render(float x, float y, float angle, DWORD color, DWORD shadowColor, DWORD alignment, float maxWidth, bool solid)
{
  if (!m_font)
    return;

  // set the main text color
  if (m_colors.size())
    m_colors[0] = color;

  // render the text at the required location, angle, and size
  if (angle)
  {
    static const float degrees_to_radians = 0.01745329252f;
    g_graphicsContext.AddTransform(TransformMatrix::CreateZRotation(angle * degrees_to_radians, x, y, g_graphicsContext.GetScalingPixelRatio()));
  }
  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;;
    alignment &= ~XBFONT_CENTER_Y;
  }
  m_font->Begin();
  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    const CGUIString &string = *i;
    DWORD align = alignment;
    if (align & XBFONT_JUSTIFIED && string.m_carriageReturn)
      align &= ~XBFONT_JUSTIFIED;
    if (solid)
      m_font->DrawText(x, y, m_colors[0], shadowColor, string.m_text, align, maxWidth);
    else
      m_font->DrawText(x, y, m_colors, shadowColor, string.m_text, align, maxWidth);
    y += m_font->GetLineHeight();
  }
  m_font->End();
  if (angle)
    g_graphicsContext.RemoveTransform();
}


void CGUITextLayout::RenderScrolling(float x, float y, float angle, DWORD color, DWORD shadowColor, DWORD alignment, float maxWidth, CScrollInfo &scrollInfo)
{
  if (!m_font)
    return;

  // set the main text color
  if (m_colors.size())
    m_colors[0] = color;

  // render the text at the required location, angle, and size
  if (angle)
  {
    static const float degrees_to_radians = 0.01745329252f;
    g_graphicsContext.AddTransform(TransformMatrix::CreateZRotation(angle * degrees_to_radians, x, y, g_graphicsContext.GetScalingPixelRatio()));
  }
  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;;
    alignment &= ~XBFONT_CENTER_Y;
  }
  m_font->Begin();
  // NOTE: This workaround is needed as otherwise multi-line text that scrolls
  //       will scroll in proportion to the number of lines.  Ideally we should
  //       do the DrawScrollingText calculation here.  This probably won't make
  //       any difference to the smoothness of scrolling though which will be
  //       jumpy with this sort of thing.  It's not exactly a well used situation
  //       though, so this hack is probably OK.
  float speed = scrollInfo.pixelSpeed;
  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    const CGUIString &string = *i;
    m_font->DrawScrollingText(x, y, m_colors, shadowColor, string.m_text, alignment, maxWidth, scrollInfo);
    y += m_font->GetLineHeight();
    scrollInfo.pixelSpeed = 0;
  }
  scrollInfo.pixelSpeed = speed;
  m_font->End();
  if (angle)
    g_graphicsContext.RemoveTransform();
}

void CGUITextLayout::RenderOutline(float x, float y, DWORD color, DWORD outlineColor, DWORD outlineWidth, DWORD alignment, float maxWidth)
{
  if (!m_font)
    return;

  // set the main text color
  if (m_colors.size())
    m_colors[0] = color;

  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;;
    alignment &= ~XBFONT_CENTER_Y;
  }
  m_font->Begin();
  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    const CGUIString &string = *i;
    DWORD align = alignment;
    if (align & XBFONT_JUSTIFIED && string.m_carriageReturn)
      align &= ~XBFONT_JUSTIFIED;

    DrawOutlineText(m_font, x, y, m_colors, outlineColor, outlineWidth, string.m_text, align, maxWidth);
    y += m_font->GetLineHeight();
  }
  m_font->End();
}

bool CGUITextLayout::Update(const CStdString &text, float maxWidth)
{
  if (text == m_lastText)
    return false;

  // convert to utf16
  CStdStringW utf16;
  utf8ToW(text, utf16);

  // update
  SetText(utf16, maxWidth);

  // and set our parameters to indicate no further update is required
  m_lastText = text;
  return true;
}

void CGUITextLayout::SetText(const CStdStringW &text, float maxWidth)
{
  vector<DWORD> parsedText;

  // empty out our previous string
  m_lines.clear();
  m_colors.clear();
  m_colors.push_back(m_textColor);

  // parse the text into our string objects
  ParseText(text, parsedText);

  // add \n to the end of the string
  parsedText.push_back(L'\n');

  // if we need to wrap the text, then do so
  if (m_wrap && maxWidth > 0)
    WrapText(parsedText, maxWidth);
  else
    LineBreakText(parsedText, m_lines);
}

void CGUITextLayout::Filter(CStdString &text)
{
  CStdStringW utf16;
  utf8ToW(text, utf16);
  vector<DWORD> colors;
  vector<DWORD> parsedText;
  ParseText(utf16, 0, colors, parsedText);
  utf16.Empty();
  for (unsigned int i = 0; i < parsedText.size(); i++)
    utf16 += (WCHAR)(0xffff & parsedText[i]);
  g_charsetConverter.wToUTF8(utf16, text);
}

void CGUITextLayout::ParseText(const CStdStringW &text, vector<DWORD> &parsedText)
{
  if (!m_font)
    return;
  ParseText(text, m_font->GetStyle(), m_colors, parsedText);
}

void CGUITextLayout::ParseText(const CStdStringW &text, DWORD defaultStyle, vector<DWORD> &colors, vector<DWORD> &parsedText)
{
  // run through the string, searching for:
  // [B] or [/B] -> toggle bold on and off
  // [I] or [/I] -> toggle italics on and off
  // [COLOR ffab007f] or [/COLOR] -> toggle color on and off
  // [CAPS <option>] or [/CAPS] -> toggle capatilization on and off

  DWORD currentStyle = defaultStyle; // start with the default font's style
  DWORD currentColor = 0;

  stack<DWORD> colorStack;
  colorStack.push(0);

  // these aren't independent, but that's probably not too much of an issue
  // eg [UPPERCASE]Glah[LOWERCASE]FReD[/LOWERCASE]Georeg[/UPPERCASE] will work (lower case >> upper case)
  // but [LOWERCASE]Glah[UPPERCASE]FReD[/UPPERCASE]Georeg[/LOWERCASE] won't
#define FONT_STYLE_UPPERCASE 4
#define FONT_STYLE_LOWERCASE 8

  int startPos = 0;
  size_t pos = text.Find(L'[');
  while (pos != CStdString::npos && pos + 1 < text.size())
  {
    DWORD newStyle = 0;
    DWORD newColor = currentColor;
    bool newLine = false;
    // have a [ - check if it's an ON or OFF switch
    bool on(true);
    int endPos = pos++; // finish of string
    if (text[pos] == L'/')
    {
      on = false;
      pos++;
    }
    // check for each type
    if (text.Mid(pos,2) == L"B]")
    { // bold - finish the current text block and assign the bold state
      newStyle = FONT_STYLE_BOLD;
      pos += 2;
    }
    else if (text.Mid(pos,2) == L"I]")
    { // italics
      newStyle = FONT_STYLE_ITALICS;
      pos += 2;
    }
    else if (text.Mid(pos,10) == L"UPPERCASE]")
    {
      newStyle = FONT_STYLE_UPPERCASE;
      pos += 10;
    }
    else if (text.Mid(pos,10) == L"LOWERCASE]")
    {
      newStyle = FONT_STYLE_LOWERCASE;
      pos += 10;
    }
    else if (text.Mid(pos,3) == L"CR]" && on)
    {
      newLine = true;
      pos += 3;
    }
    else if (text.Mid(pos,5) == L"COLOR")
    { // color
      size_t finish = text.Find(L']', pos + 5);
      if (on && finish != CStdString::npos)
      { // create new color
        newColor = colors.size();
        colors.push_back(g_colorManager.GetColor(text.Mid(pos + 5, finish - pos - 5)));
        colorStack.push(newColor);
      }
      else if (!on && finish == pos + 5)
      { // revert to previous color
        if (colorStack.size() > 1)
          colorStack.pop();
        newColor = colorStack.top();
      }
      pos = finish + 1;
    }

    if (newStyle || newColor != currentColor || newLine)
    { // we have a new style or a new color, so format up the previous segment
      CStdStringW subText = text.Mid(startPos, endPos - startPos);
      if (currentStyle & FONT_STYLE_UPPERCASE)
        subText.ToUpper();
      if (currentStyle & FONT_STYLE_LOWERCASE)
        subText.ToLower();
      AppendToUTF32(subText, ((currentStyle & 3) << 24) | (currentColor << 16), parsedText);
      if (newLine)
        parsedText.push_back(L'\n');

      // and switch to the new style
      startPos = pos;
      currentColor = newColor;
      if (on)
        currentStyle |= newStyle;
      else
        currentStyle &= ~newStyle;
    }
    pos = text.Find(L'[',pos);
  }
  // now grab the remainder of the string
  CStdStringW subText = text.Mid(startPos, text.GetLength() - startPos);
  if (currentStyle & FONT_STYLE_UPPERCASE)
    subText.ToUpper();
  if (currentStyle & FONT_STYLE_LOWERCASE)
    subText.ToLower();
  AppendToUTF32(subText, ((currentStyle & 3) << 24) | (currentColor << 16), parsedText);
}

void CGUITextLayout::SetMaxHeight(float fHeight)
{
  m_maxHeight = fHeight;
}

void CGUITextLayout::WrapText(const vector<DWORD> &text, float maxWidth)
{
  if (!m_font)
    return;

  int nMaxLines = (m_maxHeight > 0 && m_font->GetLineHeight() > 0)?(int)(m_maxHeight / m_font->GetLineHeight()):-1;

  m_lines.clear();

  vector<CGUIString> lines;
  LineBreakText(text, lines);

  for (unsigned int i = 0; i < lines.size(); i++)
  {
    const CGUIString &line = lines[i];
    vector<DWORD>::const_iterator lastSpace = line.m_text.begin();
    vector<DWORD>::const_iterator pos = line.m_text.begin();
    unsigned int lastSpaceInLine = 0;
    vector<DWORD> curLine;
    while (pos != line.m_text.end() && (nMaxLines <= 0 || m_lines.size() <= (size_t)nMaxLines))
    {
      // Get the current letter in the string
      DWORD letter = *pos;
      // check for a space
      if (CanWrapAtLetter(letter))
      {
        float width = m_font->GetTextWidth(curLine);
        if (width > maxWidth)
        {
          if (lastSpace != line.m_text.begin() && lastSpaceInLine > 0)
          {
            CGUIString string(curLine.begin(), curLine.begin() + lastSpaceInLine, false);
            m_lines.push_back(string);
            if (IsSpace(letter))
              lastSpace++;  // ignore the space
            pos = lastSpace;
            curLine.clear();
            lastSpaceInLine = 0;
            lastSpace = line.m_text.begin();
            continue;
          }
        }
        // only add spaces if we're not empty
        if (!IsSpace(letter) || curLine.size())
        {
          lastSpace = pos;
          lastSpaceInLine = curLine.size();
          curLine.push_back(letter);
        }
      }
      else
        curLine.push_back(letter);
      pos++;
    }
    // now add whatever we have left to the string
    float width = m_font->GetTextWidth(curLine);
    if (width > maxWidth)
    {
      // too long - put up to the last space on if we can + remove it from what's left.
      if (lastSpace != line.m_text.begin() && lastSpaceInLine > 0)
      {
        CGUIString string(curLine.begin(), curLine.begin() + lastSpaceInLine, false);
        m_lines.push_back(string);
        curLine.erase(curLine.begin(), curLine.begin() + lastSpaceInLine);
        while (curLine.size() && IsSpace(curLine.at(0)))
          curLine.erase(curLine.begin());
      }
    }
    CGUIString string(curLine.begin(), curLine.end(), true);
    m_lines.push_back(string);
  }
}

void CGUITextLayout::LineBreakText(const vector<DWORD> &text, vector<CGUIString> &lines)
{
  vector<DWORD>::const_iterator lineStart = text.begin();
  vector<DWORD>::const_iterator pos = text.begin();
  while (pos != text.end())
  {
    // Get the current letter in the string
    DWORD letter = *pos;

    // Handle the newline character
    if ((letter & 0xffff) == L'\n' )
    { // push back everything up till now
      CGUIString string(lineStart, pos, true);
      lines.push_back(string);
      lineStart = pos + 1;
    }
    pos++;
  }
}

void CGUITextLayout::GetTextExtent(float &width, float &height)
{
  width = 0;
  if (!m_font) return;

  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    const CGUIString &string = *i;
    float w = m_font->GetTextWidth(string.m_text);
    if (w > width)
      width = w;
  }
  height = m_font->GetTextHeight(m_lines.size());
}

float CGUITextLayout::GetTextWidth()
{
  float width, height;
  GetTextExtent(width, height);
  return width;
}

unsigned int CGUITextLayout::GetTextLength() const
{
  unsigned int length = 0;
  for (vector<CGUIString>::const_iterator i = m_lines.begin(); i != m_lines.end(); i++)
    length += i->m_text.size();
  return length;
}

void CGUITextLayout::GetFirstText(vector<DWORD> &text) const
{
  text.clear();
  if (m_lines.size())
    text = m_lines[0].m_text;
}

float CGUITextLayout::GetTextWidth(const CStdStringW &text) const
{
  // NOTE: Assumes a single line of text
  if (!m_font) return 0;
  vector<DWORD> utf32;
  AppendToUTF32(text, (m_font->GetStyle() & 3) << 24, utf32);
  return m_font->GetTextWidth(utf32);
}

void CGUITextLayout::DrawText(CGUIFont *font, float x, float y, DWORD color, DWORD shadowColor, const CStdString &text, DWORD align)
{
  if (!font) return;
  vector<DWORD> utf32;
  AppendToUTF32(text, 0, utf32);
  font->DrawText(x, y, color, shadowColor, utf32, align, 0);
}

void CGUITextLayout::DrawOutlineText(CGUIFont *font, float x, float y, DWORD color, DWORD outlineColor, DWORD outlineWidth, const CStdString &text)
{
  if (!font) return;
  vector<DWORD> utf32;
  AppendToUTF32(text, 0, utf32);
  vector<DWORD> colors;
  colors.push_back(color);
  DrawOutlineText(font, x, y, colors, outlineColor, outlineWidth, utf32, 0, 0);
}

void CGUITextLayout::DrawOutlineText(CGUIFont *font, float x, float y, const vector<DWORD> &colors, DWORD outlineColor, DWORD outlineWidth, const vector<DWORD> &text, DWORD align, float maxWidth)
{
  for (unsigned int i = 1; i < outlineWidth; i++)
  {
    unsigned int ymax = (unsigned int)(sqrt((float)outlineWidth*outlineWidth - i*i) + 0.5f);
    for (unsigned int j = 1; j < ymax; j++)
    {
      font->DrawText(x - i, y + j, outlineColor, 0, text, align, maxWidth);
      font->DrawText(x - i, y - j, outlineColor, 0, text, align, maxWidth);
      font->DrawText(x + i, y + j, outlineColor, 0, text, align, maxWidth);
      font->DrawText(x + i, y - j, outlineColor, 0, text, align, maxWidth);
    }
  }
  font->DrawText(x, y, colors, 0, text, align, maxWidth);
}

void CGUITextLayout::AppendToUTF32(const CStdStringW &utf16, DWORD colStyle, vector<DWORD> &utf32)
{
  // NOTE: Assumes a single line of text
  utf32.reserve(utf32.size() + utf16.size());
  for (unsigned int i = 0; i < utf16.size(); i++)
    utf32.push_back(utf16[i] | colStyle);
}

void CGUITextLayout::utf8ToW(const CStdString &utf8, CStdStringW &utf16)
{
#ifdef WORK_AROUND_NEEDED_FOR_LINE_BREAKS
  // NOTE: This appears to strip \n characters from text.  This may be a consequence of incorrect
  //       expression of the \n in utf8 (we just use character code 10) or it might be something
  //       more sinister.  For now, we use the workaround below.
  CStdStringArray multiLines;
  StringUtils::SplitString(utf8, "\n", multiLines);
  for (unsigned int i = 0; i < multiLines.size(); i++)
  {
    CStdStringW line;
    g_charsetConverter.utf8ToW(multiLines[i], line);
    utf16 += line;
    if (i < multiLines.size() - 1)
      utf16.push_back(L'\n');
  }
#else
  g_charsetConverter.utf8ToW(utf8, utf16);
#endif
}

void CGUITextLayout::AppendToUTF32(const CStdString &utf8, DWORD colStyle, vector<DWORD> &utf32)
{
  CStdStringW utf16;
  utf8ToW(utf8, utf16);
  AppendToUTF32(utf16, colStyle, utf32);
}

void CGUITextLayout::Reset()
{
  m_lines.clear();
  m_lastText.Empty();
}

