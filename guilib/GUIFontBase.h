/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONTBASE_H
#define CGUILIB_GUIFONTBASE_H
#pragma once

// flags for alignment
#define XBFONT_LEFT       0x00000000
#define XBFONT_RIGHT      0x00000001
#define XBFONT_CENTER_X   0x00000002
#define XBFONT_CENTER_Y   0x00000004
#define XBFONT_TRUNCATED  0x00000008
#define XBFONT_JUSTIFIED  0x00000010

#define FONT_STYLE_NORMAL       0
#define FONT_STYLE_BOLD         1
#define FONT_STYLE_ITALICS      2
#define FONT_STYLE_BOLD_ITALICS 3


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

  virtual void DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                                   const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  virtual void GetTextExtentInternal( const WCHAR* strText, FLOAT* pWidth, FLOAT* pHeight = NULL, BOOL bFirstLineOnly = FALSE) = 0;

  void DrawTextWidthInternal(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                             const WCHAR* strText, float fMaxWidth);

  virtual void DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f) = 0;

  virtual void DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                                  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                  FLOAT fMaxPixelWidth) = 0;

  unsigned int m_iMaxCharWidth;
  CStdString m_strFileName;
private:
  int m_referenceCount;
};

#endif
