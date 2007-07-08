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

#define FONT_STYLE_NORMAL       0
#define FONT_STYLE_BOLD         1
#define FONT_STYLE_ITALICS      2
#define FONT_STYLE_BOLD_ITALICS 3

#define DEGREE_TO_RADIAN 0.01745329252f

class CAngle
{
public:
  CAngle()
  {
    base_x = 1; base_y = base_z = 0;
    up_x = up_z = 0; up_y = 1;
  }
  CAngle(int theta)
  {
    base_x = cos(theta * DEGREE_TO_RADIAN);
    base_y = sin(theta * DEGREE_TO_RADIAN);
    base_z = 0;
    up_x = -base_y;
    up_y = base_x;
    up_z = 0;
  }
  float base_x, base_y, base_z;
  float up_x, up_y, up_z;
};

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

  unsigned int m_iMaxCharWidth;
  CStdString m_strFileName;
private:
  int m_referenceCount;
};

#endif
