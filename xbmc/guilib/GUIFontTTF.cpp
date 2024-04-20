/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTF.h"

#include "GUIFontManager.h"
#include "ServiceBroker.h"
#include "Texture.h"
#include "URL.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "rendering/RenderSystem.h"
#include "threads/SystemClock.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <math.h>
#include <memory>
#include <queue>
#include <utility>

// stuff for freetype
#include <ft2build.h>
#include <harfbuzz/hb-ft.h>
#if defined(HAS_GL) || defined(HAS_GLES)
#include "utils/GLUtils.h"

#include "system_gl.h"
#endif

#if defined(HAS_DX)
#include "guilib/D3DResource.h"
#endif

#ifdef TARGET_WINDOWS_STORE
#define generic GenericFromFreeTypeLibrary
#endif

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_STROKER_H

namespace
{
constexpr int VERTEX_PER_GLYPH = 4; // number of vertex for each glyph
constexpr int CHARS_PER_TEXTURE_LINE = 20; // number characters to cache per texture line
constexpr int MAX_TRANSLATED_VERTEX = 32; // max number of structs CTranslatedVertices expect to use
constexpr int MAX_GLYPHS_PER_TEXT_LINE = 1024; // max number of glyphs per text line expect to use
constexpr unsigned int SPACING_BETWEEN_CHARACTERS_IN_TEXTURE = 1;
constexpr int CHAR_CHUNK = 64; // 64 chars allocated at a time (2048 bytes)
constexpr int GLYPH_STRENGTH_BOLD = 24;
constexpr int GLYPH_STRENGTH_LIGHT = -48;
constexpr int TAB_SPACE_LENGTH = 4;

// \brief Check for conflicting alignments
void ValidateAlignments(uint32_t& aligns)
{
  // Validate the horizontal alignment (XBFONT_LEFT is implicit unless otherwise specified)
  {
    const uint32_t hAligns = XBFONT_RIGHT | XBFONT_CENTER_X | XBFONT_JUSTIFIED;
    const uint32_t commonFlags = hAligns & aligns;
    // Check if at least 2 bits are set, it means multiple aligns
    if ((commonFlags & (commonFlags - 1)) != 0)
    {
      CLog::LogF(LOGERROR, "Text with invalid multiple horizontal alignments");
      aligns &= ~commonFlags;
    }
  }

  // Validate truncate alignment
  {
    const uint32_t truncateAligns = XBFONT_TRUNCATED | XBFONT_TRUNCATED_LEFT;
    const uint32_t commonFlags = truncateAligns & aligns;
    // Check if at least 2 bits are set, it means multiple aligns
    if ((commonFlags & (commonFlags - 1)) != 0)
    {
      CLog::LogF(LOGERROR, "Text with invalid multiple truncate alignments");
      aligns &= ~commonFlags;
      aligns |= XBFONT_TRUNCATED;
    }
  }
}

} /* namespace */

class CFreeTypeLibrary
{
public:
  CFreeTypeLibrary() = default;
  virtual ~CFreeTypeLibrary()
  {
    if (m_library)
      FT_Done_FreeType(m_library);
  }

  FT_Face GetFont(const std::string& filename,
                  float size,
                  float aspect,
                  std::vector<uint8_t>& memoryBuf)
  {
    // don't have it yet - create it
    if (!m_library)
      FT_Init_FreeType(&m_library);
    if (!m_library)
    {
      CLog::LogF(LOGERROR, "Unable to initialize freetype library");
      return nullptr;
    }

    FT_Face face;

    // ok, now load the font face
    CURL realFile(CSpecialProtocol::TranslatePath(filename));
    if (realFile.GetFileName().empty())
      return nullptr;

    memoryBuf.clear();
#ifndef TARGET_WINDOWS
    if (!realFile.GetProtocol().empty())
#endif // ! TARGET_WINDOWS
    {
      // load file into memory if it is not on local drive
      // in case of win32: always load file into memory as filename is in UTF-8,
      //                   but freetype expect filename in ANSI encoding
      XFILE::CFile f;
      if (f.LoadFile(realFile, memoryBuf) <= 0)
        return nullptr;

      if (FT_New_Memory_Face(m_library, reinterpret_cast<const FT_Byte*>(memoryBuf.data()),
                             memoryBuf.size(), 0, &face) != 0)
        return nullptr;
    }
#ifndef TARGET_WINDOWS
    else if (FT_New_Face(m_library, realFile.GetFileName().c_str(), 0, &face))
      return nullptr;
#endif // ! TARGET_WINDOWS

    unsigned int ydpi = 72; // 72 points to the inch is the freetype default
    unsigned int xdpi =
        static_cast<unsigned int>(MathUtils::round_int(static_cast<double>(ydpi * aspect)));

    // we set our screen res currently to 96dpi in both directions (windows default)
    // we cache our characters (for rendering speed) so it's probably
    // not a good idea to allow free scaling of fonts - rather, just
    // scaling to pixel ratio on screen perhaps?
    if (FT_Set_Char_Size(face, 0, static_cast<int>(size * 64 + 0.5f), xdpi, ydpi))
    {
      FT_Done_Face(face);
      return nullptr;
    }

    return face;
  };

  FT_Stroker GetStroker()
  {
    if (!m_library)
      return nullptr;

    FT_Stroker stroker;
    if (FT_Stroker_New(m_library, &stroker))
      return nullptr;

    return stroker;
  };

  static void ReleaseFont(FT_Face face)
  {
    assert(face);
    FT_Done_Face(face);
  };

  static void ReleaseStroker(FT_Stroker stroker)
  {
    assert(stroker);
    FT_Stroker_Done(stroker);
  }

private:
  FT_Library m_library{nullptr};
};

XBMC_GLOBAL_REF(CFreeTypeLibrary, g_freeTypeLibrary); // our freetype library
#define g_freeTypeLibrary XBMC_GLOBAL_USE(CFreeTypeLibrary)

