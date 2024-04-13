/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITextLayout.h"

#include "GUIColorManager.h"
#include "GUIComponent.h"
#include "GUIControl.h"
#include "GUIFont.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <limits>

CGUIString::CGUIString(iString start, iString end, bool carriageReturn)
{
  m_text.assign(start, end);
  m_carriageReturn = carriageReturn;
}

std::string CGUIString::GetAsString() const
{
  std::string text;
  for (unsigned int i = 0; i < m_text.size(); i++)
    text += (char)(m_text[i] & 0xff);
  return text;
}

CGUITextLayout::CGUITextLayout(CGUIFont *font, bool wrap, float fHeight, CGUIFont *borderFont)
{
  m_varFont = m_font = font;
  m_borderFont = borderFont;
  m_textColor = 0;
  m_wrap = wrap;
  m_maxHeight = fHeight;
  m_textWidth = 0;
  m_textHeight = 0;
  m_lastUpdateW = false;
}

void CGUITextLayout::SetWrap(bool bWrap)
{
  m_wrap = bWrap;
}

void CGUITextLayout::Render(float x,
                            float y,
                            float angle,
                            UTILS::COLOR::Color color,
                            UTILS::COLOR::Color shadowColor,
                            uint32_t alignment,
                            float maxWidth,
                            bool solid)
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
    CServiceBroker::GetWinSystem()->GetGfxContext().AddTransform(TransformMatrix::CreateZRotation(angle * degrees_to_radians, x, y, CServiceBroker::GetWinSystem()->GetGfxContext().GetScalingPixelRatio()));
  }
  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;
    alignment &= ~XBFONT_CENTER_Y;
  }
  m_font->Begin();
  for (const auto& string : m_lines)
  {
    uint32_t align = alignment;
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
    CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
}

bool CGUITextLayout::UpdateScrollinfo(CScrollInfo &scrollInfo)
{
  if (!m_font)
    return false;
  if (m_lines.empty())
    return false;

  return m_font->UpdateScrollInfo(m_lines[0].m_text, scrollInfo);
}


void CGUITextLayout::RenderScrolling(float x,
                                     float y,
                                     float angle,
                                     UTILS::COLOR::Color color,
                                     UTILS::COLOR::Color shadowColor,
                                     uint32_t alignment,
                                     float maxWidth,
                                     const CScrollInfo& scrollInfo)
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
    CServiceBroker::GetWinSystem()->GetGfxContext().AddTransform(TransformMatrix::CreateZRotation(angle * degrees_to_radians, x, y, CServiceBroker::GetWinSystem()->GetGfxContext().GetScalingPixelRatio()));
  }
  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;
    alignment &= ~XBFONT_CENTER_Y;
  }
  m_font->Begin();
  // NOTE: This workaround is needed as otherwise multi-line text that scrolls
  //       will scroll in proportion to the number of lines.  Ideally we should
  //       do the DrawScrollingText calculation here.  This probably won't make
  //       any difference to the smoothness of scrolling though which will be
  //       jumpy with this sort of thing.  It's not exactly a well used situation
  //       though, so this hack is probably OK.
  for (const auto& string : m_lines)
  {
    m_font->DrawScrollingText(x, y, m_colors, shadowColor, string.m_text, alignment, maxWidth, scrollInfo);
    y += m_font->GetLineHeight();
  }
  m_font->End();
  if (angle)
    CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
}

