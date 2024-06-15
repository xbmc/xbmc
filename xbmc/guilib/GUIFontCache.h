/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFontCache.h
\brief
*/

#include "utils/ColorUtils.h"
#include "utils/TransformMatrix.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <stdint.h>
#include <vector>

constexpr float FONT_CACHE_DIST_LIMIT = 0.01f;

class CGraphicContext;

template<class Position, class Value>
class CGUIFontCache;
class CGUIFontTTF;

template<class Position, class Value>
class CGUIFontCacheImpl;

template<class Position>
struct CGUIFontCacheKey
{
  Position m_pos;
  std::vector<KODI::UTILS::COLOR::Color>& m_colors;
  vecText& m_text;
  uint32_t m_alignment;
  float m_maxPixelWidth;
  bool m_scrolling;
  const TransformMatrix& m_matrix;
  float m_scaleX;
  float m_scaleY;

  CGUIFontCacheKey(Position pos,
                   std::vector<KODI::UTILS::COLOR::Color>& colors,
                   vecText& text,
                   uint32_t alignment,
                   float maxPixelWidth,
                   bool scrolling,
                   const TransformMatrix& matrix,
                   float scaleX,
                   float scaleY)
    : m_pos(pos),
      m_colors(colors),
      m_text(text),
      m_alignment(alignment),
      m_maxPixelWidth(maxPixelWidth),
      m_scrolling(scrolling),
      m_matrix(matrix),
      m_scaleX(scaleX),
      m_scaleY(scaleY)
  {
  }
};

template<class Position, class Value>
struct CGUIFontCacheEntry
{
  const CGUIFontCache<Position, Value>& m_cache;
  CGUIFontCacheKey<Position> m_key;
  TransformMatrix m_matrix;
  std::chrono::steady_clock::time_point m_lastUsed;
  Value m_value;

  CGUIFontCacheEntry(const CGUIFontCache<Position, Value>& cache,
                     const CGUIFontCacheKey<Position>& key,
                     std::chrono::steady_clock::time_point now)
    : m_cache(cache),
      m_key(key.m_pos,
            *new std::vector<KODI::UTILS::COLOR::Color>,
            *new vecText,
            key.m_alignment,
            key.m_maxPixelWidth,
            key.m_scrolling,
            m_matrix,
            key.m_scaleX,
            key.m_scaleY),
      m_lastUsed(now)
  {
    m_key.m_colors.assign(key.m_colors.begin(), key.m_colors.end());
    m_key.m_text.assign(key.m_text.begin(), key.m_text.end());
    m_matrix = key.m_matrix;
  }

  ~CGUIFontCacheEntry();

  void Assign(const CGUIFontCacheKey<Position>& key, std::chrono::steady_clock::time_point now);
};

template<class Position>
struct CGUIFontCacheHash
{
  size_t operator()(const CGUIFontCacheKey<Position>& key) const
  {
    /* Not much effort has gone into choosing this hash function */
    size_t hash = 0, i;
    for (i = 0; i < 3 && i < key.m_text.size(); ++i)
      hash += key.m_text[i];
    if (key.m_colors.size())
      hash += key.m_colors[0];
    hash += static_cast<size_t>(MatrixHashContribution(key)); // horrible
    return hash;
  }
};

template<class Position>
struct CGUIFontCacheKeysMatch
{
  bool operator()(const CGUIFontCacheKey<Position>& a, const CGUIFontCacheKey<Position>& b) const
  {
    // clang-format off
    return a.m_text == b.m_text &&
           a.m_colors == b.m_colors &&
           a.m_alignment == b.m_alignment &&
           a.m_scrolling == b.m_scrolling &&
           a.m_maxPixelWidth == b.m_maxPixelWidth &&
           Match(a.m_pos, a.m_matrix, b.m_pos, b.m_matrix, a.m_scrolling) &&
           a.m_scaleX == b.m_scaleX &&
           a.m_scaleY == b.m_scaleY;
    // clang-format on
  }
};


template<class Position, class Value>
class CGUIFontCache
{
  std::unique_ptr<CGUIFontCacheImpl<Position, Value>> m_impl;

  CGUIFontCache(const CGUIFontCache<Position, Value>&) = delete;
  const CGUIFontCache<Position, Value>& operator=(const CGUIFontCache<Position, Value>&) = delete;

public:
  const CGUIFontTTF& m_font;

  explicit CGUIFontCache(CGUIFontTTF& font);

  ~CGUIFontCache();

  Value& Lookup(const CGraphicContext& context,
                Position& pos,
                const std::vector<KODI::UTILS::COLOR::Color>& colors,
                const vecText& text,
                uint32_t alignment,
                float maxPixelWidth,
                bool scrolling,
                std::chrono::steady_clock::time_point now,
                bool& dirtyCache);
  void Flush();
};

