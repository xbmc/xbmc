#include "include.h"
#include "GUIFont.h"
#include "GraphicContext.h"
#include "../xbmc/utils/SingleLock.h"

namespace MathUtils {
  inline int round_int (double x);
}
#define ROUND(x) (float)(MathUtils::round_int(x))

CGUIFont::CGUIFont(const CStdString& strFontName, DWORD textColor, DWORD shadowColor, CGUIFontTTF *font)
{
  m_strFontName = strFontName;
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

void CGUIFont::DrawTextWidth(float x, float y, float angle, DWORD dwColor, DWORD dwShadowColor,
                             const WCHAR* strText, float fMaxWidth)
{
  float unneeded, h;
  GetTextExtent(L"W", &unneeded, &h); // This effectively sets h = m_cellHeight for TTF fonts but we don't care about
                                      // truncating height - only width, and as some fonts have some glyphs that may
                                      // dip under the normal cellheight, we should make room for them.
  if (!g_graphicsContext.SetClipRegion(x, y, fMaxWidth, 2*h))
    return; // nothing to render
    
  if (!m_font) return;
  fMaxWidth = ROUND(fMaxWidth / g_graphicsContext.GetGUIScaleX());
  if (!dwColor) dwColor = m_textColor;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  if (angle)
    SetRotation(x, y, angle);
  DWORD textLen = min(wcslen(strText), 2048);
  dwColor = g_graphicsContext.MergeAlpha(dwColor);
  if (dwShadowColor)
  {
    dwShadowColor = g_graphicsContext.MergeAlpha(dwShadowColor);
    m_font->DrawTextInternal(x + 1, y + 1, &dwShadowColor, NULL, strText, textLen, 0, fMaxWidth);
  }
  m_font->DrawTextInternal(x, y, &dwColor, NULL, strText, textLen, 0, fMaxWidth);
  if (angle)
    g_graphicsContext.RemoveTransform();

  g_graphicsContext.RestoreClipRegion();
}


// START backward compatibility
void CGUIFont::DrawTextWidth(FLOAT x, FLOAT y, DWORD dwColor, DWORD dwShadowColor,
                             const WCHAR* strText, float fMaxWidth)
{
  DrawTextWidth(x, y, 0, dwColor, dwShadowColor, strText, fMaxWidth);
}

void CGUIFont::DrawColourTextWidth(float x, float y, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth)
{
  DrawColourTextWidth(x, y, 0, pdw256ColorPalette, numColors, dwShadowColor, strText, pbColours, fMaxWidth);
}

void CGUIFont::DrawText( float x, float y, DWORD dwColor, DWORD dwShadowColor, const WCHAR* strText, DWORD dwFlags, FLOAT fMaxPixelWidth)
{
  DrawText( x, y, 0, dwColor, dwShadowColor, strText, dwFlags, fMaxPixelWidth);
} 

void CGUIFont::DrawScrollingText(float x, float y, DWORD* color, int numColors, DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette)
{
  DrawScrollingText(x, y, 0, color, numColors, dwShadowColor, text, w, scrollInfo, pPalette);
}
// END backward compatibility

void CGUIFont::DrawColourTextWidth(float x, float y, float angle, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth)
{
  if (!m_font) return;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  dwShadowColor = g_graphicsContext.MergeAlpha(dwShadowColor);

  // add cropping box
  float unneeded, h;
  GetTextExtent(L"W", &unneeded, &h); // This effectively sets h = m_cellHeight for TTF fonts but we don't care about
                                      // truncating height - only width, and as some fonts have some glyphs that may
                                      // dip under the normal cellheight, we should make room for them.
  if (!g_graphicsContext.SetClipRegion(x, y, fMaxWidth, 2*h))
    return; // nothing to render

  fMaxWidth = ROUND(fMaxWidth / g_graphicsContext.GetGUIScaleX());
  DWORD *alphaColor = new DWORD[numColors];
  for (int i = 0; i < numColors; i++)
  {
    if (!pdw256ColorPalette[i])
      pdw256ColorPalette[i] = m_textColor;
    alphaColor[i] = g_graphicsContext.MergeAlpha(pdw256ColorPalette[i]);
  }
  if (angle)
    SetRotation(x, y, angle);
  DWORD textLen = min(wcslen(strText), 2048);
  if (dwShadowColor)
    m_font->DrawTextInternal(x + 1, y + 1, &dwShadowColor, NULL, strText, textLen, 0, fMaxWidth);
  m_font->DrawTextInternal(x, y, alphaColor, pbColours, strText, textLen, 0, fMaxWidth);
  delete[] alphaColor;
  if (angle)
    g_graphicsContext.RemoveTransform();

  g_graphicsContext.RestoreClipRegion();
}

void CGUIFont::DrawText( float x, float y, float angle, DWORD dwColor, DWORD dwShadowColor, const WCHAR* strText, DWORD dwFlags, FLOAT fMaxPixelWidth /* = 0 */)
{
  if (!m_font) return;
  float nw = 0.0f, nh = 0.0f;
  fMaxPixelWidth = ROUND(fMaxPixelWidth / g_graphicsContext.GetGUIScaleX());
  if (!dwColor) dwColor = m_textColor;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  if (angle)
    SetRotation(x, y, angle);
  DWORD textLen = min(wcslen(strText), 2048);
  if (dwShadowColor)
  {
    dwShadowColor = g_graphicsContext.MergeAlpha(dwShadowColor);
    m_font->DrawTextInternal(x + 1, y + 1, &dwShadowColor, NULL, strText, textLen, dwFlags, fMaxPixelWidth);
  }
  dwColor = g_graphicsContext.MergeAlpha(dwColor);
  m_font->DrawTextInternal( x, y, &dwColor, NULL, strText, textLen, dwFlags, fMaxPixelWidth );
  if (angle)
    g_graphicsContext.RemoveTransform();
}

float CGUIFont::GetTextWidth(const WCHAR* strText)
{
  float fTextWidth = 0.0f;
  float fTextHeight = 0.0f;

  GetTextExtent( strText, &fTextWidth, &fTextHeight );

  return fTextWidth;
}

float CGUIFont::GetTextHeight(const WCHAR *strText)
{
  float fTextWidth = 0.0f;
  float fTextHeight = 0.0f;

  GetTextExtent( strText, &fTextWidth, &fTextHeight );

  return fTextHeight;
}

void CGUIFont::DrawScrollingText(float x, float y, float angle, DWORD *color, int numColors, DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette /* = NULL */)
{
  if (!m_font) return;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  float unneeded, h;
  float sw = 0;
  GetTextExtent(L" ", &sw, &unneeded);
  unsigned int maxChars = min((long unsigned int)(text.size() + scrollInfo.suffix.size()), (long unsigned int)((w*1.05f)/sw)); //max chars on screen + extra marginchars
  GetTextExtent(L"W", &unneeded, &h); // This effectively sets h = m_cellHeight for TTF fonts but we don't care about
                                      // truncating height - only width, and as some fonts have some glyphs that may
                                      // dip under the normal cellheight, we should make room for them.
  if (text.IsEmpty() || !g_graphicsContext.SetClipRegion(x, y, w, 2*h))
    return; // nothing to render
  w = ROUND(w / g_graphicsContext.GetGUIScaleX());

  if (angle)
    SetRotation(x, y, angle);

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
    WCHAR sz[3];
    if (scrollInfo.characterPos < text.size())
      sz[0] = text[scrollInfo.characterPos];
    else if (scrollInfo.characterPos < text.size() + scrollInfo.suffix.size())
      sz[0] = scrollInfo.suffix[scrollInfo.characterPos - text.size()];
    else
      sz[0] = text[0];
    sz[1] = 0;
    float charWidth = GetTextWidth(sz);
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
  WCHAR *pOutput = new WCHAR[maxChars+1];
  WCHAR *pChar = pOutput;
  BYTE *pOutPalette = NULL;
  if (pPalette)
    pOutPalette = new BYTE[maxChars+1];
  BYTE *pPal = pOutPalette;
  unsigned int pos = scrollInfo.characterPos;
  for (unsigned int i = 0; i < maxChars; i++)
  {
    if (pos >= text.size() + scrollInfo.suffix.size())
      pos = 0;
    if (pos < text.size())
      *pChar++ = text[pos];
    else
      *pChar++ = scrollInfo.suffix[pos - text.size()];
    pos++;
  }
  if (pPalette)
  {
    pos = scrollInfo.characterPos;
    for (unsigned int i = 0; i < maxChars; i++)
    {
      if (pos >= text.size() + scrollInfo.suffix.size())
        pos = 0;
      if (pos < text.size())
        *pPal++ = pPalette[pos];
      else
        *pPal++ = 0;  // suffix uses color 0
      pos++;
    }
  }
  *pChar = L'\0';
  DWORD textLen = min(wcslen(pOutput),2048);
  if (pPalette)
  {
    DWORD *alphaColor = new DWORD[numColors];
    for (int i=0; i < numColors; i++)
    {
      if (!color[i]) color[i] = m_textColor;
      alphaColor[i] = g_graphicsContext.MergeAlpha(color[i]);
    }
    if (dwShadowColor)
    {
      dwShadowColor = g_graphicsContext.MergeAlpha(dwShadowColor);
      m_font->DrawTextInternal(x - scrollInfo.pixelPos + 1, y + 1, &dwShadowColor, NULL, pOutput, textLen, 0, w + scrollInfo.pixelPos + h*2);
    }
    m_font->DrawTextInternal(x - scrollInfo.pixelPos, y, alphaColor, pOutPalette, pOutput, textLen, 0, w + scrollInfo.pixelPos + h*2);
    delete[] alphaColor;
  }
  else
  {
    if (!*color) *color = m_textColor;
    if (dwShadowColor)
    {
      dwShadowColor = g_graphicsContext.MergeAlpha(dwShadowColor);
      m_font->DrawTextInternal(x - scrollInfo.pixelPos + 1, y + 1, &dwShadowColor, NULL, pOutput, textLen, 0, w + scrollInfo.pixelPos + h*2);
    }
    DWORD dwColor = g_graphicsContext.MergeAlpha(*color);
    m_font->DrawTextInternal(x - scrollInfo.pixelPos, y, &dwColor, NULL, pOutput, textLen, 0, w + scrollInfo.pixelPos + h*2);
  }
  delete[] pOutput;
  if (pPalette)
    delete[] pOutPalette;

  if (angle)
    g_graphicsContext.RemoveTransform();

  g_graphicsContext.RestoreClipRegion();
}

// remaps unsupported font glpyhs to other suitable ones
SHORT CGUIFont::RemapGlyph(SHORT letter)
{
  if (letter == 0x2019 || letter == 0x2018) return 0x0027;  // single quotes
  else if (letter == 0x201c || letter == 0x201d) return 0x0022;
  return 0; // no decent character map
}

void CGUIFont::DrawOutlineText(float x, float y, DWORD color, DWORD outlineColor, int outlineWidth, const WCHAR *text, DWORD flags /*= 0L*/, float maxWidth /*= 0.0f*/)
{
  Begin();
  for (int i = 1; i < outlineWidth; i++)
  {
    int ymax = (int)(sqrt((float)outlineWidth*outlineWidth - i*i) + 0.5f);
    for (int j = 1; j < ymax; j++)
    {
      DrawText(x - i, y + j, 0, outlineColor, 0, text, flags, maxWidth);
      DrawText(x - i, y - j, 0, outlineColor, 0, text, flags, maxWidth);
      DrawText(x + i, y + j, 0, outlineColor, 0, text, flags, maxWidth);
      DrawText(x + i, y - j, 0, outlineColor, 0, text, flags, maxWidth);
    }
  }
  DrawText(x, y, 0, color, 0, text, flags, maxWidth);
  End();
}

void CGUIFont::SetRotation(float x, float y, float angle)
{
  static const float degrees_to_radians = 0.01745329252f;
  g_graphicsContext.AddTransform(TransformMatrix::CreateZRotation(angle * degrees_to_radians, x, y, g_graphicsContext.GetScalingPixelRatio()));
}
