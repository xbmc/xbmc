#include "include.h"
#include "GUIFont.h"
#include "GraphicContext.h"

#include <math.h>

#include "../xbmc/utils/SingleLock.h"

#ifndef _LINUX
namespace MathUtils {
  int round_int (double x);
}
#define ROUND(x) (float)(MathUtils::round_int(x))
#else
#define ROUND roundf
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#endif

CGUIFont::CGUIFont(const CStdString& strFontName, DWORD style, DWORD textColor, DWORD shadowColor, CGUIFontTTF *font)
{
  m_strFontName = strFontName;
  m_style = style & 3;
  m_textColor = textColor;
  m_shadowColor = shadowColor;
  m_font = font;
  if (m_font)
    m_font->AddReference();
}

CGUIFont::~CGUIFont()
{
  if (m_font)
    m_font->RemoveReference();
}

CStdString& CGUIFont::GetFontName()
{
  return m_strFontName;
}

void CGUIFont::DrawText( float x, float y, const vector<DWORD> &colors, DWORD shadowColor,
                const vector<DWORD> &text, DWORD alignment, float maxPixelWidth)
{
  if (!m_font) return;

  bool clip = maxPixelWidth > 0;
  if (clip && ClippedRegionIsEmpty(x, y, maxPixelWidth, alignment))
    return;
      
  float nw = 0.0f, nh = 0.0f;
  maxPixelWidth = ROUND(maxPixelWidth / g_graphicsContext.GetGUIScaleX());
  vector<DWORD> renderColors;
  for (unsigned int i = 0; i < colors.size(); i++)
    renderColors.push_back(g_graphicsContext.MergeAlpha(colors[i] ? colors[i] : m_textColor));
  if (!shadowColor) shadowColor = m_shadowColor;
  if (shadowColor)
    m_font->DrawTextInternal(x + 1, y + 1, g_graphicsContext.MergeAlpha(shadowColor), text, alignment, maxPixelWidth);
  m_font->DrawTextInternal( x, y, renderColors, text, alignment, maxPixelWidth );

  if (clip)
    g_graphicsContext.RestoreClipRegion();
}

void CGUIFont::DrawScrollingText(float x, float y, const vector<DWORD> &colors, DWORD shadowColor,
                const vector<DWORD> &text, DWORD alignment, float maxWidth, CScrollInfo &scrollInfo)
{
  if (!m_font) return;
  if (!shadowColor) shadowColor = m_shadowColor;

  float spaceWidth = GetCharWidth(L' ');
  unsigned int maxChars = min((long unsigned int)(text.size() + scrollInfo.suffix.size()), (long unsigned int)((maxWidth*1.05f)/spaceWidth)); //max chars on screen + extra marginchars

  if (!text.size() || ClippedRegionIsEmpty(x, y, maxWidth, alignment))
    return; // nothing to render

  maxWidth = ROUND(maxWidth / g_graphicsContext.GetGUIScaleX());

  // draw at our scroll position
  // we handle the scrolling as follows:
  //   We scroll on a per-pixel basis up until we have scrolled the first character outside
  //   of our viewport, whereby we cycle the string around, and reset the scroll position.
  //
  //   m_PixelScroll is the amount in pixels to move the string by.
  //   m_CharacterScroll is the amount in characters to rotate the string by.
  //
  if (!scrollInfo.waitTime)
  {
    // First update our m_PixelScroll...
    DWORD ch;
    if (scrollInfo.characterPos < text.size())
      ch = text[scrollInfo.characterPos];
    else if (scrollInfo.characterPos < text.size() + scrollInfo.suffix.size())
      ch = scrollInfo.suffix[scrollInfo.characterPos - text.size()];
    else
      ch = text[0];
    float charWidth = GetCharWidth(ch);
    float scrollAmount = scrollInfo.pixelSpeed * g_graphicsContext.GetGUIScaleX();
    if (scrollInfo.pixelPos < charWidth - scrollAmount)
      scrollInfo.pixelPos += scrollAmount;
    else
    {
      scrollInfo.pixelPos -= (charWidth - scrollAmount);
      scrollInfo.characterPos++;
      if (scrollInfo.characterPos >= text.size() + scrollInfo.suffix.size())
        scrollInfo.Reset();
    }
  }
  else
    scrollInfo.waitTime--;

  // Now rotate our string as needed, only take a slightly larger then visible part of the text.
  unsigned int pos = scrollInfo.characterPos;
  vector<DWORD> renderText;
  renderText.reserve(maxChars);
  for (unsigned int i = 0; i < maxChars; i++)
  {
    if (pos >= text.size() + scrollInfo.suffix.size())
      pos = 0;
    if (pos < text.size())
      renderText.push_back(text[pos]);
    else
      renderText.push_back(scrollInfo.suffix[pos - text.size()]);
    pos++;
  }

  vector<DWORD> renderColors;
  for (unsigned int i = 0; i < colors.size(); i++)
    renderColors.push_back(g_graphicsContext.MergeAlpha(colors[i] ? colors[i] : m_textColor));

  if (shadowColor)
    m_font->DrawTextInternal(x - scrollInfo.pixelPos + 1, y + 1, g_graphicsContext.MergeAlpha(shadowColor), renderText, alignment, maxWidth + scrollInfo.pixelPos + m_font->m_lineHeight*2);

  m_font->DrawTextInternal(x - scrollInfo.pixelPos, y, renderColors, renderText, alignment, maxWidth + scrollInfo.pixelPos + m_font->m_lineHeight*2);

  g_graphicsContext.RestoreClipRegion();
}

// remaps unsupported font glpyhs to other suitable ones
SHORT CGUIFont::RemapGlyph(SHORT letter)
{
  if (letter == 0x2019 || letter == 0x2018) return 0x0027;  // single quotes
  else if (letter == 0x201c || letter == 0x201d) return 0x0022;
  return 0; // no decent character map
}

bool CGUIFont::ClippedRegionIsEmpty(float x, float y, float width, DWORD alignment) const
{
  if (alignment & XBFONT_CENTER_X)
    x -= width * 0.5f;
  else if (alignment & XBFONT_RIGHT)
    x -= width;
  if (alignment & XBFONT_CENTER_Y)
    y -= m_font->m_lineHeight;

  return !g_graphicsContext.SetClipRegion(x, y, width, 2.0f*m_font->m_lineHeight);
}