void CGUITextLayout::RenderOutline(float x,
                                   float y,
                                   UTILS::COLOR::Color color,
                                   UTILS::COLOR::Color outlineColor,
                                   uint32_t alignment,
                                   float maxWidth)
{
  if (!m_font)
    return;

  // set the outline color
  std::vector<UTILS::COLOR::Color> outlineColors;
  if (m_colors.size())
    outlineColors.push_back(outlineColor);

  // center our text vertically
  if (alignment & XBFONT_CENTER_Y)
  {
    y -= m_font->GetTextHeight(m_lines.size()) * 0.5f;
    alignment &= ~XBFONT_CENTER_Y;
  }
  if (m_borderFont)
  {
    // adjust so the baselines of the fonts align
    float by = y + m_font->GetTextBaseLine() - m_borderFont->GetTextBaseLine();
    m_borderFont->Begin();
    for (const auto& string : m_lines)
    {
      uint32_t align = alignment;
      if (align & XBFONT_JUSTIFIED && string.m_carriageReturn)
        align &= ~XBFONT_JUSTIFIED;
      // text centered horizontally must be computed using the original font, not the bordered
      // font, as the bordered font will be wider, and thus will end up uncentered.
      //! @todo We should really have a better way to handle text extent - at the moment we assume
      //!       that text is rendered from a posx, posy, width, and height which isn't enough to
      //!       accurately position text. We need a vertical and horizontal offset of the baseline
      //!       and cursor as well.
      float bx = x;
      if (align & XBFONT_CENTER_X)
      {
        bx -= m_font->GetTextWidth(string.m_text) * 0.5f;
        align &= ~XBFONT_CENTER_X;
      }

      // don't pass maxWidth through to the renderer for the same reason above: it will cause clipping
      // on the left.
      m_borderFont->DrawText(bx, by, outlineColors, 0, string.m_text, align, 0);
      by += m_borderFont->GetLineHeight();
    }
    m_borderFont->End();
  }

  // set the main text color
  if (m_colors.size())
    m_colors[0] = color;

  m_font->Begin();
  for (const auto& string : m_lines)
  {
    uint32_t align = alignment;
    if (align & XBFONT_JUSTIFIED && string.m_carriageReturn)
      align &= ~XBFONT_JUSTIFIED;

    // don't pass maxWidth through to the renderer for the reason above.
    m_font->DrawText(x, y, m_colors, 0, string.m_text, align, 0);
    y += m_font->GetLineHeight();
  }
  m_font->End();
}

bool CGUITextLayout::Update(const std::string &text, float maxWidth, bool forceUpdate /*= false*/, bool forceLTRReadingOrder /*= false*/)
{
  if (text == m_lastUtf8Text && !forceUpdate && !m_lastUpdateW)
    return false;

  m_lastUtf8Text = text;
  m_lastUpdateW = false;
  std::wstring utf16;
  g_charsetConverter.utf8ToW(text, utf16, false);
  UpdateCommon(utf16, maxWidth, forceLTRReadingOrder);
  return true;
}

bool CGUITextLayout::UpdateW(const std::wstring &text, float maxWidth /*= 0*/, bool forceUpdate /*= false*/, bool forceLTRReadingOrder /*= false*/)
{
  if (text == m_lastText && !forceUpdate && m_lastUpdateW)
    return false;

  m_lastText = text;
  m_lastUpdateW = true;
  UpdateCommon(text, maxWidth, forceLTRReadingOrder);
  return true;
}

void CGUITextLayout::UpdateCommon(const std::wstring &text, float maxWidth, bool forceLTRReadingOrder)
{
  // parse the text for style information
  vecText parsedText;
  std::vector<UTILS::COLOR::Color> colors;
  ParseText(text, m_font ? m_font->GetStyle() : 0, m_textColor, colors, parsedText);

  // and update
  UpdateStyled(parsedText, colors, maxWidth, forceLTRReadingOrder);
}

void CGUITextLayout::UpdateStyled(const vecText& text,
                                  const std::vector<UTILS::COLOR::Color>& colors,
                                  float maxWidth,
                                  bool forceLTRReadingOrder)
{
  // empty out our previous string
  m_lines.clear();
  m_colors = colors;

  // if we need to wrap the text, then do so
  if (m_wrap)
    WrapText(text, maxWidth);
  else
    LineBreakText(text, m_lines);

  // remove any trailing blank lines
  while (!m_lines.empty() && m_lines.back().m_text.empty())
    m_lines.pop_back();

  BidiTransform(m_lines, forceLTRReadingOrder);

  // and cache the width and height for later reading
  CalcTextExtent();
}

