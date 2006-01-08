#include "include.h"
#include "guifont.h"
#include "graphiccontext.h"

CGUIFont::CGUIFont(const CStdString& strFontName, DWORD textColor, DWORD shadowColor, CGUIFontBase *font)
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

void CGUIFont::DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD dwColor, DWORD dwShadowColor,
                             const WCHAR* strText, float fMaxWidth)
{
  if (!m_font) return;
  fOriginX = g_graphicsContext.ScaleFinalXCoord(fOriginX);
  fOriginY = g_graphicsContext.ScaleFinalYCoord(fOriginY);
  fMaxWidth *= g_graphicsContext.ScaleFinalX();
  if (!dwColor) dwColor = m_textColor;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  if (dwShadowColor)
    m_font->DrawTextWidthInternal(fOriginX + 1, fOriginY + 1, angle, g_graphicsContext.MergeAlpha(dwShadowColor), strText, fMaxWidth);
  m_font->DrawTextWidthInternal(fOriginX, fOriginY, angle, g_graphicsContext.MergeAlpha(dwColor), strText, fMaxWidth);
}


// START backward compatibility
void CGUIFont::DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor, DWORD dwShadowColor,
                             const WCHAR* strText, float fMaxWidth)
{
  DrawTextWidth(fOriginX, fOriginY, CAngle(), dwColor, dwShadowColor, strText, fMaxWidth);
}

void CGUIFont::DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth)
{
  DrawColourTextWidth(fOriginX, fOriginY, CAngle(), pdw256ColorPalette, numColors, dwShadowColor, strText, pbColours, fMaxWidth);
}

void CGUIFont::DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, DWORD dwShadowColor, const WCHAR* strText, DWORD dwFlags, FLOAT fMaxPixelWidth /* = 0 */)
{
  DrawText( sx, sy, CAngle(), dwColor, dwShadowColor, strText, dwFlags, fMaxPixelWidth);
}

void CGUIFont::DrawScrollingText(float x, float y, DWORD* color, int numColors, DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette)
{
  DrawScrollingText(x, y, CAngle(), color, numColors, dwShadowColor, text, w, scrollInfo, pPalette);
}
// END backward compatibility

void CGUIFont::DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth)
{
  if (!m_font) return;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  dwShadowColor = g_graphicsContext.MergeAlpha(dwShadowColor);

  fOriginX = g_graphicsContext.ScaleFinalXCoord(fOriginX);
  fOriginY = g_graphicsContext.ScaleFinalYCoord(fOriginY);
  DWORD *alphaColor = new DWORD[numColors];
  for (int i = 0; i < numColors; i++)
  {
    if (!pdw256ColorPalette[i])
      pdw256ColorPalette[i] = m_textColor;
    alphaColor[i] = g_graphicsContext.MergeAlpha(pdw256ColorPalette[i]);
  }
  fMaxWidth *= g_graphicsContext.ScaleFinalX();
  m_font->DrawColourTextWidth(fOriginX, fOriginY, angle, alphaColor, numColors, dwShadowColor, strText, pbColours, fMaxWidth);
  delete[] alphaColor;
}

void CGUIFont::DrawText( FLOAT sx, FLOAT sy, const CAngle &angle, DWORD dwColor, DWORD dwShadowColor, const WCHAR* strText, DWORD dwFlags, FLOAT fMaxPixelWidth /* = 0 */)
{
  if (!m_font) return;
  float nw = 0.0f, nh = 0.0f;
  sx = g_graphicsContext.ScaleFinalXCoord(sx);
  sy = g_graphicsContext.ScaleFinalYCoord(sy);
  fMaxPixelWidth *= g_graphicsContext.ScaleFinalX();
  if (!dwColor) dwColor = m_textColor;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  if (dwShadowColor)
    m_font->DrawTextImpl(sx + 1, sy + 1, angle, g_graphicsContext.MergeAlpha(dwShadowColor), strText, wcslen( strText ), dwFlags, fMaxPixelWidth);
  m_font->DrawTextImpl( sx, sy, angle, g_graphicsContext.MergeAlpha(dwColor), strText, wcslen( strText ), dwFlags, fMaxPixelWidth );
}

FLOAT CGUIFont::GetTextWidth(const WCHAR* strText)
{
  FLOAT fTextWidth = 0.0f;
  FLOAT fTextHeight = 0.0f;

  GetTextExtent( strText, &fTextWidth, &fTextHeight );

  return fTextWidth;
}

