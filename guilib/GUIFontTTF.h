/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONTTTF_H
#define CGUILIB_GUIFONTTTF_H
#pragma once

#include "GUIFontBase.h"
#include <xfont.h>

// forward definition
struct FT_FaceRec_;
struct FT_LibraryRec_;

typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_LibraryRec_ *FT_Library;


/*!
 \ingroup textures
 \brief 
 */
class CGUIFontTTF: public CGUIFontBase
{
  struct Character
  {
    WCHAR letter;
    unsigned short originX, originY;
    short left, top, right, bottom;
    float advance;
  };
public:

  CGUIFontTTF(const CStdString& strFileName);
  virtual ~CGUIFontTTF(void);

  void Clear();

  // Change font style: XFONT_NORMAL, XFONT_BOLD, XFONT_ITALICS, XFONT_BOLDITALICS
  bool Load(const CStdString& strFilename, int height = 20, int style = XFONT_NORMAL);

  virtual void Begin();
  virtual void End();

protected:
  virtual void GetTextExtentInternal(const WCHAR* strText, FLOAT* pWidth,
                             FLOAT* pHeight, BOOL bFirstLineOnly = FALSE);

  virtual void DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD dwColor,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f);

  virtual void DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD* pdw256ColorPalette,
                                  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                  FLOAT fMaxPixelWidth);

  void DrawTextInternal(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD *pdw256ColorPalette, BYTE *pbColours,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f);

  int m_iHeight;
  int m_iStyle;
  CStdString m_strFilename;

  // Stuff for pre-rendering for speed
  Character *GetCharacter(WCHAR letter);
  bool CacheCharacter(WCHAR letter, Character *ch);
  void RenderCharacter(float posX, float posY, const CAngle &angle, const Character *ch, D3DCOLOR dwColor);
  void CreateShader();
  void ClearCharacterCache();

  LPDIRECT3DTEXTURE8 m_texture;      // texture that holds our rendered characters (8bit alpha only)
  DWORD m_fontShader;                // pixel shader for rendering chars from the 8bit alpha texture
  LPDIRECT3DDEVICE8 m_pD3DDevice;

  Character *m_char;                 // our characters
  int m_maxChars;                    // size of character array (can be incremented)
  int m_numChars;                    // the current number of cached characters
  int m_textureRows;                 // the number of rows in our texture
  int m_posX;                        // current position in the texture
  int m_posY;

  float m_ellipsesWidth;               // this is used every character (width of '.')

  unsigned int m_cellBaseLine;
  unsigned int m_cellHeight;
  unsigned int m_cellAscent;          // typical distance from baseline to top of glyph - based on the 'W'
  unsigned int m_lineHeight;

  DWORD m_dwNestedBeginCount;             // speedups

  // freetype stuff
  FT_Face    m_face;
  FT_Library m_library;
};

#endif
