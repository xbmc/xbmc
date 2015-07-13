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

#include <stdint.h>
#include <vector>
#include "GUIFontTTF.h"
#include "GraphicContext.h"

inline bool Match(const CGUIFontCacheStaticPosition &a, const TransformMatrix &a_m,
  const CGUIFontCacheStaticPosition &b, const TransformMatrix &b_m,
  bool scrolling)
{
  return a.m_x == b.m_x && a.m_y == b.m_y && a_m == b_m;
}

inline bool Match(const CGUIFontCacheDynamicPosition &a, const TransformMatrix &a_m,
  const CGUIFontCacheDynamicPosition &b, const TransformMatrix &b_m,
  bool scrolling)
{
  float diffX = a.m_x - b.m_x + FONT_CACHE_DIST_LIMIT;
  float diffY = a.m_y - b.m_y + FONT_CACHE_DIST_LIMIT;
  float diffZ = a.m_z - b.m_z + FONT_CACHE_DIST_LIMIT;
  return (scrolling || diffX - floorf(diffX) < 2 * FONT_CACHE_DIST_LIMIT) &&
    diffY - floorf(diffY) < 2 * FONT_CACHE_DIST_LIMIT &&
    diffZ - floorf(diffZ) < 2 * FONT_CACHE_DIST_LIMIT &&
    a_m.m[0][0] == b_m.m[0][0] &&
    a_m.m[1][1] == b_m.m[1][1] &&
    a_m.m[2][2] == b_m.m[2][2];
  // We already know the first 3 columns of both matrices are diagonal, so no need to check the other elements
}

template<class Position, class Value>
class CGUIFontCacheImpl
{
  std::list<CGUIFontCacheEntry<Position, Value>*> m_list;

public:

  CGUIFontCacheImpl() {}
  Value &Lookup(Position &pos,
                const vecColors &colors, const vecText &text,
                uint32_t alignment, float maxPixelWidth,
                bool scrolling,
                unsigned int nowMillis, bool &dirtyCache);
  void Flush();
};

template<class Position, class Value>
CGUIFontCacheEntry<Position, Value>::~CGUIFontCacheEntry()
{
  delete &m_key.m_colors;
  delete &m_key.m_text;
}

template<class Position, class Value>
CGUIFontCache<Position, Value>::CGUIFontCache(CGUIFontTTFBase &font)
: m_impl(new CGUIFontCacheImpl<Position, Value>())
, m_font(font)
{
}

template<class Position, class Value>
CGUIFontCache<Position, Value>::~CGUIFontCache()
{
  delete m_impl;
}

template<class Position, class Value>
Value &CGUIFontCache<Position, Value>::Lookup(Position &pos,
                                              const vecColors &colors, const vecText &text,
                                              uint32_t alignment, float maxPixelWidth,
                                              bool scrolling,
                                              unsigned int nowMillis, bool &dirtyCache)
{
  if (m_impl == nullptr)
    m_impl = new CGUIFontCacheImpl<Position, Value>();

  return m_impl->Lookup(pos, colors, text, alignment, maxPixelWidth, scrolling, nowMillis, dirtyCache);
}

template<class Position, class Value>
Value &CGUIFontCacheImpl<Position, Value>::Lookup(Position &pos,
                                                  const vecColors &colors, const vecText &text,
                                                  uint32_t alignment, float maxPixelWidth,
                                                  bool scrolling,
                                                  unsigned int nowMillis, bool &dirtyCache)
{
  const CGUIFontCacheKey<Position> key(pos,
                                       const_cast<vecColors &>(colors), const_cast<vecText &>(text),
                                       alignment, maxPixelWidth,
                                       scrolling, g_graphicsContext.GetGUIMatrix(),
                                       g_graphicsContext.GetGUIScaleX(), g_graphicsContext.GetGUIScaleY());
  auto i = std::find_if(m_list.begin(), m_list.end(), [&key](CGUIFontCacheEntry<Position, Value>* entry) 
  {
    return key.m_text ==          entry->m_key.m_text &&
           key.m_colors ==        entry->m_key.m_colors &&
           key.m_alignment ==     entry->m_key.m_alignment &&
           key.m_scrolling ==     entry->m_key.m_scrolling &&
           key.m_maxPixelWidth == entry->m_key.m_maxPixelWidth &&
           key.m_scaleX ==        entry->m_key.m_scaleX &&
           key.m_scaleY ==        entry->m_key.m_scaleY &&
           Match(entry->m_key.m_pos, entry->m_key.m_matrix, key.m_pos, key.m_matrix, entry->m_key.m_scrolling);
  });

  if (i == m_list.end())
  {
    /* Cache miss */
    auto oldest = m_list.back();
    if (!m_list.empty() && nowMillis - oldest->m_lastUsedMillis > FONT_CACHE_TIME_LIMIT)
    {
      /* The oldest existing entry is old enough to expire and reuse */
      m_list.pop_back();
      delete oldest;
      m_list.push_front(new CGUIFontCacheEntry<Position, Value>(key, nowMillis));
    }
    else
    {
      /* We need a new entry instead */
      /* Yes, this causes the creation an destruction of a temporary entry, but
       * this code ought to only be used infrequently, when the cache needs to grow */
      m_list.push_front(new CGUIFontCacheEntry<Position, Value>(key, nowMillis));
    }
    dirtyCache = true;
    return m_list.back()->m_value;
  }
  else
  {
    auto* entry = (*i);
    /* Cache hit */
    /* Update the translation arguments so that they hold the offset to apply
     * to the cached values (but only in the dynamic case) */
    pos.UpdateWithOffsets(entry->m_key.m_pos, scrolling);
    /* Update time in entry and move to the back of the list */
    entry->m_lastUsedMillis = nowMillis;
    m_list.remove(*i);
    m_list.push_front(entry);
    dirtyCache = false;
    return entry->m_value;
  }
}

template<class Position, class Value>
void CGUIFontCache<Position, Value>::Flush()
{
  m_impl->Flush();
}

template<class Position, class Value>
void CGUIFontCacheImpl<Position, Value>::Flush()
{
  for (auto* i : m_list)
    delete i;
  m_list.clear();
}

template CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::CGUIFontCache(CGUIFontTTFBase &font);
template CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::~CGUIFontCache();
template CGUIFontCacheEntry<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::~CGUIFontCacheEntry();
template CGUIFontCacheStaticValue &CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::Lookup(CGUIFontCacheStaticPosition &, const vecColors &, const vecText &, uint32_t, float, bool, unsigned int, bool &);
template void CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::Flush();

template CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::CGUIFontCache(CGUIFontTTFBase &font);
template CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::~CGUIFontCache();
template CGUIFontCacheEntry<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::~CGUIFontCacheEntry();
template CGUIFontCacheDynamicValue &CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::Lookup(CGUIFontCacheDynamicPosition &, const vecColors &, const vecText &, uint32_t, float, bool, unsigned int, bool &);
template void CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::Flush();

void CVertexBuffer::clear()
{
  if (m_font != nullptr)
    m_font->DestroyVertexBuffer(*this);
}