CGUIFontTTF::CGUIFontTTF(const std::string& fontIdent)
  : m_fontIdent(fontIdent),
    m_staticCache(*this),
    m_dynamicCache(*this),
    m_renderSystem(CServiceBroker::GetRenderSystem())
{
}

CGUIFontTTF::~CGUIFontTTF(void)
{
  Clear();
}

void CGUIFontTTF::AddReference()
{
  m_referenceCount++;
}

void CGUIFontTTF::RemoveReference()
{
  // delete this object when it's reference count hits zero
  m_referenceCount--;
  if (!m_referenceCount)
    g_fontManager.FreeFontFile(this);
}


void CGUIFontTTF::ClearCharacterCache()
{
  m_texture.reset();

  DeleteHardwareTexture();

  m_texture = nullptr;
  m_char.clear();
  m_char.reserve(CHAR_CHUNK);
  memset(m_charquick, 0, sizeof(m_charquick));
  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -static_cast<int>(GetTextureLineHeight());
  m_textureHeight = 0;
}

void CGUIFontTTF::Clear()
{
  m_texture.reset();
  m_texture = nullptr;
  memset(m_charquick, 0, sizeof(m_charquick));
  m_posX = 0;
  m_posY = 0;
  m_nestedBeginCount = 0;

  if (m_hbFont)
    hb_font_destroy(m_hbFont);
  m_hbFont = nullptr;
  if (m_face)
    g_freeTypeLibrary.ReleaseFont(m_face);
  m_face = nullptr;
  if (m_stroker)
    g_freeTypeLibrary.ReleaseStroker(m_stroker);
  m_stroker = nullptr;

  m_vertexTrans.clear();
  m_vertex.clear();

  m_fontFileInMemory.clear();
}

