/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFont.h"
#include "guilib/GUIVecText.h"
#include "utils/ColorUtils.h"
#include "utils/Geometry.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>

#ifdef HAS_DX
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
#endif

class CGraphicContext;
class CTexture;
class CRenderSystemBase;

struct FT_FaceRec_;
struct FT_LibraryRec_;
struct FT_GlyphSlotRec_;
struct FT_BitmapGlyphRec_;
struct FT_StrokerRec_;

typedef struct FT_FaceRec_* FT_Face;
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_GlyphSlotRec_* FT_GlyphSlot;
typedef struct FT_BitmapGlyphRec_* FT_BitmapGlyph;
typedef struct FT_StrokerRec_* FT_Stroker;

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

typedef uint64_t glyph_and_style_t;

struct Character
{
  short m_offsetX;
  short m_offsetY;
  float m_left;
  float m_top;
  float m_right;
  float m_bottom;
  float m_advance;
  FT_UInt m_glyphIndex;
  glyph_and_style_t m_glyphAndStyle;
};

struct CFontData
{
  CFontData() = default;
  ~CFontData();
  void Clear();
  void ClearCharacterCache();

  bool m_mainFont{false};

  unsigned int m_cellBaseLine{0};
  unsigned int m_cellHeight{0};

  // freetype stuff
  FT_Face m_face{nullptr};
  FT_Stroker m_stroker{nullptr};

  hb_font_t* m_hbFont{nullptr};

  // used only in some cases, see CFreeTypeLibrary::GetFont()
  std::vector<uint8_t> m_fontFileInMemory;

  std::vector<Character> m_char; // our characters
};

struct CFontTable
{
  enum class ADDON
  {
    IN_KODI = 0,
    RESOURCE_FONT_EXCLUDED,
    RESOURCE_FONT_LIMITED,
    RESOURCE_FONT_ACTIVE,
    RESOURCE_FONT_EMOJI,
    RESOURCE_LANGUAGE_AM_ET,
    RESOURCE_LANGUAGE_AR_SA,
    RESOURCE_LANGUAGE_HE_IL,
    RESOURCE_LANGUAGE_HI_IN,
    RESOURCE_LANGUAGE_HY_AM,
    RESOURCE_LANGUAGE_KN_IN,
    RESOURCE_LANGUAGE_KO_KR,
    RESOURCE_LANGUAGE_JA_JP,
    RESOURCE_LANGUAGE_ML_IN,
    RESOURCE_LANGUAGE_MY_MM,
    RESOURCE_LANGUAGE_SI_LK,
    RESOURCE_LANGUAGE_TA_IN,
    RESOURCE_LANGUAGE_TE_IN,
    RESOURCE_LANGUAGE_TH_TH,
    RESOURCE_LANGUAGE_ZH_CN,
    RESOURCE_LANGUAGE_ZH_TW,
    NOT_USED
  };

  enum class ALIGN
  {
    LTOR,
    RTOL
  };
  char32_t m_unicode_code_begin;
  char32_t m_unicode_code_end;
  const char* m_filename;
  ALIGN alignment;
  const char* m_name;
  ADDON m_addon;
};

class CGUIFontTTF
{
  // use lookup table for the first 4096 glyphs (almost any letter or symbol) to
  // speed up GUI rendering and decrease CPU usage and less memory reallocations
  static constexpr int MAX_GLYPH_IDX = 4096;
  static constexpr size_t LOOKUPTABLE_SIZE = MAX_GLYPH_IDX * FONT_STYLES_COUNT;

  friend class CGUIFont;

public:
  virtual ~CGUIFontTTF();

  static CGUIFontTTF* CreateGUIFontTTF(const std::string& fontIdent);

  void Clear();

  bool Load(const std::string& strFilename,
            float height = 20.0f,
            float aspect = 1.0f,
            float lineSpacing = 1.0f,
            bool border = false);

  void Begin();
  void End();
  /* The next two should only be called if we've declared we can do hardware clipping */
  virtual CVertexBuffer CreateVertexBuffer(const std::vector<SVertex>& vertices) const
  {
    assert(false);
    return CVertexBuffer();
  }
  virtual void DestroyVertexBuffer(CVertexBuffer& bufferHandle) const {}

  const std::string& GetFontIdent() const { return m_fontIdent; }

protected:
  explicit CGUIFontTTF(const std::string& fontIdent);

  struct Glyph
  {
    hb_glyph_info_t m_glyphInfo{};
    hb_glyph_position_t m_glyphPosition{};
    CFontData* m_fontData{nullptr};

    Glyph(const hb_glyph_info_t& glyphInfo,
          const hb_glyph_position_t& glyphPosition,
          CFontData* fData)
      : m_glyphInfo(glyphInfo), m_glyphPosition(glyphPosition), m_fontData(fData)
    {
    }
    Glyph(CFontData* fData) : m_fontData(fData) {}
  };

  struct RunInfo
  {
    unsigned int m_startOffset;
    unsigned int m_endOffset;
    hb_buffer_t* m_buffer;
    hb_script_t m_script;
    hb_glyph_info_t* m_glyphInfos;
    hb_glyph_position_t* m_glyphPositions;
  };

