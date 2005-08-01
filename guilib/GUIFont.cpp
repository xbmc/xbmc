#include "include.h"
#include "guifontxpr.h"
#include "graphiccontext.h"

CGUIFont::CGUIFont(const CStdString& strFontName)
{
  m_strFontName = strFontName;
}

CGUIFont::~CGUIFont()
{}

CStdString& CGUIFont::GetFontName()
{
  return m_strFontName;
}

void CGUIFont::DrawShadowText( FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                               const WCHAR* strText, DWORD dwFlags,
                               FLOAT fMaxPixelWidth,
                               int iShadowWidth,
                               int iShadowHeight,
                               DWORD dwShadowColor)
{
  Begin();
  float nw = 0.0f, nh = 0.0f;
  fOriginX = g_graphicsContext.ScaleFinalXCoord(fOriginX);
  fOriginY = g_graphicsContext.ScaleFinalYCoord(fOriginY);
  fMaxPixelWidth *= g_graphicsContext.ScaleFinalX();

  for (int x = -iShadowWidth; x < iShadowWidth; x++)
  {
    for (int y = -iShadowHeight; y < iShadowHeight; y++)
    {
      DrawTextImpl( (float)x + fOriginX, (float)y + fOriginY, g_graphicsContext.MergeAlpha(dwShadowColor), strText, wcslen( strText ), dwFlags);
    }
  }

  DrawTextImpl( fOriginX, fOriginY, g_graphicsContext.MergeAlpha(dwColor), strText, wcslen( strText ), dwFlags);
  End();
}

void CGUIFont::DrawTextWidthInternal(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                             const WCHAR* strText, float fMaxWidth)
{
  float fTextHeight, fTextWidth;
  GetTextExtentInternal( strText, &fTextWidth, &fTextHeight);
  if (fTextWidth <= fMaxWidth)
  {
    DrawTextImpl( fOriginX, fOriginY, dwColor, strText, wcslen( strText ), 0, 0.0f);
    return ;
  }
  

  int iMinCharsLeft;
  int iStrLength = wcslen( strText );
  WCHAR *wszText = new WCHAR[iStrLength + 1];
  wcscpy(wszText, strText);
  while (fTextWidth >= fMaxWidth && fTextWidth > 0)
  {
    iMinCharsLeft = (int)((fTextWidth - fMaxWidth) / m_iMaxCharWidth);
    if (iMinCharsLeft > 5)
    {
      // at least 5 chars are left, strip al remaining characters instead
      // of doing it one by one.
      iStrLength -= iMinCharsLeft;
      wszText[iStrLength] = 0;
      GetTextExtentInternal(wszText, &fTextWidth, &fTextHeight);
    }
    else
    {
      wszText[--iStrLength] = 0;
      GetTextExtentInternal(wszText, &fTextWidth, &fTextHeight);
    }
  }

  DrawTextImpl( fOriginX, fOriginY, dwColor, wszText, wcslen( wszText ), 0, 0.0f);
  delete[] wszText;
}

void CGUIFont::DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                             const WCHAR* strText, float fMaxWidth)
{
  fOriginX = g_graphicsContext.ScaleFinalXCoord(fOriginX);
  fOriginY = g_graphicsContext.ScaleFinalYCoord(fOriginY);
  fMaxWidth *= g_graphicsContext.ScaleFinalX();

  DrawTextWidthInternal(fOriginX, fOriginY, g_graphicsContext.MergeAlpha(dwColor), strText, fMaxWidth);
}