bool CGUIFontTTF::Load(
    const std::string& strFilename, float height, float aspect, float lineSpacing, bool border)
{
  // we now know that this object is unique - only the GUIFont objects are non-unique, so no need
  // for reference tracking these fonts
  m_face = g_freeTypeLibrary.GetFont(strFilename, height, aspect, m_fontFileInMemory);
  if (!m_face)
    return false;

  m_hbFont = hb_ft_font_create(m_face, 0);
  if (!m_hbFont)
    return false;
  /*
   the values used are described below

      XBMC coords                                     Freetype coords

                0  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  bbox.yMax, ascender
                        A                 \
                       A A                |
                      A   A               |
                      AAAAA  pppp   cellAscender
                      A   A  p   p        |
                      A   A  p   p        |
   m_cellBaseLine  _ _A_ _A_ pppp_ _ _ _ _/_ _ _ _ _  0, base line.
                             p            \
                             p      cellDescender
     m_cellHeight  _ _ _ _ _ p _ _ _ _ _ _/_ _ _ _ _  bbox.yMin, descender

   */
  int cellDescender = std::min<int>(m_face->bbox.yMin, m_face->descender);
  int cellAscender = std::max<int>(m_face->bbox.yMax, m_face->ascender);

  if (border)
  {
    /*
     add on the strength of any border - the non-bordered font needs
     aligning with the bordered font by utilising GetTextBaseLine()
     */
    FT_Pos strength = FT_MulFix(m_face->units_per_EM, m_face->size->metrics.y_scale) / 12;
    if (strength < 128)
      strength = 128;

    cellDescender -= strength;
    cellAscender += strength;

    m_stroker = g_freeTypeLibrary.GetStroker();
    if (m_stroker)
      FT_Stroker_Set(m_stroker, strength, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
  }

  // scale to pixel sizing, rounding so that maximal extent is obtained
  float scaler = height / m_face->units_per_EM;
  cellDescender =
      MathUtils::round_int(cellDescender * static_cast<double>(scaler) - 0.5); // round down
  cellAscender = MathUtils::round_int(cellAscender * static_cast<double>(scaler) + 0.5); // round up

  m_cellBaseLine = cellAscender;
  m_cellHeight = cellAscender - cellDescender;

  m_height = height;

  m_texture.reset();
  m_texture = nullptr;

  m_textureHeight = 0;
  m_textureWidth = ((m_cellHeight * CHARS_PER_TEXTURE_LINE) & ~63) + 64;

  m_textureWidth = CTexture::PadPow2(m_textureWidth);

  if (m_textureWidth > m_renderSystem->GetMaxTextureSize())
    m_textureWidth = m_renderSystem->GetMaxTextureSize();
  m_textureScaleX = 1.0f / m_textureWidth;

  // set the posX and posY so that our texture will be created on first character write.
  m_posX = m_textureWidth;
  m_posY = -static_cast<int>(GetTextureLineHeight());

  return true;
}

void CGUIFontTTF::Begin()
{
  if (m_nestedBeginCount == 0 && m_texture && FirstBegin())
  {
    m_vertexTrans.clear();
    m_vertex.clear();
  }
  // Keep track of the nested begin/end calls.
  m_nestedBeginCount++;
}

void CGUIFontTTF::End()
{
  if (m_nestedBeginCount == 0)
    return;

  if (--m_nestedBeginCount > 0)
    return;

  LastEnd();
}

void CGUIFontTTF::DrawTextInternal(CGraphicContext& context,
                                   float x,
                                   float y,
                                   const std::vector<UTILS::COLOR::Color>& colors,
                                   const vecText& text,
                                   uint32_t alignment,
                                   float maxPixelWidth,
                                   bool scrolling,
                                   float dx,
                                   float dy)
{
  if (text.empty())
  {
    return;
  }

  Begin();
  uint32_t rawAlignment = alignment;
  bool dirtyCache(false);

#if not defined(HAS_DX)
  // round coordinates to the pixel grid. otherwise, we might sample at the wrong positions.
  if (!scrolling)
    x = std::round(x);
  y = std::round(y);
#else
  x += dx;
  y += dy;
#endif

#if not defined(HAS_DX)
  // GL can scissor and shader clip
  const bool hardwareClipping = true;
#else
  // FIXME: remove static (CPU based) clipping for GLES/DX
  const bool hardwareClipping = m_renderSystem->ScissorsCanEffectClipping();
#endif

  // FIXME: remove positional stuff once GLES/DX are brought up to date
  CGUIFontCacheStaticPosition staticPos(x, y);
  CGUIFontCacheDynamicPosition dynamicPos;

#if not defined(HAS_DX)
  // dummy positions for the time being
  dynamicPos = CGUIFontCacheDynamicPosition(0.0f, 0.0f, 0.0f);
#else
  if (hardwareClipping)
  {
    dynamicPos =
        CGUIFontCacheDynamicPosition(context.ScaleFinalXCoord(x, y), context.ScaleFinalYCoord(x, y),
                                     context.ScaleFinalZCoord(x, y));
  }
#endif

  CVertexBuffer unusedVertexBuffer;
  CVertexBuffer& vertexBuffer =
      hardwareClipping
          ? m_dynamicCache.Lookup(context, dynamicPos, colors, text, alignment, maxPixelWidth,
                                  scrolling, std::chrono::steady_clock::now(), dirtyCache)
          : unusedVertexBuffer;
  std::shared_ptr<std::vector<SVertex>> tempVertices = std::make_shared<std::vector<SVertex>>();
  std::shared_ptr<std::vector<SVertex>>& vertices =
      hardwareClipping ? tempVertices
                       : static_cast<std::shared_ptr<std::vector<SVertex>>&>(m_staticCache.Lookup(
                             context, staticPos, colors, text, alignment, maxPixelWidth, scrolling,
                             std::chrono::steady_clock::now(), dirtyCache));

  // reserves vertex vector capacity, only the ones that are going to be used
  if (hardwareClipping)
  {
    if (m_vertexTrans.capacity() == 0)
      m_vertexTrans.reserve(MAX_TRANSLATED_VERTEX);
  }
  else
  {
    if (m_vertex.capacity() == 0)
      m_vertex.reserve(VERTEX_PER_GLYPH * MAX_GLYPHS_PER_TEXT_LINE);
  }

  if (dirtyCache)
  {
    // Try to validate any conflicting alignments
    //! @todo: This validate is the last resort and can result in a bad rendered text
    //! because the alignment it is used also by caller components for other operations
    //! this inform the problem on the log, potentially can be improved
    //! by add validating alignments from each parent caller component
    ValidateAlignments(alignment);

    const std::vector<Glyph> glyphs = GetHarfBuzzShapedGlyphs(text);
    // save the origin, which is scaled separately
#if not defined(HAS_DX)
    // the origin is now at [0,0], and not at "random" locations anymore. positioning is done in the vertex shader.
    m_originX = 0;
    m_originY = 0;
#else
    m_originX = x;
    m_originY = y;
#endif

    // cache the ellipses width
    if (!m_ellipseCached)
    {
      m_ellipseCached = true;
      Character* ellipse = GetCharacter(L'.', 0);
      if (ellipse)
        m_ellipsesWidth = ellipse->m_advance;
    }

    // Define the width of ellipses of three chars "..."
    const float ellipsesWidth = 3 * m_ellipsesWidth;

    // Check if we will really need to truncate or justify the text
    if (alignment & XBFONT_TRUNCATED)
    {
      if (maxPixelWidth <= 0.0f || GetTextWidthInternal(text, glyphs) <= maxPixelWidth)
        alignment &= ~XBFONT_TRUNCATED;
    }
    else if (alignment & XBFONT_TRUNCATED_LEFT)
    {
      if (maxPixelWidth <= 0.0f || GetTextWidthInternal(text, glyphs) <= maxPixelWidth)
        alignment &= ~XBFONT_TRUNCATED_LEFT;
    }
    else if (alignment & XBFONT_JUSTIFIED)
    {
      if (maxPixelWidth <= 0.0f)
        alignment &= ~XBFONT_JUSTIFIED;
    }

    // calculate sizing information
    float startX = 0;
    float startY = (alignment & XBFONT_CENTER_Y) ? -0.5f * m_cellHeight : 0; // vertical centering

    size_t startPosGlyph{0}; // Defines the index position where start rendering glyphs
    float textWidth{0}; // The text width, by taking in account truncate (and ellipses)

    if (alignment & XBFONT_TRUNCATED_LEFT)
    {
      // To truncate to the left, we skip all characters that exceed the maximum width,
      // so the rendering starts from the first character that falls within the maximum width,
      // taking into account also the ellipses
      textWidth = ellipsesWidth;

      // We need to iterate from the end to the beginning
      for (auto itRGlyph = glyphs.crbegin(); itRGlyph != glyphs.crend(); ++itRGlyph)
      {
        const character_t ch = text[itRGlyph->m_glyphInfo.cluster];
        Character* c = GetCharacter(ch, itRGlyph->m_glyphInfo.codepoint);
        if (!c)
          continue;

        float nextWidth;
        if ((ch & 0xffff) == static_cast<character_t>('\t'))
          nextWidth = GetTabSpaceLength();
        else
          nextWidth = textWidth + c->m_advance;

        if (nextWidth > maxPixelWidth)
        {
          // Start rendering from the glyph that does not exceed the maximum width
          startPosGlyph = std::distance(itRGlyph, glyphs.crend());
          break;
        }
        textWidth = nextWidth;
      }
    }
    else
    {
      // Calculates the text width based on the characters that can be contained within the maximum width
      if (alignment & XBFONT_TRUNCATED)
        textWidth = ellipsesWidth;

      for (const Glyph& glyph : glyphs)
      {
        const character_t ch = text[glyph.m_glyphInfo.cluster];
        Character* c = GetCharacter(ch, glyph.m_glyphInfo.codepoint);
        if (!c)
          continue;

        float nextWidth;
        if ((ch & 0xffff) == static_cast<character_t>('\t'))
          nextWidth = GetTabSpaceLength();
        else
          nextWidth = textWidth + c->m_advance;

        if (nextWidth > maxPixelWidth)
          break;

        textWidth = nextWidth;
      }
    }

    if (alignment & XBFONT_RIGHT)
    {
      // Moves the x pos with the purpose of having the text effect aligned to the right
      startX += maxPixelWidth - textWidth;
    }
    else if (alignment & XBFONT_CENTER_X)
    {
      textWidth *= 0.5f;
      startX -= textWidth;
    }

    float spacePerSpaceCharacter = 0; // for justification effects
    if (alignment & XBFONT_JUSTIFIED)
    {
      // first compute the size of the text to render in both characters and pixels
      unsigned int numSpaces = 0;
      float linePixels = 0;
      for (const auto& glyph : glyphs)
      {
        Character* ch = GetCharacter(text[glyph.m_glyphInfo.cluster], glyph.m_glyphInfo.codepoint);
        if (ch)
        {
          if ((text[glyph.m_glyphInfo.cluster] & 0xffff) == L' ')
            numSpaces += 1;
          linePixels += ch->m_advance;
        }
      }
      if (numSpaces > 0)
        spacePerSpaceCharacter = (maxPixelWidth - linePixels) / numSpaces;
    }

    float cursorX = 0; // current position along the line
    float offsetX = 0;
    float offsetY = 0;

    // Collect all the Character info in a first pass, in case any of them
    // are not currently cached and cause the texture to be enlarged, which
    // would invalidate the texture coordinates.
    std::queue<Character> characters;

    if (alignment & XBFONT_TRUNCATED_LEFT)
      cursorX += ellipsesWidth;

    auto glyphBegin = glyphs.cbegin() + startPosGlyph;

    for (auto itGlyph = glyphBegin; itGlyph != glyphs.cend(); ++itGlyph)
    {
      Character* ch =
          GetCharacter(text[itGlyph->m_glyphInfo.cluster], itGlyph->m_glyphInfo.codepoint);
      if (!ch)
      {
        Character null = {};
        characters.push(null);
        continue;
      }
      characters.push(*ch);

      if (maxPixelWidth > 0)
      {
        float nextCursorX = cursorX;

        if (alignment & XBFONT_TRUNCATED)
          nextCursorX += ch->m_advance + ellipsesWidth;

        if (nextCursorX > maxPixelWidth)
          break;
      }

      cursorX += ch->m_advance;
    }

    // Reserve vector space: 4 vertex for each glyph
    tempVertices->reserve(VERTEX_PER_GLYPH * glyphs.size());
    cursorX = 0;

    for (auto itGlyph = glyphBegin; itGlyph != glyphs.cend(); ++itGlyph)
    {
      // If starting text on a new line, determine justification effects
      // Get the current letter in the CStdString
      UTILS::COLOR::Color color = (text[itGlyph->m_glyphInfo.cluster] & 0xff0000) >> 16;
      if (color >= colors.size())
        color = 0;
      color = colors[color];

      // grab the next character
      Character* ch = &characters.front();

      if ((text[itGlyph->m_glyphInfo.cluster] & 0xffff) == static_cast<character_t>('\t'))
      {
        const float tabwidth = GetTabSpaceLength();
        const float a = cursorX / tabwidth;
        cursorX += tabwidth - ((a - floorf(a)) * tabwidth);
        characters.pop();
        continue;
      }

      if (alignment & XBFONT_TRUNCATED)
      {
        // Check if we will be exceeded the max allowed width
        if (cursorX + ch->m_advance + ellipsesWidth > maxPixelWidth)
        {
          // Yup. Let's draw the ellipses, then bail
          // Perhaps we should really bail to the next line in this case??
          Character* period = GetCharacter(L'.', 0);
          if (!period)
            break;

          for (int i = 0; i < 3; i++)
          {
            RenderCharacter(context, startX + cursorX, startY, period, color, !scrolling,
                            *tempVertices);
            cursorX += period->m_advance;
          }
          break;
        }
      }
      else if (alignment & XBFONT_TRUNCATED_LEFT && itGlyph == glyphBegin)
      {
        // Add ellipsis only at the beginning of the text
        Character* period = GetCharacter(L'.', 0);
        if (!period)
          break;

        for (int i = 0; i < 3; i++)
        {
          RenderCharacter(context, startX + cursorX, startY, period, color, !scrolling,
                          *tempVertices);
          cursorX += period->m_advance;
        }
      }
      else if (maxPixelWidth > 0 && cursorX > maxPixelWidth)
        break; // exceeded max allowed width - stop rendering

      offsetX = static_cast<float>(
          MathUtils::round_int(static_cast<double>(itGlyph->m_glyphPosition.x_offset) / 64));
      offsetY = static_cast<float>(
          MathUtils::round_int(static_cast<double>(itGlyph->m_glyphPosition.y_offset) / 64));
      RenderCharacter(context, startX + cursorX + offsetX, startY - offsetY, ch, color, !scrolling,
                      *tempVertices);
      if (alignment & XBFONT_JUSTIFIED)
      {
        if ((text[itGlyph->m_glyphInfo.cluster] & 0xffff) == L' ')
          cursorX += ch->m_advance + spacePerSpaceCharacter;
        else
          cursorX += ch->m_advance;
      }
      else
        cursorX += ch->m_advance;
      characters.pop();
    }
    if (hardwareClipping)
    {
      CVertexBuffer& vertexBuffer =
          m_dynamicCache.Lookup(context, dynamicPos, colors, text, rawAlignment, maxPixelWidth,
                                scrolling, std::chrono::steady_clock::now(), dirtyCache);
      CVertexBuffer newVertexBuffer = CreateVertexBuffer(*tempVertices);
      vertexBuffer = newVertexBuffer;
#if not defined(HAS_DX)
      m_vertexTrans.emplace_back(x, y, 0.0f, &vertexBuffer, context.GetClipRegion(), dx, dy);
#else
      m_vertexTrans.emplace_back(.0f, .0f, .0f, &vertexBuffer, context.GetClipRegion());
#endif
    }
    else
    {
      m_staticCache.Lookup(context, staticPos, colors, text, rawAlignment, maxPixelWidth, scrolling,
                           std::chrono::steady_clock::now(), dirtyCache) =
          *static_cast<CGUIFontCacheStaticValue*>(&tempVertices);
      /* Append the new vertices to the set collected since the first Begin() call */
      m_vertex.insert(m_vertex.end(), tempVertices->begin(), tempVertices->end());
    }
  }
  else
  {
    if (hardwareClipping)
#if not defined(HAS_DX)
      m_vertexTrans.emplace_back(x, y, 0.0f, &vertexBuffer, context.GetClipRegion(), dx, dy);
#else
      m_vertexTrans.emplace_back(dynamicPos.m_x, dynamicPos.m_y, dynamicPos.m_z, &vertexBuffer,
                                 context.GetClipRegion());
#endif
    else
      /* Append the vertices from the cache to the set collected since the first Begin() call */
      m_vertex.insert(m_vertex.end(), vertices->begin(), vertices->end());
  }

  End();
}


float CGUIFontTTF::GetTextWidthInternal(const vecText& text)
{
  const std::vector<Glyph> glyphs = GetHarfBuzzShapedGlyphs(text);
  return GetTextWidthInternal(text, glyphs);
}

// this routine assumes a single line (i.e. it was called from GUITextLayout)
float CGUIFontTTF::GetTextWidthInternal(const vecText& text, const std::vector<Glyph>& glyphs)
{
  float width = 0;
  for (auto it = glyphs.begin(); it != glyphs.end(); it++)
  {
    const character_t ch = text[(*it).m_glyphInfo.cluster];
    Character* c = GetCharacter(ch, (*it).m_glyphInfo.codepoint);
    if (c)
    {
      // If last character in line, we want to add render width
      // and not advance distance - this makes sure that italic text isn't
      // choped on the end (as render width is larger than advance then).
      if (std::next(it) == glyphs.end())
        width += std::max(c->m_right - c->m_left + c->m_offsetX, c->m_advance);
      else if ((ch & 0xffff) == static_cast<character_t>('\t'))
        width += GetTabSpaceLength();
      else
        width += c->m_advance;
    }
  }

  return width;
}

float CGUIFontTTF::GetCharWidthInternal(character_t ch)
{
  Character* c = GetCharacter(ch, 0);
  if (c)
  {
    if ((ch & 0xffff) == static_cast<character_t>('\t'))
      return GetTabSpaceLength();
    else
      return c->m_advance;
  }

  return 0;
}

float CGUIFontTTF::GetTextHeight(float lineSpacing, int numLines) const
{
  return static_cast<float>(numLines - 1) * GetLineHeight(lineSpacing) + m_cellHeight;
}

float CGUIFontTTF::GetLineHeight(float lineSpacing) const
{
  if (!m_face)
    return 0.0f;

  return lineSpacing * m_face->size->metrics.height / 64.0f;
}

unsigned int CGUIFontTTF::GetTextureLineHeight() const
{
  return m_cellHeight + SPACING_BETWEEN_CHARACTERS_IN_TEXTURE;
}

unsigned int CGUIFontTTF::GetMaxFontHeight() const
{
  return m_maxFontHeight + SPACING_BETWEEN_CHARACTERS_IN_TEXTURE;
}

std::vector<CGUIFontTTF::Glyph> CGUIFontTTF::GetHarfBuzzShapedGlyphs(const vecText& text)
{
  std::vector<Glyph> glyphs;
  if (text.empty())
  {
    return glyphs;
  }

  std::vector<hb_script_t> scripts;
  std::vector<RunInfo> runs;
  hb_unicode_funcs_t* ufuncs = hb_unicode_funcs_get_default();
  hb_script_t lastScript{HB_SCRIPT_INVALID};
  int lastScriptIndex = -1;
  int lastSetIndex = -1;

  scripts.reserve(text.size());
  for (const auto& character : text)
  {
    scripts.emplace_back(hb_unicode_script(ufuncs, static_cast<wchar_t>(0xffff & character)));
  }

  // HB_SCRIPT_COMMON or HB_SCRIPT_INHERITED should be replaced with previous script
  for (size_t i = 0; i < scripts.size(); ++i)
  {
    if (scripts[i] == HB_SCRIPT_COMMON || scripts[i] == HB_SCRIPT_INHERITED)
    {
      if (lastScriptIndex != -1)
      {
        scripts[i] = lastScript;
        lastSetIndex = i;
      }
    }
    else
    {
      for (size_t j = lastSetIndex + 1; j < i; ++j)
        scripts[j] = scripts[i];
      lastScript = scripts[i];
      lastScriptIndex = i;
      lastSetIndex = i;
    }
  }

  lastScript = scripts[0];
  int lastRunStart = 0;

  for (unsigned int i = 0; i <= static_cast<unsigned int>(scripts.size()); ++i)
  {
    if (i == scripts.size() || scripts[i] != lastScript)
    {
      RunInfo run{};
      run.m_startOffset = lastRunStart;
      run.m_endOffset = i;
      run.m_script = lastScript;
      runs.emplace_back(run);

      if (i < scripts.size())
      {
        lastScript = scripts[i];
        lastRunStart = i;
      }
      else
      {
        break;
      }
    }
  }

  for (auto& run : runs)
  {
    run.m_buffer = hb_buffer_create();
    hb_buffer_set_direction(run.m_buffer, static_cast<hb_direction_t>(HB_DIRECTION_LTR));
    hb_buffer_set_script(run.m_buffer, run.m_script);

    for (unsigned int j = run.m_startOffset; j < run.m_endOffset; j++)
    {
      hb_buffer_add(run.m_buffer, static_cast<wchar_t>(0xffff & text[j]), j);
    }

    hb_buffer_set_content_type(run.m_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
    hb_shape(m_hbFont, run.m_buffer, nullptr, 0);
    unsigned int glyphCount;
    run.m_glyphInfos = hb_buffer_get_glyph_infos(run.m_buffer, &glyphCount);
    run.m_glyphPositions = hb_buffer_get_glyph_positions(run.m_buffer, &glyphCount);

    for (size_t k = 0; k < glyphCount; k++)
    {
      glyphs.emplace_back(run.m_glyphInfos[k], run.m_glyphPositions[k]);
    }

    hb_buffer_destroy(run.m_buffer);
  }

  return glyphs;
}

CGUIFontTTF::Character* CGUIFontTTF::GetCharacter(character_t chr, FT_UInt glyphIndex)
{
  const wchar_t letter = static_cast<wchar_t>(chr & 0xffff);

  // ignore linebreaks
  if (letter == L'\r')
    return nullptr;

  const character_t style = (chr & 0x7000000) >> 24; // style = 0 - 6

  if (!glyphIndex)
    glyphIndex = FT_Get_Char_Index(m_face, letter);

  // quick access to the most frequently used glyphs
  if (glyphIndex < MAX_GLYPH_IDX)
  {
    character_t ch = (style << 12) | glyphIndex; // 2^12 = 4096

    if (ch < LOOKUPTABLE_SIZE && m_charquick[ch])
      return m_charquick[ch];
  }

  // letters are stored based on style and glyph
  character_t ch = (style << 16) | glyphIndex;

  // perform binary search on sorted array by m_glyphAndStyle and
  // if not found obtains position to insert the new m_char to keep sorted
  int low = 0;
  int high = m_char.size() - 1;
  while (low <= high)
  {
    int mid = (low + high) >> 1;
    if (ch > m_char[mid].m_glyphAndStyle)
      low = mid + 1;
    else if (ch < m_char[mid].m_glyphAndStyle)
      high = mid - 1;
    else
      return &m_char[mid];
  }
  // if we get to here, then low is where we should insert the new character

  int startIndex = low;

  // increase the size of the buffer if we need it
  if (m_char.size() == m_char.capacity())
  {
    m_char.reserve(m_char.capacity() + CHAR_CHUNK);
    startIndex = 0;
  }

  // render the character to our texture
  // must End() as we can't render text to our texture during a Begin(), End() block
  unsigned int nestedBeginCount = m_nestedBeginCount;
  m_nestedBeginCount = 1;
  if (nestedBeginCount)
    End();

  m_char.emplace(m_char.begin() + low);
  if (!CacheCharacter(glyphIndex, style, m_char.data() + low))
  { // unable to cache character - try clearing them all out and starting over
    CLog::LogF(LOGDEBUG, "Unable to cache character. Clearing character cache of {} characters",
               m_char.size());
    ClearCharacterCache();
    low = 0;
    startIndex = 0;
    m_char.emplace(m_char.begin());
    if (!CacheCharacter(glyphIndex, style, m_char.data()))
    {
      CLog::LogF(LOGERROR, "Unable to cache character (out of memory?)");
      if (nestedBeginCount)
        Begin();
      m_nestedBeginCount = nestedBeginCount;
      return nullptr;
    }
  }

  if (nestedBeginCount)
    Begin();
  m_nestedBeginCount = nestedBeginCount;

  // update the lookup table with only the m_char addresses that have changed
  for (size_t i = startIndex; i < m_char.size(); ++i)
  {
    if (m_char[i].m_glyphIndex < MAX_GLYPH_IDX)
    {
      // >> 16 is style (0-6), then 16 - 12 (>> 4) is equivalent to style * 4096
      character_t ch = ((m_char[i].m_glyphAndStyle & 0xffff0000) >> 4) | m_char[i].m_glyphIndex;

      if (ch < LOOKUPTABLE_SIZE)
        m_charquick[ch] = m_char.data() + i;
    }
  }

  return m_char.data() + low;
}

bool CGUIFontTTF::CacheCharacter(FT_UInt glyphIndex, uint32_t style, Character* ch)
{
  FT_Glyph glyph = nullptr;
  if (FT_Load_Glyph(m_face, glyphIndex, FT_LOAD_TARGET_LIGHT))
  {
    CLog::LogF(LOGDEBUG, "Failed to load glyph {:x}", glyphIndex);
    return false;
  }

  // make bold if applicable
  if (style & FONT_STYLE_BOLD)
    SetGlyphStrength(m_face->glyph, GLYPH_STRENGTH_BOLD);
  // and italics if applicable
  if (style & FONT_STYLE_ITALICS)
    ObliqueGlyph(m_face->glyph);
  // and light if applicable
  if (style & FONT_STYLE_LIGHT)
    SetGlyphStrength(m_face->glyph, GLYPH_STRENGTH_LIGHT);
  // grab the glyph
  if (FT_Get_Glyph(m_face->glyph, &glyph))
  {
    CLog::LogF(LOGDEBUG, "Failed to get glyph {:x}", glyphIndex);
    return false;
  }
  if (m_stroker)
    FT_Glyph_StrokeBorder(&glyph, m_stroker, 0, 1);
  // render the glyph
  if (FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, nullptr, 1))
  {
    CLog::LogF(LOGDEBUG, "Failed to render glyph {:x} to a bitmap", glyphIndex);
    return false;
  }

  FT_BitmapGlyph bitGlyph = (FT_BitmapGlyph)glyph;
  FT_Bitmap bitmap = bitGlyph->bitmap;
  bool isEmptyGlyph = (bitmap.width == 0 || bitmap.rows == 0);

  if (!isEmptyGlyph)
  {
    if (bitGlyph->left < 0)
      m_posX += -bitGlyph->left;

    // check we have enough room for the character.
    // cast-fest is here to avoid warnings due to freeetype version differences (signedness of width).
    if (static_cast<int>(m_posX + bitGlyph->left + bitmap.width +
                         SPACING_BETWEEN_CHARACTERS_IN_TEXTURE) > static_cast<int>(m_textureWidth))
    { // no space - gotta drop to the next line (which means creating a new texture and copying it across)
      m_posX = 1;
      m_posY += GetTextureLineHeight();
      if (bitGlyph->left < 0)
        m_posX += -bitGlyph->left;

      if (m_posY + GetTextureLineHeight() >= m_textureHeight)
      {
        // create the new larger texture
        unsigned int newHeight = m_posY + GetTextureLineHeight();
        // check for max height
        if (newHeight > m_renderSystem->GetMaxTextureSize())
        {
          CLog::LogF(LOGDEBUG, "New cache texture is too large ({} > {} pixels long)", newHeight,
                     m_renderSystem->GetMaxTextureSize());
          FT_Done_Glyph(glyph);
          return false;
        }

        std::unique_ptr<CTexture> newTexture = ReallocTexture(newHeight);
        if (!newTexture)
        {
          FT_Done_Glyph(glyph);
          CLog::LogF(LOGDEBUG, "Failed to allocate new texture of height {}", newHeight);
          return false;
        }
        m_texture = std::move(newTexture);
      }
      m_posY = GetMaxFontHeight();
    }

    if (!m_texture)
    {
      FT_Done_Glyph(glyph);
      CLog::LogF(LOGDEBUG, "no texture to cache character to");
      return false;
    }
  }

  // set the character in our table
  ch->m_glyphAndStyle = (style << 16) | glyphIndex;
  ch->m_glyphIndex = glyphIndex;
  ch->m_offsetX = static_cast<short>(bitGlyph->left);
  ch->m_offsetY = static_cast<short>(m_cellBaseLine - bitGlyph->top);
  ch->m_left = isEmptyGlyph ? 0.0f : (static_cast<float>(m_posX));
  ch->m_top = isEmptyGlyph ? 0.0f : (static_cast<float>(m_posY));
  ch->m_right = ch->m_left + bitmap.width;
  ch->m_bottom = ch->m_top + bitmap.rows;
  ch->m_advance =
      static_cast<float>(MathUtils::round_int(static_cast<double>(m_face->glyph->advance.x) / 64));

  // we need only render if we actually have some pixels
  if (!isEmptyGlyph)
  {
    // ensure our rect will stay inside the texture (it *should* but we need to be certain)
    unsigned int x1 = std::max(m_posX, 0);
    unsigned int y1 = std::max(m_posY, 0);
    unsigned int x2 = std::min(x1 + bitmap.width, m_textureWidth);
    unsigned int y2 = std::min(y1 + bitmap.rows, m_textureHeight);
    m_maxFontHeight = std::max(m_maxFontHeight, y2);
    CopyCharToTexture(bitGlyph, x1, y1, x2, y2);

    m_posX += SPACING_BETWEEN_CHARACTERS_IN_TEXTURE +
              static_cast<unsigned short>(ch->m_right - ch->m_left);
  }

  // free the glyph
  FT_Done_Glyph(glyph);

  return true;
}

void CGUIFontTTF::RenderCharacter(CGraphicContext& context,
                                  float posX,
                                  float posY,
                                  const Character* ch,
                                  UTILS::COLOR::Color color,
                                  bool roundX,
                                  std::vector<SVertex>& vertices)
{
  // actual image width isn't same as the character width as that is
  // just baseline width and height should include the descent
  const float width = ch->m_right - ch->m_left;
  const float height = ch->m_bottom - ch->m_top;

  // return early if nothing to render
  if (width == 0 || height == 0)
    return;

  // posX and posY are relative to our origin, and the textcell is offset
  // from our (posX, posY).  Plus, these are unscaled quantities compared to the underlying GUI resolution
#if not defined(HAS_DX)
  CRect vertex((posX + ch->m_offsetX), (posY + ch->m_offsetY), (posX + ch->m_offsetX + width),
               (posY + ch->m_offsetY + height));
#else
  CRect vertex((posX + ch->m_offsetX) * context.GetGUIScaleX(),
               (posY + ch->m_offsetY) * context.GetGUIScaleY(),
               (posX + ch->m_offsetX + width) * context.GetGUIScaleX(),
               (posY + ch->m_offsetY + height) * context.GetGUIScaleY());
  vertex += CPoint(m_originX, m_originY);
#endif
  CRect texture(ch->m_left, ch->m_top, ch->m_right, ch->m_bottom);

#if defined(HAS_DX)
  if (!m_renderSystem->ScissorsCanEffectClipping())
    context.ClipRect(vertex, texture);

  // transform our positions - note, no scaling due to GUI calibration/resolution occurs
  float x[VERTEX_PER_GLYPH] = {context.ScaleFinalXCoord(vertex.x1, vertex.y1),
                               context.ScaleFinalXCoord(vertex.x2, vertex.y1),
                               context.ScaleFinalXCoord(vertex.x2, vertex.y2),
                               context.ScaleFinalXCoord(vertex.x1, vertex.y2)};

  if (roundX)
  {
    // We only round the "left" side of the character, and then use the direction of rounding to
    // move the "right" side of the character.  This ensures that a constant width is kept when rendering
    // the same letter at the same size at different places of the screen, avoiding the problem
    // of the "left" side rounding one way while the "right" side rounds the other way, thus getting
    // altering the width of thin characters substantially.  This only really works for positive
    // coordinates (due to the direction of truncation for negatives) but this is the only case that
    // really interests us anyway.
    float rx0 = static_cast<float>(MathUtils::round_int(static_cast<double>(x[0])));
    float rx3 = static_cast<float>(MathUtils::round_int(static_cast<double>(x[3])));
    x[1] = static_cast<float>(MathUtils::truncate_int(static_cast<double>(x[1])));
    x[2] = static_cast<float>(MathUtils::truncate_int(static_cast<double>(x[2])));
    if (x[0] > 0.0f && rx0 > x[0])
      x[1] += 1;
    else if (x[0] < 0.0f && rx0 < x[0])
      x[1] -= 1;
    if (x[3] > 0.0f && rx3 > x[3])
      x[2] += 1;
    else if (x[3] < 0.0f && rx3 < x[3])
      x[2] -= 1;
    x[0] = rx0;
    x[3] = rx3;
  }

  const float y[VERTEX_PER_GLYPH] = {
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x1, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x2, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x2, vertex.y2)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalYCoord(vertex.x1, vertex.y2))))};

  const float z[VERTEX_PER_GLYPH] = {
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x1, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x2, vertex.y1)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x2, vertex.y2)))),
      static_cast<float>(MathUtils::round_int(
          static_cast<double>(context.ScaleFinalZCoord(vertex.x1, vertex.y2))))};

  // tex coords converted to 0..1 range
  const float tl = texture.x1 * m_textureScaleX;
  const float tr = texture.x2 * m_textureScaleX;
  const float tt = texture.y1 * m_textureScaleY;
  const float tb = texture.y2 * m_textureScaleY;
