/*!
\file GUIFont.h
\brief
*/

#ifndef CGUILIB_GUIFONTTTF_H
#define CGUILIB_GUIFONTTTF_H
#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "rendering/SceneGraph.h"
#include <map>

// forward definition
class CBaseTexture;

struct FT_FaceRec_;
struct FT_LibraryRec_;
struct FT_GlyphSlotRec_;
struct FT_BitmapGlyphRec_;
struct FT_StrokerRec_;

typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_GlyphSlotRec_ *FT_GlyphSlot;
typedef struct FT_BitmapGlyphRec_ *FT_BitmapGlyph;
typedef struct FT_StrokerRec_ *FT_Stroker;

typedef uint32_t character_t;
typedef uint32_t color_t;
typedef std::vector<character_t> vecText;
typedef std::vector<color_t> vecColors;
typedef std::map<color_t, CBatchDraw> mapDraws;

/*!
 \ingroup textures
 \brief
 */

class CGUIFontTTF
{
  friend class CGUIFont;

public:

  CGUIFontTTF(const CStdString& strFileName);
  ~CGUIFontTTF(void);

  void Clear();

  bool Load(const CStdString& strFilename, float height = 20.0f, float aspect = 1.0f, float lineSpacing = 1.0f, bool border = false);

  void Begin();
  void End();

  const CStdString& GetFileName() const { return m_strFileName; };

private:
  struct Character
  {
    short offsetX, offsetY;
    float left, top, right, bottom;
    float advance;
    character_t letterAndStyle;
  };
  void AddReference();
  void RemoveReference();

  float GetTextWidthInternal(vecText::const_iterator start, vecText::const_iterator end);
  float GetCharWidthInternal(character_t ch);
  float GetTextHeight(float lineSpacing, int numLines) const;
  float GetTextBaseLine() const { return (float)m_cellBaseLine; }
  float GetLineHeight(float lineSpacing) const;
  float GetFontHeight() const { return m_height; }

  void DrawTextInternal(float x, float y, const vecColors &colors, const vecText &text,
                            uint32_t alignment, float maxPixelWidth, bool scrolling);

  float m_height;
  CStdString m_strFilename;

  // Stuff for pre-rendering for speed
  inline Character *GetCharacter(character_t letter);
  bool CacheCharacter(wchar_t letter, uint32_t style, Character *ch);
  void RenderCharacter(float posX, float posY, const Character *ch, color_t color, bool roundX);
  void ClearCharacterCache();

  CBaseTexture* ReallocTexture(unsigned int& newHeight);
  bool CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
  void DeleteHardwareTexture();

  // modifying glyphs
  void EmboldenGlyph(FT_GlyphSlot slot);
  void ObliqueGlyph(FT_GlyphSlot slot);

  CBaseTexture* m_texture;        // texture that holds our rendered characters (8bit alpha only)

  unsigned int m_textureWidth;       // width of our texture
  unsigned int m_textureHeight;      // heigth of our texture
  unsigned int m_texturePitch;
  unsigned int m_textureBlockSize;
  int m_posX;                        // current position in the texture
  int m_posY;

  /*! \brief the height of each line in the texture.
   Accounts for spacing between lines to avoid characters overlapping.
   */
  unsigned int GetTextureLineHeight() const;
  static unsigned int spacing_between_characters_in_texture;

  color_t m_color;

  Character *m_char;                 // our characters
  Character *m_charquick[256*4];     // ascii chars (4 styles) here
  int m_maxChars;                    // size of character array (can be incremented)
  int m_numChars;                    // the current number of cached characters

  float m_ellipsesWidth;               // this is used every character (width of '.')

  unsigned int m_cellBaseLine;
  unsigned int m_cellHeight;

  unsigned int m_nestedBeginCount;             // speedups

  // freetype stuff
  FT_Face    m_face;
  FT_Stroker m_stroker;

  float m_originX;
  float m_originY;

  bool m_bTextureLoaded;
  unsigned int m_nTexture;

  float    m_textureScaleX;
  float    m_textureScaleY;

  static int justification_word_weight;

  CStdString m_strFileName;
  int m_referenceCount;
  unsigned char *m_cachePixels;
  mapDraws m_batchDraws;
};
#endif
