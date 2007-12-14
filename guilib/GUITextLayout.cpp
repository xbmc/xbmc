#include "include.h"
#include "GUITextLayout.h"
#include "GUIFont.h"
#include "GUIControl.h"
#include "GUIColorManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/StringUtils.h"

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

  vector<DWORD> parsedText;

  // empty out our previous string
  m_lines.clear();
  m_colors.clear();
  m_colors.push_back(m_textColor);

  // parse the text into our string objects
  ParseText(text, parsedText);

  // if we need to wrap the text, then do so
  if (m_wrap && maxWidth > 0)
    WrapText(parsedText, maxWidth);
  else
    LineBreakText(parsedText);

  // and set our parameters to indicate no further update is required
  m_lastText = text;
  return true;
}

void CGUITextLayout::ParseText(const CStdString &text, vector<DWORD> &parsedText)
{
  if (!m_font)
    return;

  // run through the string, searching for:
  // [B] or [/B] -> toggle bold on and off
  // [I] or [/I] -> toggle italics on and off
  // [COLOR ffab007f] or [/COLOR] -> toggle color on and off
  // [CAPS <option>] or [/CAPS] -> toggle capatilization on and off

  DWORD currentStyle = m_font->GetStyle(); // start with the default font's style
  DWORD currentColor = 0;

  stack<DWORD> colorStack;
  colorStack.push(0);

  // these aren't independent, but that's probably not too much of an issue
  // eg [UPPERCASE]Glah[LOWERCASE]FReD[/LOWERCASE]Georeg[/UPPERCASE] will work (lower case >> upper case)
  // but [LOWERCASE]Glah[UPPERCASE]FReD[/UPPERCASE]Georeg[/LOWERCASE] won't
#define FONT_STYLE_UPPERCASE 4
#define FONT_STYLE_LOWERCASE 8

  int startPos = 0;
  size_t pos = text.Find('[');
  while (pos != CStdString::npos && pos + 1 < text.size())
  {
    DWORD newStyle = 0;
    DWORD newColor = currentColor;
    bool newLine = false;
    // have a [ - check if it's an ON or OFF switch
    bool on(true);
    int endPos = pos++; // finish of string
    if (text[pos] == '/')
    {
      on = false;
      pos++;
    }
    // check for each type
    if (text.Mid(pos,2) == "B]")
    { // bold - finish the current text block and assign the bold state
      newStyle = FONT_STYLE_BOLD;
      pos += 2;
    }
    else if (text.Mid(pos,2) == "I]")
    { // italics
      newStyle = FONT_STYLE_ITALICS;
      pos += 2;
    }
    else if (text.Mid(pos,10) == "UPPERCASE]")
    {
      newStyle = FONT_STYLE_UPPERCASE;
      pos += 10;
    }
    else if (text.Mid(pos,10) == "LOWERCASE]")
    {
      newStyle = FONT_STYLE_LOWERCASE;
      pos += 10;
    }
    else if (text.Mid(pos,3) == "CR]" && on)
    {
      newLine = true;
      pos += 3;
    }
    else if (text.Mid(pos,5) == "COLOR")
    { // color
      size_t finish = text.Find("]", pos + 5);
      if (on && finish != CStdString::npos)
      { // create new color
        newColor = m_colors.size();
        m_colors.push_back(g_colorManager.GetColor(text.Mid(pos + 5, finish - pos - 5)));
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
      CStdString subText = text.Mid(startPos, endPos - startPos);
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
    pos = text.Find('[',pos);
  }
  // now grab the remainder of the string
  CStdString subText = text.Mid(startPos, text.GetLength() - startPos);
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

void CGUITextLayout::WrapText(vector<DWORD> &text, float maxWidth)
{
  if (!m_font)
    return;

  int nMaxLines = (m_maxHeight > 0 && m_font->GetLineHeight() > 0)?(int)(m_maxHeight / m_font->GetLineHeight()):-1;

  m_lines.clear();
  // add \n to the end of the string
  text.push_back(L'\n');
  vector<DWORD> line;
  unsigned int lastSpaceInLine = 0;
  vector<DWORD>::iterator lastSpace = text.begin();
  vector<DWORD>::iterator pos = text.begin();
  while (pos != text.end() && (nMaxLines <= 0 || m_lines.size() <= (size_t)nMaxLines))
  {
    // Get the current letter in the string
    DWORD letter = *pos;

    // Handle the newline character
    if ((letter & 0xffff) == L'\n' )
    {
      float width = m_font->GetTextWidth(line);
      if (width > maxWidth && lastSpaceInLine > 0)
      {
        CGUIString string(line.begin(), line.begin() + lastSpaceInLine, false);
        m_lines.push_back(string);
        lastSpaceInLine++;
        if (lastSpaceInLine < line.size())
        {
          CGUIString string(line.begin() + lastSpaceInLine, line.end(), true);
          m_lines.push_back(string);
        }
      }
      else
      {
        CGUIString string(line.begin(), line.end(), true);
        m_lines.push_back(string);
      }
      line.clear();
      lastSpaceInLine = 0;
      lastSpace = text.begin();
    }
    else
    {
      if ((letter & 0xffff) == L' ')
      {
        float width = m_font->GetTextWidth(line);
        if (width > maxWidth)
        {
          if (lastSpace != text.begin() && lastSpaceInLine > 0)
          {
            CGUIString string(line.begin(), line.begin() + lastSpaceInLine, false);
            m_lines.push_back(string);
            pos = ++lastSpace;
            line.clear();
            lastSpaceInLine = 0;
            lastSpace = text.begin();
            continue;
          }
        }
        // only add spaces if we're not empty
        if (line.size())
        {
          lastSpace = pos;
          lastSpaceInLine = line.size();
          line.push_back(letter);
        }
      }
      else
        line.push_back(letter);
    }
    pos++;
  }
}

void CGUITextLayout::LineBreakText(vector<DWORD> &text)
{
  text.push_back(L'\n');
  vector<DWORD> line;
  vector<DWORD>::iterator pos = text.begin();
  while (pos != text.end())
  {
    // Get the current letter in the string
    DWORD letter = *pos++;

    // Handle the newline character
    if ((letter & 0xffff) == L'\n' )
    { // push back everything up till now
      CGUIString string(line.begin(), line.end(), false);
      m_lines.push_back(string);
      line.clear();
    }
    else
      line.push_back(letter);
  }
}

void CGUITextLayout::GetTextExtent(float &width, float &height)
{
  if (!m_font) return;
  width = 0;
  for (vector<CGUIString>::iterator i = m_lines.begin(); i != m_lines.end(); i++)
  {
    const CGUIString &string = *i;
    float w = m_font->GetTextWidth(string.m_text);
    if (w > width)
      width = w;
  }
  height = m_font->GetTextHeight(m_lines.size());
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

void CGUITextLayout::AppendToUTF32(const CStdString &text, DWORD colStyle, vector<DWORD> &utf32)
{
  // convert text to utf32
#ifdef WORK_AROUND_NEEDED_FOR_LINE_BREAKS
  // NOTE: This appears to strip \n characters from text.  This may be a consequence of incorrect
  //       expression of the \n in utf8 (we just use character code 10) or it might be something
  //       more sinister.  For now, we use the workaround below.
  CStdStringW utf16;
  g_charsetConverter.utf8ToUTF16(text, utf16);
  utf16.Replace(L"\r", L"");
  utf32.reserve(utf32.size() + utf16.size());
  for (unsigned int i = 0; i < utf16.size(); i++)
    utf32.push_back(utf16[i] | colStyle);
#else
  // workaround - break into \n separated, and re-assemble
  CStdStringArray multiLines;
  StringUtils::SplitString(text, "\n", multiLines);
  for (unsigned int i = 0; i < multiLines.size(); i++)
  {
    CStdStringW utf16;
    g_charsetConverter.utf8ToUTF16(multiLines[i], utf16);
    utf16.Replace(L"\r", L"");  // filter out '\r'
    utf32.reserve(utf32.size() + utf16.size() + 1);
    for (unsigned int j = 0; j < utf16.size(); j++)
      utf32.push_back(utf16[j] | colStyle);
    if (i < multiLines.size() - 1)
      utf32.push_back(L'\n');
  }
#endif
}

void CGUITextLayout::Reset()
{
  m_lines.clear();
  m_lastText.Empty();
}