#else
  // when scaling by shader, we have to grow the vertex and texture coords
  // by .5 or we would ommit pixels when animating.
  const float tl = (texture.x1 - .5f) * m_textureScaleX;
  const float tr = (texture.x2 + .5f) * m_textureScaleX;
  const float tt = (texture.y1 - .5f) * m_textureScaleY;
  const float tb = (texture.y2 + .5f) * m_textureScaleY;
#endif

  vertices.resize(vertices.size() + VERTEX_PER_GLYPH);
  SVertex* v = &vertices[vertices.size() - VERTEX_PER_GLYPH];
  m_color = color;

#if !defined(HAS_DX)
  uint8_t r = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  uint8_t g = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  uint8_t b = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  uint8_t a = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);
#endif

  for (int i = 0; i < VERTEX_PER_GLYPH; i++)
  {
#ifdef HAS_DX
    CD3DHelper::XMStoreColor(&v[i].col, color);
#else
    v[i].r = r;
    v[i].g = g;
    v[i].b = b;
    v[i].a = a;
#endif
  }

#if defined(HAS_DX)
  for (int i = 0; i < VERTEX_PER_GLYPH; i++)
  {
    v[i].x = x[i];
    v[i].y = y[i];
    v[i].z = z[i];
  }

  v[0].u = tl;
  v[0].v = tt;

  v[1].u = tr;
  v[1].v = tt;

  v[2].u = tr;
  v[2].v = tb;

  v[3].u = tl;
  v[3].v = tb;
