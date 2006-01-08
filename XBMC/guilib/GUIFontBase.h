/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONTBASE_H
#define CGUILIB_GUIFONTBASE_H
#pragma once

#include "common/xbfont.h"
#include <xfont.h>

/*!
 \ingroup textures
 \brief 
 */
class CGUIFontBase
{
  friend class CGUIFont;
public:
  CGUIFontBase(const CStdString& strFileName);
  virtual ~CGUIFontBase();

  CStdString& GetFileName();

  virtual void Begin() {};
  virtual void End() {};

protected:
  void AddReference();
  void RemoveReference();

  virtual void DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  virtual void GetTextExtentInternal( const WCHAR* strText, FLOAT* pWidth, FLOAT* pHeight, BOOL bFirstLineOnly = FALSE) = 0;

  void DrawTextWidthInternal(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD dwColor,
                             const WCHAR* strText, float fMaxWidth);

  virtual void DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD dwColor,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f) = 0;

  virtual void DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD* pdw256ColorPalette,
                                  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                  FLOAT fMaxPixelWidth) = 0;

  float m_iMaxCharWidth;
  CStdString m_strFileName;
private:
  int m_referenceCount;
};

#endif
