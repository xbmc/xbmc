#include "include.h"
#include "guifont.h"
#include "graphiccontext.h"
#include "../xbmc/Utils/SingleLock.h"

#define ROUND(x) floorf(x + 0.5f)

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
  fMaxWidth = ROUND(fMaxWidth * g_graphicsContext.GetGUIScaleX());
  if (!dwColor) dwColor = m_textColor;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  if (dwShadowColor)
    m_font->DrawTextWidthInternal(fOriginX + 1, fOriginY + 1, Transform(angle), g_graphicsContext.MergeAlpha(dwShadowColor), strText, fMaxWidth);
  m_font->DrawTextWidthInternal(fOriginX, fOriginY, Transform(angle), g_graphicsContext.MergeAlpha(dwColor), strText, fMaxWidth);
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

  fMaxWidth = ROUND(fMaxWidth * g_graphicsContext.GetGUIScaleX());
  DWORD *alphaColor = new DWORD[numColors];
  for (int i = 0; i < numColors; i++)
  {
    if (!pdw256ColorPalette[i])
      pdw256ColorPalette[i] = m_textColor;
    alphaColor[i] = g_graphicsContext.MergeAlpha(pdw256ColorPalette[i]);
  }
  m_font->DrawColourTextWidth(fOriginX, fOriginY, Transform(angle), alphaColor, numColors, dwShadowColor, strText, pbColours, fMaxWidth);
  delete[] alphaColor;
}

