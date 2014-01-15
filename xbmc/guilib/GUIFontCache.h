/*!
\file GUIFontCache.h
\brief
*/

#ifndef CGUILIB_GUIFONTCACHE_H
#define CGUILIB_GUIFONTCACHE_H
#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <cstddef>
#include <cstring>
#include <stdint.h>

#include <algorithm>
#include <vector>

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include "boost/multi_index/hashed_index.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/shared_ptr.hpp"

#include "TransformMatrix.h"

using namespace boost::multi_index;

#define FONT_CACHE_TIME_LIMIT (1000)

template<class Position, class Value> class CGUIFontCache;
class CGUIFontTTFBase;

template<class Position>
struct CGUIFontCacheKey
{
  Position m_pos;
  vecColors &m_colors;
  vecText &m_text;
  uint32_t m_alignment;
  float m_maxPixelWidth;
  bool m_scrolling;
  const TransformMatrix &m_matrix;
  float m_scaleX;
  float m_scaleY;

  CGUIFontCacheKey(Position pos,
                   vecColors &colors, vecText &text,
                   uint32_t alignment, float maxPixelWidth,
                   bool scrolling, const TransformMatrix &matrix,
                   float scaleX, float scaleY) :
    m_pos(pos),
    m_colors(colors), m_text(text),
    m_alignment(alignment), m_maxPixelWidth(maxPixelWidth),
    m_scrolling(scrolling), m_matrix(matrix),
    m_scaleX(scaleX), m_scaleY(scaleY)
  {}
};

template<class Position, class Value>
struct CGUIFontCacheEntry
{
  const CGUIFontCache<Position, Value> &m_cache;
  CGUIFontCacheKey<Position> m_key;
  TransformMatrix m_matrix;

  /* These need to be declared as mutable to get round the fact that only
   * const iterators are available. These fields do not affect comparison or
   * hash functors, so from the container's point of view, they are mutable. */
  mutable unsigned int m_lastUsedMillis;
  mutable Value m_value;

  CGUIFontCacheEntry(const CGUIFontCache<Position, Value> &cache, const CGUIFontCacheKey<Position> &key, unsigned int nowMillis) :
    m_cache(cache),
    m_key(key.m_pos,
          *new vecColors, *new vecText,
          key.m_alignment, key.m_maxPixelWidth,
          key.m_scrolling, m_matrix,
          key.m_scaleX, key.m_scaleY),
    m_lastUsedMillis(nowMillis)
  {
    m_key.m_colors.assign(key.m_colors.begin(), key.m_colors.end());
    m_key.m_text.assign(key.m_text.begin(), key.m_text.end());
    m_matrix = key.m_matrix;
  }

  CGUIFontCacheEntry(const CGUIFontCacheEntry &other) :
    m_cache(other.m_cache),
    m_key(other.m_key.m_pos,
          *new vecColors, *new vecText,
          other.m_key.m_alignment, other.m_key.m_maxPixelWidth,
          other.m_key.m_scrolling, m_matrix,
          other.m_key.m_scaleX, other.m_key.m_scaleY),
    m_lastUsedMillis(other.m_lastUsedMillis),
    m_value(other.m_value)
  {
    m_key.m_colors.assign(other.m_key.m_colors.begin(), other.m_key.m_colors.end());
    m_key.m_text.assign(other.m_key.m_text.begin(), other.m_key.m_text.end());
    m_matrix = other.m_key.m_matrix;
  }

  struct Reassign
  {
    Reassign(const CGUIFontCacheKey<Position> &key, unsigned int nowMillis) : m_key(key), m_nowMillis(nowMillis) {}
    void operator()(CGUIFontCacheEntry &entry);
  private:
    const CGUIFontCacheKey<Position> &m_key;
    unsigned int m_nowMillis;
  };

  ~CGUIFontCacheEntry();
};

template<class Position>
struct CGUIFontCacheHash
{
  size_t operator()(const CGUIFontCacheKey<Position> &key) const
  {
    /* Not much effort has gone into choosing this hash function */
    size_t hash = 0, i;
    for (i = 0; i < 3 && i < key.m_text.size(); ++i)
      hash += key.m_text[i];
    if (key.m_colors.size())
      hash += key.m_colors[0];
    hash += MatrixHashContribution(key);
    return hash;
  }
};

template<class Position>
struct CGUIFontCacheKeysMatch
{
  bool operator()(const CGUIFontCacheKey<Position> &a, const CGUIFontCacheKey<Position> &b) const
  {
    return a.m_text == b.m_text &&
           a.m_colors == b.m_colors &&
           a.m_alignment == b.m_alignment &&
           a.m_scrolling == b.m_scrolling &&
           a.m_maxPixelWidth == b.m_maxPixelWidth &&
           Match(a.m_pos, a.m_matrix, b.m_pos, b.m_matrix, a.m_scrolling) &&
           a.m_scaleX == b.m_scaleX &&
           a.m_scaleY == b.m_scaleY;
  }
};

template<class Position, class Value>
class CGUIFontCache
{
  /* Empty structs used as tags to identify indexes */
  struct Age {};
  struct Hash {};

  typedef multi_index_container<
      CGUIFontCacheEntry<Position, Value>,
      indexed_by<
          sequenced<tag<Age> >,
          hashed_unique<tag<Hash>, member<CGUIFontCacheEntry<Position, Value>, CGUIFontCacheKey<Position>, &CGUIFontCacheEntry<Position, Value>::m_key>, CGUIFontCacheHash<Position>, CGUIFontCacheKeysMatch<Position> >
      >
  > EntryList;

  typedef typename EntryList::template index<Age>::type::iterator EntryAgeIterator;
  typedef typename EntryList::template index<Hash>::type::iterator EntryHashIterator;

  EntryList m_list;

public:
  const CGUIFontTTFBase &m_font;

  CGUIFontCache(CGUIFontTTFBase &font) : m_font(font) {}
  Value &Lookup(Position &pos,
                const vecColors &colors, const vecText &text,
                uint32_t alignment, float maxPixelWidth,
                bool scrolling,
                unsigned int nowMillis, bool &dirtyCache);
  void Flush();
};

struct CGUIFontCacheStaticPosition
{
  float m_x;
  float m_y;
  CGUIFontCacheStaticPosition(float x, float y) : m_x(x), m_y(y) {}
};

struct CGUIFontCacheStaticValue : public boost::shared_ptr<std::vector<SVertex> >
{
  void clear()
  {
    if (*this)
      (*this)->clear();
  }
};

inline bool Match(const CGUIFontCacheStaticPosition &a, const TransformMatrix &a_m,
                  const CGUIFontCacheStaticPosition &b, const TransformMatrix &b_m,
                  bool scrolling)
{
  return a.m_x == b.m_x && a.m_y == b.m_y && a_m == b_m;
}

inline float MatrixHashContribution(const CGUIFontCacheKey<CGUIFontCacheStaticPosition> &a)
{
  /* Ensure horizontally translated versions end up in different buckets */
  return a.m_matrix.m[0][3];
}

#endif