// BidiTransform is used to handle RTL text flipping in the string
void CGUITextLayout::BidiTransform(std::vector<CGUIString>& lines, bool forceLTRReadingOrder)
{
  for (unsigned int i = 0; i < lines.size(); i++)
  {
    CGUIString& line = lines[i];
    unsigned int lineLength = line.m_text.size();
    std::wstring logicalText;
    vecText style;

    logicalText.reserve(lineLength);
    style.reserve(lineLength);

    // Separate the text and style for the input styled text
    for (const auto& it : line.m_text)
    {
      logicalText.push_back((wchar_t)(it & 0xffff));
      style.push_back(it & 0xffff0000);
    }

    // Allocate memory for visual to logical map and call bidi
    int* visualToLogicalMap = new (std::nothrow) int[lineLength + 1]();
    std::wstring visualText = BidiFlip(logicalText, forceLTRReadingOrder, visualToLogicalMap);

    vecText styledVisualText;
    styledVisualText.reserve(lineLength);

    // If memory allocation failed, fallback to text with no styling
    if (!visualToLogicalMap)
    {
      for (unsigned int j = 0; j < visualText.size(); j++)
      {
        styledVisualText.push_back(visualText[j]);
      }
    }
    else
    {
      for (unsigned int j = 0; j < visualText.size(); j++)
      {
        styledVisualText.push_back(style[visualToLogicalMap[j]] | visualText[j]);
      }
    }

    delete[] visualToLogicalMap;

    // replace the original line with the processed one
    lines[i] = CGUIString(styledVisualText.begin(), styledVisualText.end(), line.m_carriageReturn);
  }
}

std::wstring CGUITextLayout::BidiFlip(const std::wstring& text,
                                      bool forceLTRReadingOrder,
                                      int* visualToLogicalMap /*= nullptr*/)
{
  std::wstring visualText;
  std::u32string utf32logical;
  std::u32string utf32visual;

  // Convert to utf32, call bidi then convert the result back to utf16
  g_charsetConverter.wToUtf32(text, utf32logical);
  g_charsetConverter.utf32logicalToVisualBiDi(utf32logical, utf32visual, forceLTRReadingOrder,
                                              false,
                                              visualToLogicalMap);
  g_charsetConverter.utf32ToW(utf32visual, visualText);

  return visualText;
}

void CGUITextLayout::Filter(std::string &text)
{
  std::wstring utf16;
  g_charsetConverter.utf8ToW(text, utf16, false);
  std::vector<UTILS::COLOR::Color> colors;
  vecText parsedText;
  ParseText(utf16, 0, 0xffffffff, colors, parsedText);
  utf16.clear();
  for (unsigned int i = 0; i < parsedText.size(); i++)
    utf16 += (wchar_t)(0xffff & parsedText[i]);
  g_charsetConverter.wToUTF8(utf16, text);
}

