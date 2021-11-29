/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTF.h"
#include "windowing/GraphicContext.h"

#include <stdint.h>
#include <vector>

template<class Position, class Value>
class CGUIFontCacheImpl
{
  struct EntryList
  {
    using HashMap = std::multimap<size_t, CGUIFontCacheEntry<Position, Value>*>;
    using HashIter = typename HashMap::iterator;
    using AgeMap = std::multimap<size_t, HashIter>;

    ~EntryList() { Flush(); }
    HashIter Insert(size_t hash, CGUIFontCacheEntry<Position, Value>* v)
    {
      auto r(hashMap.insert(typename HashMap::value_type(hash, v)));
      if (r->second)
        ageMap.insert(typename AgeMap::value_type(r->second->m_lastUsedMillis, r));

      return r;
    }
    void Flush()
    {
      ageMap.clear();
      for (auto it = hashMap.begin(); it != hashMap.end(); ++it)
        delete (it->second);
      hashMap.clear();
    }
    typename HashMap::iterator FindKey(CGUIFontCacheKey<Position> key)
    {
      CGUIFontCacheHash<Position> hashGen;
      CGUIFontCacheKeysMatch<Position> keyMatch;
      auto range = hashMap.equal_range(hashGen(key));
      for (auto ret = range.first; ret != range.second; ++ret)
      {
        if (keyMatch(ret->second->m_key, key))
        {
          return ret;
        }
      }

      return hashMap.end();
    }
    void UpdateAge(HashIter it, size_t millis)
    {
      auto range = ageMap.equal_range(it->second->m_lastUsedMillis);
      for (auto ageit = range.first; ageit != range.second; ++ageit)
      {
        if (ageit->second == it)
        {
          ageMap.erase(ageit);
          ageMap.insert(typename AgeMap::value_type(millis, it));
          it->second->m_lastUsedMillis = millis;
          return;
        }
      }
    }

    HashMap hashMap;
    AgeMap ageMap;
  };

  EntryList m_list;
  CGUIFontCache<Position, Value>* m_parent;

public:
  explicit CGUIFontCacheImpl(CGUIFontCache<Position, Value>* parent) : m_parent(parent) {}
  Value& Lookup(const CGraphicContext& context,
                Position& pos,
                const std::vector<UTILS::COLOR::Color>& colors,
                const vecText& text,
                uint32_t alignment,
                float maxPixelWidth,
                bool scrolling,
                unsigned int nowMillis,
                bool& dirtyCache);
  void Flush();
};

template<class Position, class Value>
CGUIFontCacheEntry<Position, Value>::~CGUIFontCacheEntry()
{
  delete &m_key.m_colors;
  delete &m_key.m_text;
  m_value.clear();
}

template<class Position, class Value>
void CGUIFontCacheEntry<Position, Value>::Assign(const CGUIFontCacheKey<Position>& key,
                                                 unsigned int nowMillis)
{
  m_key.m_pos = key.m_pos;
  m_key.m_colors.assign(key.m_colors.begin(), key.m_colors.end());
  m_key.m_text.assign(key.m_text.begin(), key.m_text.end());
  m_key.m_alignment = key.m_alignment;
  m_key.m_maxPixelWidth = key.m_maxPixelWidth;
  m_key.m_scrolling = key.m_scrolling;
  m_matrix = key.m_matrix;
  m_key.m_scaleX = key.m_scaleX;
  m_key.m_scaleY = key.m_scaleY;
  m_lastUsedMillis = nowMillis;
  m_value.clear();
}

template<class Position, class Value>
CGUIFontCache<Position, Value>::CGUIFontCache(CGUIFontTTF& font)
  : m_impl(new CGUIFontCacheImpl<Position, Value>(this)), m_font(font)
{
}

template<class Position, class Value>
CGUIFontCache<Position, Value>::~CGUIFontCache()
{
  delete m_impl;
}

template<class Position, class Value>
Value& CGUIFontCache<Position, Value>::Lookup(const CGraphicContext& context,
                                              Position& pos,
                                              const std::vector<UTILS::COLOR::Color>& colors,
                                              const vecText& text,
                                              uint32_t alignment,
                                              float maxPixelWidth,
                                              bool scrolling,
                                              unsigned int nowMillis,
                                              bool& dirtyCache)
{
  if (!m_impl)
    m_impl = new CGUIFontCacheImpl<Position, Value>(this);

  return m_impl->Lookup(context, pos, colors, text, alignment, maxPixelWidth, scrolling, nowMillis,
                        dirtyCache);
}