#else
  // GL / GLES uses triangle strips, not quads, so have to rearrange the vertex order
  // GL uses vertex shaders to manipulate text rotation/translation/scaling/clipping.

  // nudge position to align with raster grid. messes up kerning, but also avoids
  // linear filtering (when not scaled/rotated).
  float xOffset = 0.0f;
  if (roundX)
    xOffset = (vertex.x1 - std::floor(vertex.x1));
  float yOffset = (vertex.y1 - std::floor(vertex.y1));

  v[0].u = tl;
  v[0].v = tt;
  v[0].x = vertex.x1 - xOffset - 0.5f;
  v[0].y = vertex.y1 - yOffset - 0.5f;
  v[0].z = 0;

  v[1].u = tl;
  v[1].v = tb;
  v[1].x = vertex.x1 - xOffset - 0.5f;
  v[1].y = vertex.y2 - yOffset + 0.5f;
  v[1].z = 0;

  v[2].u = tr;
  v[2].v = tt;
  v[2].x = vertex.x2 - xOffset + 0.5f;
  v[2].y = vertex.y1 - yOffset - 0.5f;
  v[2].z = 0;

  v[3].u = tr;
  v[3].v = tb;
  v[3].x = vertex.x2 - xOffset + 0.5f;
  v[3].y = vertex.y2 - yOffset + 0.5f;
  v[3].z = 0;
