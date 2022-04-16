/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <stdint.h>
#include <vector>

#include "utils/auto_buffer.h"
#include "utils/Color.h"
#include "utils/Geometry.h"

#ifdef HAS_DX
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
#endif

constexpr size_t LOOKUPTABLE_SIZE = 256 * 8;

class CTexture;
class CRenderSystemBase;

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
typedef std::vector<character_t> vecText;

/*!
 \ingroup textures
 \brief
 */

#ifdef HAS_DX
struct SVertex
{
  float x, y, z;
  XMFLOAT4 col;
  float u, v;
  float u2, v2;
};
#else
struct SVertex
{
  float x, y, z;
  unsigned char r, g, b, a;
  float u, v;
};
#endif

#include "GUIFontCache.h"


class CGUIFontTTF
{
  friend class CGUIFont;

public:
  virtual ~CGUIFontTTF();

  static CGUIFontTTF* CreateGUIFontTTF(const std::string& fileName);

  void Clear();

  bool Load(const std::string& strFilename, float height = 20.0f, float aspect = 1.0f, float lineSpacing = 1.0f, bool border = false);

  void Begin();
  void End();
  /* The next two should only be called if we've declared we can do hardware clipping */
  virtual CVertexBuffer CreateVertexBuffer(const std::vector<SVertex> &vertices) const { assert(false); return CVertexBuffer(); }
  virtual void DestroyVertexBuffer(CVertexBuffer &bufferHandle) const {}

  const std::string& GetFileName() const { return m_strFileName; };

protected:
  explicit CGUIFontTTF(const std::string& strFileName);

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

  void DrawTextInternal(float x, float y, const std::vector<UTILS::Color> &colors, const vecText &text,
                            uint32_t alignment, float maxPixelWidth, bool scrolling);

  float m_height;
  std::string m_strFilename;

  // Stuff for pre-rendering for speed
  inline Character *GetCharacter(character_t letter);
  bool CacheCharacter(wchar_t letter, uint32_t style, Character *ch);
  void RenderCharacter(float posX, float posY, const Character *ch, UTILS::Color color, bool roundX, std::vector<SVertex> &vertices);
  void ClearCharacterCache();

  virtual CTexture* ReallocTexture(unsigned int& newHeight) = 0;
  virtual bool CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) = 0;
  virtual void DeleteHardwareTexture() = 0;

  // modifying glyphs
  void SetGlyphStrength(FT_GlyphSlot slot, int glyphStrength);
  static void ObliqueGlyph(FT_GlyphSlot slot);

  CTexture* m_texture; // texture that holds our rendered characters (8bit alpha only)

  unsigned int m_textureWidth;       // width of our texture
  unsigned int m_textureHeight;      // height of our texture
  int m_posX;                        // current position in the texture
  int m_posY;

  /*! \brief the height of each line in the texture.
   Accounts for spacing between lines to avoid characters overlapping.
   */
  unsigned int GetTextureLineHeight() const;
  static const unsigned int spacing_between_characters_in_texture;

  UTILS::Color m_color;

  Character *m_char;                 // our characters
  Character *m_charquick[LOOKUPTABLE_SIZE];     // ascii chars (7 styles) here
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

  unsigned int m_nTexture;

  struct CTranslatedVertices
  {
    float translateX;
    float translateY;
    float translateZ;
    const CVertexBuffer *vertexBuffer;
    CRect clip;
    CTranslatedVertices(float translateX, float translateY, float translateZ, const CVertexBuffer *vertexBuffer, const CRect &clip) : translateX(translateX), translateY(translateY), translateZ(translateZ), vertexBuffer(vertexBuffer), clip(clip) {}
  };
  std::vector<CTranslatedVertices> m_vertexTrans;
  std::vector<SVertex> m_vertex;

  float    m_textureScaleX;
  float    m_textureScaleY;

  std::string m_strFileName;
  XUTILS::auto_buffer m_fontFileInMemory; // used only in some cases, see CFreeTypeLibrary::GetFont()

  CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue> m_staticCache;
  CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue> m_dynamicCache;

  CRenderSystemBase *m_renderSystem = nullptr;

private:
  virtual bool FirstBegin() = 0;
  virtual void LastEnd() = 0;
  CGUIFontTTF(const CGUIFontTTF&) = delete;
  CGUIFontTTF& operator=(const CGUIFontTTF&) = delete;
  int m_referenceCount;
};
