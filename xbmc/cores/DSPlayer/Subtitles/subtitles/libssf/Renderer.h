/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "StringMap.h"
#include "Glyph.h"

namespace ssf
{
  template<class T>
  class Cache
  {
  protected:
    StringMapW<T> m_key2obj;
    std::list<CStdStringW> m_objs;
    size_t m_limit;

  public:
    Cache(size_t limit) {m_limit = max(1, limit);}
    virtual ~Cache() {clear();}

    void clear()
    {
      StringMap<T>::iterator pos = m_key2obj.begin();
      while(pos != m_key2obj.end()) { delete pos->first; pos++; }
      m_key2obj.clear();
      m_objs.clear();
    }

    void Add(const CStdStringW& key, T& obj, bool fFlush = true)
    {
      StringMapW<T>::iterator it = m_key2obj.find(key);
      if(it != m_key2obj.end()) delete it->second;
      else m_objs.push_back(key);

      m_key2obj[key] = obj;

      if(fFlush) Flush();
    }

    void Flush()
    {
      while(m_objs.size() > m_limit)
      {
        CStdStringW key = m_objs.front(); m_objs.pop_front();
        //ASSERT(m_key2obj.Lookup(key));
        delete m_key2obj[key];
        m_key2obj.erase( m_key2obj.find(key) );
      }
    }

    bool Lookup(const CStdStringW& key, T& val)
    {
      StringMap<T>::iterator it = m_key2obj.find(key);
      return (it != m_key2obj.end()) && it->second;
    }

    void Invalidate(const CStdStringW& key)
    {
      StringMap<T>::iterator it = m_key2obj.find(key);
      if(it != m_key2obj.end())
      {
        delete it->second;
        m_key2obj[key] = NULL;
      }
    }
  };

  class FontCache : public Cache<FontWrapper*> 
  {
  public:
    FontCache() : Cache(20) {}
    FontWrapper* Create(HDC hDC, const LOGFONT& lf);
  };

  class GlyphPathCache : public Cache<GlyphPath*>
  {
  public:
    GlyphPathCache() : Cache(100) {}
    GlyphPath* Create(HDC hDC, const FontWrapper* f, WCHAR c);
  };

  class Row : public std::list<boost::shared_ptr<Glyph>>
  {
  public:
    int ascent, descent, border, width;
    Row() {ascent = descent = border = width = 0;}
  };

  class RenderedSubtitle
  {
  public:
    Com::SmartRect m_spdrc;
    Com::SmartRect m_clip;
    std::list<boost::shared_ptr<Glyph>> m_glyphs;

    RenderedSubtitle(const Com::SmartRect& spdrc, const Com::SmartRect& clip) : m_spdrc(spdrc), m_clip(clip) {}
    virtual ~RenderedSubtitle() {}

    Com::SmartRect Draw(SubPicDesc& spd) const;
  };

  class RenderedSubtitleCache : public Cache<RenderedSubtitle*>
  {
  public:
    RenderedSubtitleCache() : Cache(10) {}
  };

  class SubRect
  {
  public:
    Com::SmartRect rect;
    float layer;
    SubRect() {}
    SubRect(const Com::SmartRect& r, float l) : rect(r), layer(l) {}
  };

  class SubRectAllocator : public StringMapW<SubRect>
  {
    Com::SmartSize vs;
    Com::SmartRect vr;
  public:
    void UpdateTarget(const Com::SmartSize& vs, const Com::SmartRect& vr);
    void GetRect(Com::SmartRect& rect, const Subtitle* s, const Align& align, int tlb, int brb);
  };

  class Renderer
  {
    HDC m_hDC;

    FontCache m_fc;
    GlyphPathCache m_gpc;
    RenderedSubtitleCache m_rsc;
    SubRectAllocator m_sra;

  public:
    Renderer();
    virtual ~Renderer();

    void NextSegment(std::list<boost::shared_ptr<Subtitle>>& subs);
    RenderedSubtitle* Lookup(const Subtitle* s, const Com::SmartSize& vs, const Com::SmartRect& vr);
  };
}