template<class Position, class Value>
Value& CGUIFontCacheImpl<Position, Value>::Lookup(const CGraphicContext& context,
                                                  Position& pos,
                                                  const std::vector<UTILS::COLOR::Color>& colors,
                                                  const vecText& text,
                                                  uint32_t alignment,
                                                  float maxPixelWidth,
                                                  bool scrolling,
                                                  unsigned int nowMillis,
                                                  bool& dirtyCache)
{
  const CGUIFontCacheKey<Position> key(pos, const_cast<std::vector<UTILS::COLOR::Color>&>(colors),
                                       const_cast<vecText&>(text), alignment, maxPixelWidth,
                                       scrolling, context.GetGUIMatrix(), context.GetGUIScaleX(),
                                       context.GetGUIScaleY());

  auto i = m_list.FindKey(key);
  if (i == m_list.hashMap.end())
  {
    // Cache miss
    dirtyCache = true;
    CGUIFontCacheEntry<Position, Value>* entry = nullptr;
    if (!m_list.ageMap.empty() &&
        (nowMillis - m_list.ageMap.begin()->first) > FONT_CACHE_TIME_LIMIT)
    {
      entry = m_list.ageMap.begin()->second->second;
      m_list.hashMap.erase(m_list.ageMap.begin()->second);
      m_list.ageMap.erase(m_list.ageMap.begin());
    }

    // add new entry
    CGUIFontCacheHash<Position> hashgen;
    if (!entry)
      entry = new CGUIFontCacheEntry<Position, Value>(*m_parent, key, nowMillis);
    else
      entry->Assign(key, nowMillis);

    return m_list.Insert(hashgen(key), entry)->second->m_value;
  }
  else
  {
    // Cache hit
    // Update the translation arguments so that they hold the offset to apply
    // to the cached values (but only in the dynamic case)
    pos.UpdateWithOffsets(i->second->m_key.m_pos, scrolling);

    // Update time in entry and move to the back of the list
    m_list.UpdateAge(i, nowMillis);

    dirtyCache = false;

    return i->second->m_value;
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
  m_list.Flush();
}

template CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::CGUIFontCache(
    CGUIFontTTF& font);
template CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::~CGUIFontCache();
template CGUIFontCacheEntry<CGUIFontCacheStaticPosition,
                            CGUIFontCacheStaticValue>::~CGUIFontCacheEntry();
template CGUIFontCacheStaticValue& CGUIFontCache<
    CGUIFontCacheStaticPosition,
    CGUIFontCacheStaticValue>::Lookup(const CGraphicContext& context,
                                      CGUIFontCacheStaticPosition&,
                                      const std::vector<UTILS::COLOR::Color>&,
                                      const vecText&,
                                      uint32_t,
                                      float,
                                      bool,
                                      unsigned int,
                                      bool&);
template void CGUIFontCache<CGUIFontCacheStaticPosition, CGUIFontCacheStaticValue>::Flush();

template CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::CGUIFontCache(
    CGUIFontTTF& font);
template CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::~CGUIFontCache();
template CGUIFontCacheEntry<CGUIFontCacheDynamicPosition,
                            CGUIFontCacheDynamicValue>::~CGUIFontCacheEntry();
template CGUIFontCacheDynamicValue& CGUIFontCache<
    CGUIFontCacheDynamicPosition,
    CGUIFontCacheDynamicValue>::Lookup(const CGraphicContext& context,
                                       CGUIFontCacheDynamicPosition&,
                                       const std::vector<UTILS::COLOR::Color>&,
                                       const vecText&,
                                       uint32_t,
                                       float,
                                       bool,
                                       unsigned int,
                                       bool&);
template void CGUIFontCache<CGUIFontCacheDynamicPosition, CGUIFontCacheDynamicValue>::Flush();

void CVertexBuffer::clear()
{
  if (m_font)
    m_font->DestroyVertexBuffer(*this);
}
