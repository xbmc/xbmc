#include "include.h"
#include "GUIFontBase.h"
#include "GUIFontManager.h"

CGUIFontBase::CGUIFontBase(const CStdString& strFileName)
{
  m_strFileName = strFileName;
  m_referenceCount = 0;
}

CGUIFontBase::~CGUIFontBase()
{
}

void CGUIFontBase::AddReference()
{
  m_referenceCount++;
}

void CGUIFontBase::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
  {
    g_fontManager.FreeFontFile(this);
  }
}

CStdString& CGUIFontBase::GetFileName()
{
  return m_strFileName;
}

void CGUIFontBase::DrawTextWidthInternal(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
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
    if (iMinCharsLeft > 5 && iStrLength > iMinCharsLeft)
    {
      // at least 5 chars are on the right, strip all these characters
      // instead of doing it one by one
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

void CGUIFontBase::DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth)
{
  int nStringLength = wcslen(strText);
  WCHAR *pszBuffer = new WCHAR[nStringLength + 1];

  wcscpy(pszBuffer, strText);

  float fTextHeight, fTextWidth;
  GetTextExtentInternal( pszBuffer, &fTextWidth, &fTextHeight);
  if (fTextWidth <= fMaxWidth)
  {
    if (dwShadowColor)
      DrawTextImpl(fOriginX + 1, fOriginY + 1, dwShadowColor, pszBuffer, nStringLength, 0, 0.0f);
    DrawColourTextImpl( fOriginX, fOriginY, pdw256ColorPalette, pszBuffer, pbColours, nStringLength, 0, 0.0f);
    delete[] pszBuffer;
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
  if (dwShadowColor)
    DrawTextImpl(fOriginX + 1, fOriginY + 1, dwShadowColor, pszBuffer, wcslen( pszBuffer ), 0, 0.0f);
  DrawColourTextImpl( fOriginX, fOriginY, pdw256ColorPalette, pszBuffer, pbColours, wcslen( pszBuffer ), 0, 0.0f);
  delete[] pszBuffer;
}