void CGUITextLayout::ParseText(const std::wstring& text,
                               uint32_t defaultStyle,
                               UTILS::COLOR::Color defaultColor,
                               std::vector<UTILS::COLOR::Color>& colors,
                               vecText& parsedText)
{
  // run through the string, searching for:
  // [B] or [/B] -> toggle bold on and off
  // [I] or [/I] -> toggle italics on and off
  // [COLOR ffab007f] or [/COLOR] -> toggle color on and off
  // [CAPS <option>] or [/CAPS] -> toggle capatilization on and off
  // [TABS] tab amount [/TABS] -> add tabulator space in view

  uint32_t currentStyle = defaultStyle; // start with the default font's style
  UTILS::COLOR::Color currentColor = 0;

  colors.push_back(defaultColor);
  std::stack<UTILS::COLOR::Color> colorStack;
  colorStack.push(0);

  // these aren't independent, but that's probably not too much of an issue
  // eg [UPPERCASE]Glah[LOWERCASE]FReD[/LOWERCASE]Georeg[/UPPERCASE] will work (lower case >> upper case)
  // but [LOWERCASE]Glah[UPPERCASE]FReD[/UPPERCASE]Georeg[/LOWERCASE] won't

  int startPos = 0;
  size_t pos = text.find(L'[');
  while (pos != std::string::npos && pos + 1 < text.size())
  {
    uint32_t newStyle = 0;
    UTILS::COLOR::Color newColor = currentColor;
    bool colorTagChange = false;
    bool newLine = false;
    int tabs = 0;
    // have a [ - check if it's an ON or OFF switch
    bool on(true);
    size_t endPos = pos++; // finish of string
    if (text[pos] == L'/')
    {
      on = false;
      pos++;
    }
    // check for each type
    if (text.compare(pos, 2, L"B]") == 0)
    { // bold - finish the current text block and assign the bold state
      pos += 2;
      if ((on && text.find(L"[/B]",pos) != std::string::npos) ||          // check for a matching end point
         (!on && (currentStyle & FONT_STYLE_BOLD)))       // or matching start point
        newStyle = FONT_STYLE_BOLD;
    }
    else if (text.compare(pos, 2, L"I]") == 0)
    { // italics
      pos += 2;
      if ((on && text.find(L"[/I]", pos) != std::string::npos) ||          // check for a matching end point
         (!on && (currentStyle & FONT_STYLE_ITALICS)))    // or matching start point
        newStyle = FONT_STYLE_ITALICS;
    }
    else if (text.compare(pos, 10, L"UPPERCASE]") == 0)
    {
      pos += 10;
      if ((on && text.find(L"[/UPPERCASE]", pos) != std::string::npos) ||  // check for a matching end point
         (!on && (currentStyle & FONT_STYLE_UPPERCASE)))  // or matching start point
        newStyle = FONT_STYLE_UPPERCASE;
    }
    else if (text.compare(pos, 10, L"LOWERCASE]") == 0)
    {
      pos += 10;
      if ((on && text.find(L"[/LOWERCASE]", pos) != std::string::npos) ||  // check for a matching end point
         (!on && (currentStyle & FONT_STYLE_LOWERCASE)))  // or matching start point
        newStyle = FONT_STYLE_LOWERCASE;
    }
    else if (text.compare(pos, 11, L"CAPITALIZE]") == 0)
    {
      pos += 11;
      if ((on && text.find(L"[/CAPITALIZE]", pos) != std::string::npos) ||  // check for a matching end point
         (!on && (currentStyle & FONT_STYLE_CAPITALIZE)))  // or matching start point
        newStyle = FONT_STYLE_CAPITALIZE;
    }
    else if (text.compare(pos, 6, L"LIGHT]") == 0)
    {
      pos += 6;
      if ((on && text.find(L"[/LIGHT]", pos) != std::string::npos) ||
         (!on && (currentStyle & FONT_STYLE_LIGHT)))
        newStyle = FONT_STYLE_LIGHT;
    }
    else if (text.compare(pos, 5, L"TABS]") == 0 && on)
    {
      pos += 5;
      const size_t end = text.find(L"[/TABS]", pos);
      if (end != std::string::npos)
      {
        std::string t;
        g_charsetConverter.wToUTF8(text.substr(pos), t);
        tabs = atoi(t.c_str());
        pos = end + 7;
      }
    }
    else if (text.compare(pos, 3, L"CR]") == 0 && on)
    {
      newLine = true;
      pos += 3;
    }
    else if (text.compare(pos,5, L"COLOR") == 0)
    { // color
      size_t finish = text.find(L']', pos + 5);
      if (on && finish != std::string::npos && text.find(L"[/COLOR]",finish) != std::string::npos)
      {
        std::string t;
        g_charsetConverter.wToUTF8(text.substr(pos + 5, finish - pos - 5), t);
        UTILS::COLOR::Color color = CServiceBroker::GetGUI()->GetColorManager().GetColor(t);
        const auto& it = std::find(colors.begin(), colors.end(), color);
        if (it == colors.end())
        { // create new color
          if (colors.size() <= 0xFF)
          {
            newColor = colors.size();
            colors.push_back(color);
          }
          else // we have only 8 bits for color index, fallback to first color if reach max.
            newColor = 0;
        }
        else
          // reuse existing color
          newColor = it - colors.begin();
        colorStack.push(newColor);
        colorTagChange = true;
      }
      else if (!on && finish == pos + 5 && colorStack.size() > 1)
      { // revert to previous color
        colorStack.pop();
        newColor = colorStack.top();
        colorTagChange = true;
      }
      if (finish != std::string::npos)
        pos = finish + 1;
    }

    if (newStyle || colorTagChange || newLine || tabs)
    { // we have a new style or a new color, so format up the previous segment
      std::wstring subText = text.substr(startPos, endPos - startPos);
      if (currentStyle & FONT_STYLE_UPPERCASE)
        StringUtils::ToUpper(subText);
      if (currentStyle & FONT_STYLE_LOWERCASE)
        StringUtils::ToLower(subText);
      if (currentStyle & FONT_STYLE_CAPITALIZE)
        StringUtils::ToCapitalize(subText);
      AppendToUTF32(subText, ((currentStyle & FONT_STYLE_MASK) << 24) | (currentColor << 16), parsedText);
      if (newLine)
        parsedText.push_back(L'\n');
      for (int i = 0; i < tabs; ++i)
        parsedText.push_back(L'\t');

      // and switch to the new style
      startPos = pos;
      currentColor = newColor;
      if (on)
        currentStyle |= newStyle;
      else
        currentStyle &= ~newStyle;
    }
    pos = text.find(L'[', pos);
  }
  // now grab the remainder of the string
  std::wstring subText = text.substr(startPos);
  if (currentStyle & FONT_STYLE_UPPERCASE)
    StringUtils::ToUpper(subText);
  if (currentStyle & FONT_STYLE_LOWERCASE)
    StringUtils::ToLower(subText);
  if (currentStyle & FONT_STYLE_CAPITALIZE)
    StringUtils::ToCapitalize(subText);
  AppendToUTF32(subText, ((currentStyle & FONT_STYLE_MASK) << 24) | (currentColor << 16), parsedText);
}

