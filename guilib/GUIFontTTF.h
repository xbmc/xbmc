/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONTTTF_H
#define CGUILIB_GUIFONTTTF_H
#pragma once

#include "common/xbfont.h"
#include "GUIFont.h"
#include <xfont.h>
#include <hash_map>

/*!
 \ingroup textures
 \brief 
 */
class CGUIFontTTF: public CGUIFont
{
  struct Character
  {
    int letter;
    unsigned short left, top, right, bottom;
    unsigned short width, height;           // just for packing to 16 bytes.
  };
public:

  CGUIFontTTF(const CStdString& strFontName);
  virtual ~CGUIFontTTF(void);

  void Clear();

  // Change font style: XFONT_NORMAL, XFONT_BOLD, XFONT_ITALICS, XFONT_BOLDITALICS
  bool Load(const CStdString& strFilename, int height = 20, int style = XFONT_NORMAL);

  virtual void Begin();
  virtual void End();
  virtual void GetTextExtent(const WCHAR* strText, FLOAT* pWidth,
                             FLOAT* pHeight, BOOL bFirstLineOnly = FALSE);

protected:
  virtual void DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f);

  virtual void DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                                  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                  FLOAT fMaxPixelWidth);

  void DrawTextInternal(FLOAT fOriginX, FLOAT fOriginY, DWORD *pdw256ColorPalette, BYTE *pbColours,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f);

  XFONT* m_pTrueTypeFont;
  int m_iHeight;
  int m_iStyle;
  CStdString m_strFilename;

  // Stuff for pre-rendering for speed
  Character *GetCharacter(WCHAR letter);
  void CacheCharacter(WCHAR letter, Character *ch);
  void RenderCharacter(int posX, int posY, const Character *ch, D3DCOLOR dwColor);

  LPDIRECT3DTEXTURE8 m_texture;
  LPDIRECT3DDEVICE8 m_pD3DDevice;
  Character *m_char;                 // our characters
  int m_maxChars;                    // size of character array (can be incremented)
  int m_numChars;                    // the current number of cached characters
  int m_textureRows;                 // the number of rows in our texture
  int m_charGap;                     // space between characters in pixels in our texture (for kerning)
  int m_posX;                        // current position in the texture
  int m_posY;
  unsigned int m_descent;
  unsigned int m_ellipsesWidth;               // this is used every character (width of '.')

  DWORD m_dwNestedBeginCount;             // speedups
};

#endif