inline void CGUIFont::GetTextExtent(const WCHAR *strText, FLOAT *pWidth, FLOAT *pHeight, BOOL bFirstLineOnly /* = 0 */)
{
  if (!m_font) return;
  m_font->GetTextExtentInternal(strText, pWidth, pHeight, bFirstLineOnly);
  *pWidth /= g_graphicsContext.ScaleFinalX();
  *pHeight /= g_graphicsContext.ScaleFinalY();
}

void CGUIFont::DrawScrollingText(float x, float y, const CAngle &angle, DWORD *color, int numColors, DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette /* = NULL */)
{
  if (!m_font) return;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  float unneeded, h;
  float sw = 0;
  GetTextExtent(L" ", &sw, &unneeded);
  unsigned int maxChars = min(text.size(), unsigned int((w*1.05f)/sw)); //max chars on screen + extra marginchars
  GetTextExtent(L"W", &unneeded, &h);
  if (!g_graphicsContext.SetViewPort(x, y, w, h))
    return; // nothing to render
  x = g_graphicsContext.ScaleFinalXCoord(x);
  y = g_graphicsContext.ScaleFinalYCoord(y);
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
    else
      sz[0] = L' ';
    sz[1] = 0;
    float charWidth;
    m_font->GetTextExtentInternal(sz, &charWidth, &unneeded);
    if (scrollInfo.pixelPos < charWidth - 1)
      scrollInfo.pixelPos++;
    else
    {
      scrollInfo.pixelPos = 0;
      scrollInfo.characterPos++;
      if (scrollInfo.characterPos > text.size())
        scrollInfo.Reset();
    }
  }
  else
    scrollInfo.waitTime--;
  // Now rotate our string as needed, only take a slightly larger then visible part of the text.
  WCHAR *pOutput = new WCHAR[maxChars+2];
  WCHAR *pChar = pOutput;
  BYTE *pOutPalette = NULL;
  if (pPalette)
    pOutPalette = new BYTE[maxChars+2];
  BYTE *pPal = pOutPalette;
  unsigned int pos = scrollInfo.characterPos;
  for (unsigned int i = 0; i <= maxChars; i++)
  {
    if (pos < text.size())
    {
      *pChar++ = text[pos];
      pos++;
    }
    else
    {
      *pChar++ = L' ';
      pos = 0;
    }
  }
  if (pPalette)
  {
    pos = scrollInfo.characterPos;
    for (unsigned int i = 0; i <= maxChars; i++)
    {
      if (pos < text.size())
      {
        *pPal++ = pPalette[pos];
        pos++;
      }
      else
      {
        *pPal++ =0;
        pos = 0;
      }
    }
  }
  *pChar = L'\0';
  if (pPalette)
  {
    DWORD *alphaColor = new DWORD[numColors];
    for (int i=0; i < numColors; i++)
    {
      if (!color[i]) color[i] = m_textColor;
      alphaColor[i] = g_graphicsContext.MergeAlpha(color[i]);
    }
    if (dwShadowColor)
      m_font->DrawTextImpl(x - scrollInfo.pixelPos + 1, y + 1, angle, g_graphicsContext.MergeAlpha(dwShadowColor), pOutput, wcslen(pOutput), 0, w + scrollInfo.pixelPos + h*2);
    m_font->DrawColourTextImpl(x - scrollInfo.pixelPos, y, angle, color, pOutput, pOutPalette, wcslen(pOutput), 0, w + scrollInfo.pixelPos + h*2);
    delete[] alphaColor;
  }
  else
  {
    if (!*color) *color = m_textColor;
    if (dwShadowColor)
      m_font->DrawTextImpl(x - scrollInfo.pixelPos + 1, y + 1, angle, g_graphicsContext.MergeAlpha(dwShadowColor), pOutput, wcslen(pOutput), 0, w + scrollInfo.pixelPos + h*2);
    m_font->DrawTextWidthInternal(x - scrollInfo.pixelPos, y, angle, g_graphicsContext.MergeAlpha(*color), pOutput, w + scrollInfo.pixelPos + h*2);
  }
  delete[] pOutput;
  if (pPalette)
    delete[] pOutPalette;
  g_graphicsContext.RestoreViewPort();
}