  void AddReference();
  void RemoveReference();

  std::vector<Glyph> GetHarfBuzzShapedGlyphs(const vecText& text);

  float GetTextWidthInternal(const vecText& text);
  float GetTextWidthInternal(const vecText& text, const std::vector<Glyph>& glyph);
  float GetCharWidthInternal(const character_t& ch);
  float GetTextHeight(float lineSpacing, int numLines) const;
  float GetTextBaseLine() const { return static_cast<float>(m_fontMain.m_cellBaseLine); }
  float GetLineHeight(float lineSpacing) const;
  float GetFontHeight() const { return m_height; }

  void DrawTextInternal(CGraphicContext& context,
                        float x,
                        float y,
                        const std::vector<UTILS::COLOR::Color>& colors,
                        const vecText& text,
                        uint32_t alignment,
                        float maxPixelWidth,
                        bool scrolling);

  float m_height{0.0f};
  float m_aspect{1.0f};
  float m_lineSpacing{1.0f};
  bool m_border{false};

  // Stuff for pre-rendering for speed
  Character* GetCharacter(const character_t& letter, FT_UInt glyphIndex, CFontData* fontData);
  bool CacheCharacter(FT_UInt glyphIndex, uint32_t style, Character* ch, CFontData* fontData);
  void RenderCharacter(CGraphicContext& context,
                       float posX,
                       float posY,
                       const Character* ch,
                       UTILS::COLOR::Color color,
                       bool roundX,
                       std::vector<SVertex>& vertices);
  void ClearCharacterCache();

  virtual std::unique_ptr<CTexture> ReallocTexture(unsigned int& newHeight) = 0;
  virtual bool CopyCharToTexture(FT_BitmapGlyph bitGlyph,
                                 unsigned int x1,
                                 unsigned int y1,
                                 unsigned int x2,
                                 unsigned int y2) = 0;
  virtual void DeleteHardwareTexture() = 0;

  bool LoadFont(const std::string& strFileName,
                float height,
                float aspect,
                float lineSpacing,
                bool border,
                bool mainFont,
                CFontData& data);

  // modifying glyphs
  static void SetGlyphStrength(FT_GlyphSlot slot, FT_Face face, int glyphStrength);
  static void ObliqueGlyph(FT_GlyphSlot slot);

  // texture that holds our rendered characters (8bit alpha only)
  std::unique_ptr<CTexture> m_texture;

  unsigned int m_textureWidth{0}; // width of our texture
  unsigned int m_textureHeight{0}; // height of our texture
  int m_posX{0}; // current position in the texture
  int m_posY{0};

  /*! \brief the height of each line in the texture.
   Accounts for spacing between lines to avoid characters overlapping.
   */
  unsigned int GetTextureLineHeight() const;
  unsigned int GetMaxFontHeight() const;

  UTILS::COLOR::Color m_color{UTILS::COLOR::NONE};

  // room for the first MAX_GLYPH_IDX glyphs in 7 styles
  Character* m_charquick[LOOKUPTABLE_SIZE]{nullptr};

  bool m_ellipseCached{false};
  float m_ellipsesWidth{0.0f}; // this is used every character (width of '.')

  unsigned int m_cellBaseLine{0};
  unsigned int m_cellHeight{0};
  unsigned int m_maxFontHeight{0};

  unsigned int m_nestedBeginCount{0}; // speedups

  float m_originX{0.0f};
  float m_originY{0.0f};

  unsigned int m_nTexture{0};

  CFontData m_fontMain;

  struct CTranslatedVertices
  {
    float m_translateX;
    float m_translateY;
    float m_translateZ;
    const CVertexBuffer* m_vertexBuffer;
    CRect m_clip;
    CTranslatedVertices(float translateX,
                        float translateY,
                        float translateZ,
                        const CVertexBuffer* vertexBuffer,
                        const CRect& clip)
      : m_translateX(translateX),
        m_translateY(translateY),
        m_translateZ(translateZ),
        m_vertexBuffer(vertexBuffer),
        m_clip(clip)
    {
    }
  };
  std::vector<CTranslatedVertices> m_vertexTrans;
  std::vector<SVertex> m_vertex;

  float m_textureScaleX{0.0f};
  float m_textureScaleY{0.0f};

  const std::string m_fontIdent;

  CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue> m_staticCache;
  CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue> m_dynamicCache;

  CRenderSystemBase* m_renderSystem;

private:
  float GetTabSpaceLength();
  CFontData* FindFallback(char32_t letter, const CFontTable*& fontTable);
  uint32_t GetNeededAlignment(char32_t firstLetter, uint32_t alignmentDefault, bool& useRTOLChar);

  virtual bool FirstBegin() = 0;
  virtual void LastEnd() = 0;
  CGUIFontTTF(const CGUIFontTTF&) = delete;
  CGUIFontTTF& operator=(const CGUIFontTTF&) = delete;
  int m_referenceCount{0};

  std::vector<std::pair<CFontTable, std::shared_ptr<CFontData>>> m_fallbackUsed;
};