struct CGUIFontCacheStaticPosition
{
  float m_x;
  float m_y;
  CGUIFontCacheStaticPosition(float x, float y) : m_x(x), m_y(y) {}
  void UpdateWithOffsets(const CGUIFontCacheStaticPosition& cached, bool scrolling) {}
};

struct CGUIFontCacheStaticValue : public std::shared_ptr<std::vector<SVertex>>
{
  void clear()
  {
    if (*this)
      (*this)->clear();
  }
};

inline bool Match(const CGUIFontCacheStaticPosition& a,
                  const TransformMatrix& a_m,
                  const CGUIFontCacheStaticPosition& b,
                  const TransformMatrix& b_m,
                  bool scrolling)
{
  return a.m_x == b.m_x && a.m_y == b.m_y && a_m == b_m;
}

inline float MatrixHashContribution(const CGUIFontCacheKey<CGUIFontCacheStaticPosition>& a)
{
  /* Ensure horizontally translated versions end up in different buckets */
  return a.m_matrix.m[0][3];
}

struct CGUIFontCacheDynamicPosition
{
  float m_x;
  float m_y;
  float m_z;
  CGUIFontCacheDynamicPosition() = default;
  CGUIFontCacheDynamicPosition(float x, float y, float z) : m_x(x), m_y(y), m_z(z) {}
  void UpdateWithOffsets(const CGUIFontCacheDynamicPosition& cached, bool scrolling)
  {
    if (scrolling)
      m_x = m_x - cached.m_x;
    else
      m_x = floorf(m_x - cached.m_x + FONT_CACHE_DIST_LIMIT);
    m_y = floorf(m_y - cached.m_y + FONT_CACHE_DIST_LIMIT);
    m_z = floorf(m_z - cached.m_z + FONT_CACHE_DIST_LIMIT);
  }
};

struct CVertexBuffer
{
#if defined(HAS_GL) || defined(HAS_GLES)
  typedef unsigned int BufferHandleType;
#define BUFFER_HANDLE_INIT 0
#elif defined(HAS_DX)
  typedef void* BufferHandleType;
#define BUFFER_HANDLE_INIT nullptr
#endif
  BufferHandleType bufferHandle = BUFFER_HANDLE_INIT; // this is really a GLuint
  size_t size = 0;
  CVertexBuffer() : m_font(nullptr) {}
  CVertexBuffer(BufferHandleType bufferHandle, size_t size, const CGUIFontTTF* font)
    : bufferHandle(bufferHandle), size(size), m_font(font)
  {
  }
  CVertexBuffer(const CVertexBuffer& other)
    : bufferHandle(other.bufferHandle), size(other.size), m_font(other.m_font)
  {
    /* In practice, the copy constructor is only called before a vertex buffer
     * has been attached. If this should ever change, we'll need another support
     * function in GUIFontTTFGL/DX to duplicate a buffer, given its handle. */
    assert(other.bufferHandle == 0);
  }
  CVertexBuffer& operator=(CVertexBuffer& other)
  {
    /* This is used with move-assignment semantics for initialising the object in the font cache */
    assert(bufferHandle == 0);
    bufferHandle = other.bufferHandle;
    other.bufferHandle = 0;
    size = other.size;
    m_font = other.m_font;
    return *this;
  }
  void clear();

private:
  const CGUIFontTTF* m_font;
};

typedef CVertexBuffer CGUIFontCacheDynamicValue;

inline bool Match(const CGUIFontCacheDynamicPosition& a,
                  const TransformMatrix& a_m,
                  const CGUIFontCacheDynamicPosition& b,
                  const TransformMatrix& b_m,
                  bool scrolling)
{
  float diffX = a.m_x - b.m_x + FONT_CACHE_DIST_LIMIT;
  float diffY = a.m_y - b.m_y + FONT_CACHE_DIST_LIMIT;
  float diffZ = a.m_z - b.m_z + FONT_CACHE_DIST_LIMIT;
  // clang-format off
  return (scrolling ||
          diffX - floorf(diffX) < 2 * FONT_CACHE_DIST_LIMIT) &&
          diffY - floorf(diffY) < 2 * FONT_CACHE_DIST_LIMIT &&
          diffZ - floorf(diffZ) < 2 * FONT_CACHE_DIST_LIMIT &&
          a_m.m[0][0] == b_m.m[0][0] &&
          a_m.m[1][1] == b_m.m[1][1] &&
          a_m.m[2][2] == b_m.m[2][2];
  // clang-format on
  // We already know the first 3 columns of both matrices are diagonal, so no need to check the other elements
}

inline float MatrixHashContribution(const CGUIFontCacheKey<CGUIFontCacheDynamicPosition>& a)
{
  return 0;
}