void CGUITextLayout::SetMaxHeight(float fHeight)
{
  m_maxHeight = fHeight;
}

void CGUITextLayout::WrapText(const vecText &text, float maxWidth)
{
  if (!m_font)
    return;

  m_lines.clear();

  if (maxWidth < 0)
  {
    CLog::LogF(LOGWARNING, "Cannot wrap the text due to invalid max width value.");
    return;
  }

  if (maxWidth == 0) // Unlimited max width
  {
    LineBreakText(text, m_lines);
    return;
  }

  std::vector<CGUIString> lines;
  LineBreakText(text, lines);

  size_t nMaxLines;
  if (m_maxHeight > 0 && m_font->GetLineHeight() > 0)
    nMaxLines = static_cast<size_t>(std::ceil(m_maxHeight / m_font->GetLineHeight()));
  else
    nMaxLines = std::numeric_limits<size_t>::max();

  // Split lines that exceed the maximum width,
  // lines can be split by last space char or if there are no spaces by character
  // to mimics the line behavior of word processors
  for (const CGUIString& line : lines)
  {
    if (m_lines.size() >= nMaxLines)
      return;

    if (line.m_text.empty()) // Blank line
    {
      m_lines.emplace_back(line);
      continue;
    }

    vecText::const_iterator pos = line.m_text.begin();
    vecText::const_iterator lastBeginPos = line.m_text.begin();
    vecText::const_iterator lastSpacePos = line.m_text.end();
    vecText curLine;

    while (pos < line.m_text.end())
    {
      // Get the current letter in the string
      const character_t& letter = *pos;

      if (CanWrapAtLetter(letter)) // Check for a space char
        lastSpacePos = pos;

      curLine.emplace_back(letter);

      const float currWidth = m_font->GetTextWidth(curLine);

      if (currWidth > maxWidth)
      {
        if (lastSpacePos > pos) // No space char where split the line, so split by char
        {
          // If the pos is equal to lastBeginPos, maxWidth is not large enough to contain 1 character
          // Push a line with the single character and move on to the next character.
          if (pos == lastBeginPos)
            ++pos;

          CGUIString linePart{lastBeginPos, pos, false};
          m_lines.emplace_back(linePart);
        }
        else
        {
          CGUIString linePart{lastBeginPos, lastSpacePos, false};
          m_lines.emplace_back(linePart);

          pos = lastSpacePos + 1;
          lastSpacePos = line.m_text.end();
        }

        curLine.clear();
        lastBeginPos = pos;

        if (m_lines.size() >= nMaxLines)
          return;

        continue;
      }

      ++pos;
    }

    // Add the remaining text part
    if (!curLine.empty())
    {
      CGUIString linePart{curLine.begin(), curLine.end(), false};
      m_lines.emplace_back(linePart);
    }

    // Restore carriage return marker for the end of paragraph
    if (!m_lines.empty())
      m_lines.back().m_carriageReturn = line.m_carriageReturn;
  }
}