#endif
}

// Oblique code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTF::ObliqueGlyph(FT_GlyphSlot slot)
{
  /* only oblique outline glyphs */
  if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
    return;

  /* we don't touch the advance width */

  /* For italic, simply apply a shear transform, with an angle */
  /* of about 12 degrees.                                      */

  FT_Matrix transform;
  transform.xx = 0x10000L;
  transform.yx = 0x00000L;

  transform.xy = 0x06000L;
  transform.yy = 0x10000L;

  FT_Outline_Transform(&slot->outline, &transform);
}

// Embolden code - original taken from freetype2 (ftsynth.c)
void CGUIFontTTF::SetGlyphStrength(FT_GlyphSlot slot, int glyphStrength)
{
  if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
    return;

  /* some reasonable strength */
  FT_Pos strength = FT_MulFix(m_face->units_per_EM, m_face->size->metrics.y_scale) / glyphStrength;

  FT_BBox bbox_before, bbox_after;
  FT_Outline_Get_CBox(&slot->outline, &bbox_before);
  FT_Outline_Embolden(&slot->outline, strength); // ignore error
  FT_Outline_Get_CBox(&slot->outline, &bbox_after);

  FT_Pos dx = bbox_after.xMax - bbox_before.xMax;
  FT_Pos dy = bbox_after.yMax - bbox_before.yMax;

  if (slot->advance.x)
    slot->advance.x += dx;

  if (slot->advance.y)
    slot->advance.y += dy;

  slot->metrics.width += dx;
  slot->metrics.height += dy;
  slot->metrics.horiBearingY += dy;
  slot->metrics.horiAdvance += dx;
  slot->metrics.vertBearingX -= dx / 2;
  slot->metrics.vertBearingY += dy;
  slot->metrics.vertAdvance += dy;
}

float CGUIFontTTF::GetTabSpaceLength()
{
  const Character* c = GetCharacter(static_cast<character_t>('X'), 0);
  return c ? c->m_advance * TAB_SPACE_LENGTH : 28.0f * TAB_SPACE_LENGTH;
}
