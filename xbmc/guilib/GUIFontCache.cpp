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

#if defined(TARGET_DARWIN )
// apple clang with c++11 doesn't like
// the forwards declarations of boost
// disable them ...
#define BOOST_DETAIL_NO_CONTAINER_FWD 1
#endif

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include "boost/multi_index/hashed_index.hpp"
#include "boost/multi_index/member.hpp"

using namespace boost::multi_index;

template<class Position, class Value>
class CGUIFontCacheImpl
{
  /* Empty structs used as tags to identify indexes */
  struct Age {};
  struct Hash {};

  typedef multi_index_container <
    CGUIFontCacheEntry<Position, Value>,
    indexed_by<
    sequenced<tag<Age> >,
    hashed_unique<tag<Hash>,
    member<CGUIFontCacheEntry<Position, Value>,
    CGUIFontCacheKey<Position>, &CGUIFontCacheEntry<Position, Value>::m_key
    >,
    CGUIFontCacheHash<Position>, CGUIFontCacheKeysMatch < Position >
    >
    >
  > EntryList;

  typedef typename EntryList::template index<Age>::type::iterator EntryAgeIterator;
  typedef typename EntryList::template index<Hash>::type::iterator EntryHashIterator;

  EntryList m_list;
  CGUIFontCache<Position, Value> *m_parent;

public:

  CGUIFontCacheImpl(CGUIFontCache<Position, Value>* parent) : m_parent(parent) {}
  Value &Lookup(Position &pos,
                const vecColors &colors, const vecText &text,
                uint32_t alignment, float maxPixelWidth,
                bool scrolling,
                unsigned int nowMillis, bool &dirtyCache);
  void Flush();
};

template<class Position, class Value>
void CGUIFontCacheEntry<Position, Value>::Reassign::operator()(CGUIFontCacheEntry<Position, Value> &entry)
{
  entry.m_key.m_pos = m_key.m_pos;
  entry.m_key.m_colors.assign(m_key.m_colors.begin(), m_key.m_colors.end());
  entry.m_key.m_text.assign(m_key.m_text.begin(), m_key.m_text.end());
  entry.m_key.m_alignment = m_key.m_alignment;
  entry.m_key.m_maxPixelWidth = m_key.m_maxPixelWidth;
  entry.m_key.m_scrolling = m_key.m_scrolling;
  entry.m_matrix = m_key.m_matrix;
  entry.m_key.m_scaleX = m_key.m_scaleX;
  entry.m_key.m_scaleY = m_key.m_scaleY;

  entry.m_lastUsedMillis = m_nowMillis;
  entry.m_value.clear();
}

template<class Position, class Value>
CGUIFontCacheEntry<Position, Value>::~CGUIFontCacheEntry()
{
  delete &m_key.m_colors;
  delete &m_key.m_text;
  m_value.clear();
}

template<class Position, class Value>
CGUIFontCache<Position, Value>::CGUIFontCache(CGUIFontTTFBase &font)
: m_impl(new CGUIFontCacheImpl<Position, Value>(this))
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
    m_impl = new CGUIFontCacheImpl<Position, Value>(this);

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
  auto i = m_list.template get<Hash>().find(key);
  if (i == m_list.template get<Hash>().end())
  {
    /* Cache miss */
    EntryAgeIterator oldest = m_list.template get<Age>().begin();
    if (!m_list.template get<Age>().empty() && nowMillis - oldest->m_lastUsedMillis > FONT_CACHE_TIME_LIMIT)
    {
      /* The oldest existing entry is old enough to expire and reuse */
      m_list.template get<Hash>().modify(m_list.template project<Hash>(oldest), typename CGUIFontCacheEntry<Position, Value>::Reassign(key, nowMillis));
      m_list.template get<Age>().relocate(m_list.template get<Age>().end(), oldest);
    }
    else
    {
      /* We need a new entry instead */
      /* Yes, this causes the creation an destruction of a temporary entry, but
       * this code ought to only be used infrequently, when the cache needs to grow */
      m_list.template get<Age>().push_back(CGUIFontCacheEntry<Position, Value>(*m_parent, key, nowMillis));
    }
    dirtyCache = true;
    return (--m_list.template get<Age>().end())->m_value;
  }
  else
  {
    /* Cache hit */
    /* Update the translation arguments so that they hold the offset to apply
     * to the cached values (but only in the dynamic case) */
    pos.UpdateWithOffsets(i->m_key.m_pos, scrolling);
    /* Update time in entry and move to the back of the list */
    i->m_lastUsedMillis = nowMillis;
    m_list.template get<Age>().relocate(m_list.template get<Age>().end(), m_list.template project<Age>(i));
    dirtyCache = false;
    return i->m_value;
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
  m_list.template get<Age>().clear();
}

template CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::CGUIFontCache(CGUIFontTTFBase &font);
template CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::~CGUIFontCache();
template void CGUIFontCacheEntry<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::Reassign::operator()(CGUIFontCacheEntry<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue> &entry);
template CGUIFontCacheEntry<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::~CGUIFontCacheEntry();
template CGUIFontCacheStaticValue &CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::Lookup(CGUIFontCacheStaticPosition &, const vecColors &, const vecText &, uint32_t, float, bool, unsigned int, bool &);
template void CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::Flush();

template CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::CGUIFontCache(CGUIFontTTFBase &font);
template CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::~CGUIFontCache();
template void CGUIFontCacheEntry<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::Reassign::operator()(CGUIFontCacheEntry<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue> &entry);
template CGUIFontCacheEntry<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::~CGUIFontCacheEntry();
template CGUIFontCacheDynamicValue &CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::Lookup(CGUIFontCacheDynamicPosition &, const vecColors &, const vecText &, uint32_t, float, bool, unsigned int, bool &);
template void CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::Flush();

void CVertexBuffer::clear()
{
  if (m_font != NULL)
    m_font->DestroyVertexBuffer(*this);
}