void CGUIFont::DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette, int numColors,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth)
{

  float nh = 0.0f;

  fOriginX = g_graphicsContext.ScaleFinalXCoord(fOriginX);
  fOriginY = g_graphicsContext.ScaleFinalYCoord(fOriginY);
  DWORD *alphaColor = new DWORD[numColors];
  for (int i = 0; i < numColors; i++)
    alphaColor[i] = g_graphicsContext.MergeAlpha(pdw256ColorPalette[i]);

  fMaxWidth *= g_graphicsContext.ScaleFinalX();

  int nStringLength = wcslen(strText);
  WCHAR *pszBuffer = new WCHAR[nStringLength + 1];

  wcscpy(pszBuffer, strText);

  float fTextHeight, fTextWidth;
  GetTextExtentInternal( pszBuffer, &fTextWidth, &fTextHeight);
  if (fTextWidth <= fMaxWidth)
  {
    DrawColourTextImpl( fOriginX, fOriginY, alphaColor, pszBuffer, pbColours, nStringLength, 0, 0.0f);
    delete[] alphaColor;
    return ;
  }

  if (fMaxWidth)
  {
    int iMinCharsLeft;
    while (fTextWidth >= fMaxWidth && nStringLength)
    {
      iMinCharsLeft = (int)((fTextWidth - fMaxWidth) / m_iMaxCharWidth);
      if (nStringLength > iMinCharsLeft && iMinCharsLeft > 5)
      {
        // at least 5 chars are left, strip al remaining characters instead
        // of doing it one by one.
        nStringLength -= iMinCharsLeft;
        pszBuffer[ nStringLength ] = 0;
        GetTextExtentInternal( pszBuffer, &fTextWidth, &fTextHeight);
      }
      else
      {
        pszBuffer[ --nStringLength ] = 0;
        GetTextExtentInternal( pszBuffer, &fTextWidth, &fTextHeight);
      }
    }
  }
  DrawColourTextImpl( fOriginX, fOriginY, alphaColor, pszBuffer, pbColours, wcslen( pszBuffer ), 0, 0.0f);
  delete[] pszBuffer;
  delete[] alphaColor;
}

void CGUIFont::DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, const WCHAR* strText, DWORD dwFlags, FLOAT fMaxPixelWidth)
{
  float nw = 0.0f, nh = 0.0f;
  sx = g_graphicsContext.ScaleFinalXCoord(sx);
  sy = g_graphicsContext.ScaleFinalYCoord(sy);
  fMaxPixelWidth *= g_graphicsContext.ScaleFinalX();
  DrawTextImpl( sx, sy, g_graphicsContext.MergeAlpha(dwColor), strText, wcslen( strText ), dwFlags, fMaxPixelWidth );
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
  GetTextExtentInternal(strText, pWidth, pHeight, bFirstLineOnly);
  *pWidth /= g_graphicsContext.ScaleFinalX();
  *pHeight /= g_graphicsContext.ScaleFinalY();
}

void CGUIFont::DrawScrollingText(float x, float y, DWORD *color, int numColors, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette /* = NULL */)
{
  float unneeded, h;
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
    GetTextExtentInternal(sz, &charWidth, &unneeded);
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
  // Now rotate our string as needed
  WCHAR *pOutput = new WCHAR[text.size()+2];
  WCHAR *pChar = pOutput;
  BYTE *pOutPalette = NULL;
  if (pPalette)
    pOutPalette = new BYTE[text.size()+2];
  BYTE *pPal = pOutPalette;
  for (unsigned int i = scrollInfo.characterPos; i < text.size() + 1; i++)
  {
    if (i < text.size())
      *pChar++ = text[i];
    else
      *pChar++ = L' ';
  }
  for (unsigned int i = 0; i < scrollInfo.characterPos; i++)
  {
    if (i < text.size())
      *pChar++ = text[i];
    else
      *pChar++ = L' ';
  }
  if (pPalette)
  {
    for (unsigned int i = scrollInfo.characterPos; i < text.size() + 1; i++)
    {
      if (i < text.size())
        *pPal++ = pPalette[i];
      else
        *pPal++ =0;
    }
    for (unsigned int i = 0; i < scrollInfo.characterPos; i++)
    {
      if (i < text.size())
        *pPal++ = pPalette[i];
      else
        *pPal++ = L' ';
    }
  }
  *pChar = L'\0';
  if (pPalette)
  {
    DWORD *alphaColor = new DWORD[numColors];
    for (int i=0; i < numColors; i++)
      alphaColor[i] = g_graphicsContext.MergeAlpha(color[i]);
    DrawColourTextImpl(x - scrollInfo.pixelPos, y, color, pOutput, pOutPalette, wcslen(pOutput), 0, w + scrollInfo.pixelPos + h*2);
    delete[] alphaColor;
  }
  else
    DrawTextWidthInternal(x - scrollInfo.pixelPos, y, g_graphicsContext.MergeAlpha(*color), pOutput, w + scrollInfo.pixelPos + h*2);
  delete[] pOutput;
  if (pPalette)
    delete[] pOutPalette;
  g_graphicsContext.RestoreViewPort();
}