void CGUIFont::DrawText( FLOAT sx, FLOAT sy, const CAngle &angle, DWORD dwColor, DWORD dwShadowColor, const WCHAR* strText, DWORD dwFlags, FLOAT fMaxPixelWidth /* = 0 */)
{
  if (!m_font) return;
  float nw = 0.0f, nh = 0.0f;
  fMaxPixelWidth = ROUND(fMaxPixelWidth * g_graphicsContext.GetGUIScaleX());
  if (!dwColor) dwColor = m_textColor;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  if (dwShadowColor)
    m_font->DrawTextImpl(sx + 1, sy + 1, Transform(angle), g_graphicsContext.MergeAlpha(dwShadowColor), strText, wcslen( strText ), dwFlags, fMaxPixelWidth);
  m_font->DrawTextImpl( sx, sy, Transform(angle), g_graphicsContext.MergeAlpha(dwColor), strText, wcslen( strText ), dwFlags, fMaxPixelWidth );
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

inline void CGUIFont::GetTextExtent(const WCHAR *strText, FLOAT *pWidth, FLOAT *pHeight, BOOL bFirstLineOnly /* = 0 */)
{
  if (!m_font) return;
  CSingleLock lock(g_graphicsContext);
  m_font->GetTextExtentInternal(strText, pWidth, pHeight, bFirstLineOnly);
  *pWidth /= g_graphicsContext.GetGUIScaleX();
  *pHeight /= g_graphicsContext.GetGUIScaleY();
}

void CGUIFont::DrawScrollingText(float x, float y, const CAngle &angle, DWORD *color, int numColors, DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette /* = NULL */)
{
  if (!m_font) return;
  if (!dwShadowColor) dwShadowColor = m_shadowColor;
  float unneeded, h;
  float sw = 0;
  GetTextExtent(L" ", &sw, &unneeded);
  unsigned int maxChars = min(text.size() + scrollInfo.suffix.size(), unsigned int((w*1.05f)/sw)); //max chars on screen + extra marginchars
  GetTextExtent(L"W", &unneeded, &h);
  w = ROUND(w * g_graphicsContext.GetGUIScaleX());
  if (text.IsEmpty() || !g_graphicsContext.SetClipRegion(x, y, w, h))
    return; // nothing to render
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
    float charWidth;
    m_font->GetTextExtentInternal(sz, &charWidth, &unneeded);
    if (scrollInfo.pixelPos < charWidth - scrollInfo.pixelSpeed)
      scrollInfo.pixelPos += scrollInfo.pixelSpeed;
    else
    {
      scrollInfo.pixelPos -= (charWidth - scrollInfo.pixelSpeed);
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
  if (pPalette)
  {
    DWORD *alphaColor = new DWORD[numColors];
    for (int i=0; i < numColors; i++)
    {
      if (!color[i]) color[i] = m_textColor;
      alphaColor[i] = g_graphicsContext.MergeAlpha(color[i]);
    }
    if (dwShadowColor)
      m_font->DrawTextImpl(x - scrollInfo.pixelPos + 1, y + 1, Transform(angle), g_graphicsContext.MergeAlpha(dwShadowColor), pOutput, wcslen(pOutput), 0, w + scrollInfo.pixelPos + h*2);
    m_font->DrawColourTextImpl(x - scrollInfo.pixelPos, y, Transform(angle), alphaColor, pOutput, pOutPalette, wcslen(pOutput), 0, w + scrollInfo.pixelPos + h*2);
    delete[] alphaColor;
  }
  else
  {
    if (!*color) *color = m_textColor;
    if (dwShadowColor)
      m_font->DrawTextImpl(x - scrollInfo.pixelPos + 1, y + 1, Transform(angle), g_graphicsContext.MergeAlpha(dwShadowColor), pOutput, wcslen(pOutput), 0, w + scrollInfo.pixelPos + h*2);
    m_font->DrawTextWidthInternal(x - scrollInfo.pixelPos, y, Transform(angle), g_graphicsContext.MergeAlpha(*color), pOutput, w + scrollInfo.pixelPos + h*2);
  }
  delete[] pOutput;
  if (pPalette)
    delete[] pOutPalette;
  g_graphicsContext.RestoreClipRegion();
}

// transform our "angle" vector by the appropriate amount.
// note that our transform is non-linear, so we must subtract off the transform
// of the origin.  Plus, our transform is scaling, and we don't wish to scale fonts
// as this screws up the aliasing.
CAngle CGUIFont::Transform(const CAngle &angle)
{
  CAngle result;
  result.base_x = g_graphicsContext.ScaleFinalXCoord(angle.base_x, angle.base_y) - g_graphicsContext.ScaleFinalXCoord(0, 0);
  result.base_y = g_graphicsContext.ScaleFinalYCoord(angle.base_x, angle.base_y) - g_graphicsContext.ScaleFinalYCoord(0, 0);
  result.base_z = g_graphicsContext.ScaleFinalZCoord(angle.base_x, angle.base_y) - g_graphicsContext.ScaleFinalZCoord(0, 0);

  result.up_x = g_graphicsContext.ScaleFinalXCoord(angle.up_x, angle.up_y) - g_graphicsContext.ScaleFinalXCoord(0, 0);
  result.up_y = g_graphicsContext.ScaleFinalYCoord(angle.up_x, angle.up_y) - g_graphicsContext.ScaleFinalYCoord(0, 0);
  result.up_z = g_graphicsContext.ScaleFinalZCoord(angle.up_x, angle.up_y) - g_graphicsContext.ScaleFinalZCoord(0, 0);

  // remove the gui scaling from these values
  const float guiScaleX = 1/g_graphicsContext.GetGUIScaleX();
  const float guiScaleY = 1/g_graphicsContext.GetGUIScaleY();
  result.base_x *= guiScaleX; result.up_x *= guiScaleX;
  result.base_y *= guiScaleY; result.up_y *= guiScaleY;
  result.base_z *= guiScaleY; result.up_z *= guiScaleY;
  return result;
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
      DrawText(x - i, y + j, outlineColor, 0, text, flags, maxWidth);
      DrawText(x - i, y - j, outlineColor, 0, text, flags, maxWidth);
      DrawText(x + i, y + j, outlineColor, 0, text, flags, maxWidth);
      DrawText(x + i, y - j, outlineColor, 0, text, flags, maxWidth);
    }
  }
  DrawText(x, y, color, 0, text, flags, maxWidth);
  End();
}