void CGUITextLayout::LineBreakText(const vecText &text, std::vector<CGUIString> &lines)
{
  int nMaxLines = (m_maxHeight > 0 && m_font && m_font->GetLineHeight() > 0)?(int)ceilf(m_maxHeight / m_font->GetLineHeight()):-1;
  vecText::const_iterator lineStart = text.begin();
  vecText::const_iterator pos = text.begin();
  while (pos != text.end() && (nMaxLines <= 0 || lines.size() < (size_t)nMaxLines))
  {
    // Get the current letter in the string
    character_t letter = *pos;

    // Handle the newline character
    if ((letter & 0xffff) == L'\n' )
    { // push back everything up till now
      CGUIString string(lineStart, pos, true);
      lines.push_back(string);
      lineStart = pos + 1;
    }
    ++pos;
  }
  // handle the last line if non-empty
  if (lineStart < text.end() && (nMaxLines <= 0 || lines.size() < (size_t)nMaxLines))
  {
    CGUIString string(lineStart, text.end(), true);
    lines.push_back(string);
  }
}

void CGUITextLayout::GetTextExtent(float &width, float &height) const
{
  width = m_textWidth;
  height = m_textHeight;
}

void CGUITextLayout::CalcTextExtent()
{
  m_textWidth = 0;
  m_textHeight = 0;
  if (!m_font) return;

  for (const auto& string : m_lines)
  {
    float w = m_font->GetTextWidth(string.m_text);
    if (w > m_textWidth)
      m_textWidth = w;
  }
  m_textHeight = m_font->GetTextHeight(m_lines.size());
}

unsigned int CGUITextLayout::GetTextLength() const
{
  unsigned int length = 0;
  for (const auto& string : m_lines)
    length += string.m_text.size();
  return length;
}

void CGUITextLayout::GetFirstText(vecText &text) const
{
  text.clear();
  if (m_lines.size())
    text = m_lines[0].m_text;
}

float CGUITextLayout::GetTextWidth(const std::wstring &text) const
{
  // NOTE: Assumes a single line of text
  if (!m_font) return 0;
  vecText utf32;
  AppendToUTF32(text, (m_font->GetStyle() & FONT_STYLE_MASK) << 24, utf32);
  return m_font->GetTextWidth(utf32);
}

float CGUITextLayout::GetTextWidth(const vecText& text) const
{
  return m_font->GetTextWidth(text);
}

std::string CGUITextLayout::GetText() const
{
  if (m_lastUpdateW)
  {
    std::string utf8;
    g_charsetConverter.wToUTF8(m_lastText, utf8);
    return utf8;
  }
  return m_lastUtf8Text;
}

void CGUITextLayout::DrawText(CGUIFont* font,
                              float x,
                              float y,
                              UTILS::COLOR::Color color,
                              UTILS::COLOR::Color shadowColor,
                              const std::string& text,
                              uint32_t align)
{
  if (!font) return;
  vecText utf32;
  AppendToUTF32(text, 0, utf32);
  font->DrawText(x, y, color, shadowColor, utf32, align, 0);
}

void CGUITextLayout::AppendToUTF32(const std::wstring &utf16, character_t colStyle, vecText &utf32)
{
  // NOTE: Assumes a single line of text
  utf32.reserve(utf32.size() + utf16.size());
  for (unsigned int i = 0; i < utf16.size(); i++)
    utf32.push_back(utf16[i] | colStyle);
}

void CGUITextLayout::AppendToUTF32(const std::string &utf8, character_t colStyle, vecText &utf32)
{
  std::wstring utf16;
  // no need to bidiflip here - it's done in BidiTransform above
  g_charsetConverter.utf8ToW(utf8, utf16, false);
  AppendToUTF32(utf16, colStyle, utf32);
}

void CGUITextLayout::Reset()
{
  m_lines.clear();
  m_lastText.clear();
  m_lastUtf8Text.clear();
  m_textWidth = m_textHeight = 0;
}


