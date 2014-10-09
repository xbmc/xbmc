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
 *  TODO: do something about bidi (at least handle these ranges: 0590-07BF, FB1D-FDFF, FE70-FEFF)
 *
 */

#include "stdafx.h"
#include "Renderer.h"
#include "Arabic.h"

namespace ssf
{
  template <class T>
  void ReverseList(T& l)
  {
    POSITION pos = l.begin();
    while(pos)
    {
      POSITION cur = pos;
      l.GetNext(pos);
      l.push_front(l.GetAt(cur));
      l.RemoveAt(cur);
    }
  }

  static Com::SmartPoint GetAlignPoint(const Placement& placement, const Size& scale, const Com::SmartRect& frame, const Com::SmartSize& size)
  {
    Com::SmartPoint p;

    p.x = frame.left;
    p.x += placement.pos.auto_x 
      ? placement.align.h * (frame.Width() - size.cx)
      : placement.pos.x * scale.cx - placement.align.h * size.cx;

    p.y = frame.top;
    p.y += placement.pos.auto_y 
      ? placement.align.v * (frame.Height() - size.cy) 
      : placement.pos.y * scale.cy - placement.align.v * size.cy;

    return p;
  }

  static Com::SmartPoint GetAlignPoint(const Placement& placement, const Size& scale, const Com::SmartRect& frame)
  {
    Com::SmartSize size(0, 0);
    return GetAlignPoint(placement, scale, frame, size);
  }

  //

  Renderer::Renderer()
  {
    m_hDC = CreateCompatibleDC(NULL);
    SetBkMode(m_hDC, TRANSPARENT); 
    SetTextColor(m_hDC, 0xffffff); 
    SetMapMode(m_hDC, MM_TEXT);
  }

  Renderer::~Renderer()
  {
    DeleteDC(m_hDC);
  }

  void Renderer::NextSegment(std::list<boost::shared_ptr<Subtitle>>& subs)
  {
    StringMapW<bool> names;
    std::list<boost::shared_ptr<Subtitle>>::iterator it = subs.begin();
    for(; it != subs.end(); ++it) names[(*it).get()->m_name] = true;

    StringMapW<SubRect>::iterator it2 = m_sra.begin();
    for(; it2 != m_sra.end(); ++it2)
    {
      CStdStringW name = it2->first;
      if(names.find(name) == names.end()) m_sra.erase(it2);
    }
  }

  RenderedSubtitle* Renderer::Lookup(const Subtitle* s, const Com::SmartSize& vs, const Com::SmartRect& vr)
  {
    m_sra.UpdateTarget(vs, vr);

    if(s->m_text.empty())
      return NULL;

    Com::SmartRect spdrc = s->m_frame.reference == _T("video") ? vr : Com::SmartRect(Com::SmartPoint(0, 0), vs);

    if(spdrc.IsRectEmpty())
      return NULL;

    RenderedSubtitle* rs = NULL;

    if(m_rsc.Lookup(s->m_name, rs))
    {
      if(!s->m_animated && rs->m_spdrc == spdrc)
        return rs;

      m_rsc.Invalidate(s->m_name);
    }

    Style style = s->m_text.front().style;

    Size scale;

    scale.cx = (float)spdrc.Width() / s->m_frame.resolution.cx;
    scale.cy = (float)spdrc.Height() / s->m_frame.resolution.cy;

    Com::SmartRect frame;

    frame.left = (int)(64.0f * (spdrc.left + style.placement.margin.l * scale.cx) + 0.5);
    frame.top = (int)(64.0f * (spdrc.top + style.placement.margin.t * scale.cy) + 0.5);
    frame.right = (int)(64.0f * (spdrc.right - style.placement.margin.r * scale.cx) + 0.5);
    frame.bottom = (int)(64.0f * (spdrc.bottom - style.placement.margin.b * scale.cy) + 0.5);

    Com::SmartRect clip;

    if(style.placement.clip.l == -1) clip.left = 0;
    else clip.left = (int)(spdrc.left + style.placement.clip.l * scale.cx);
    if(style.placement.clip.t == -1) clip.top = 0;
    else clip.top = (int)(spdrc.top + style.placement.clip.t * scale.cy); 
    if(style.placement.clip.r == -1) clip.right = vs.cx;
    else clip.right = (int)(spdrc.left + style.placement.clip.r * scale.cx);
    if(style.placement.clip.b == -1) clip.bottom = vs.cy;
    else clip.bottom = (int)(spdrc.top + style.placement.clip.b * scale.cy);

    clip.left = max(clip.left, 0);
    clip.top = max(clip.top, 0);
    clip.right = min(clip.right, vs.cx);
    clip.bottom = min(clip.bottom, vs.cy);

    scale.cx *= 64;
    scale.cy *= 64;

    bool vertical = s->m_direction.primary == _T("down") || s->m_direction.primary == _T("up");

    // create glyph paths

    WCHAR c_prev = 0, c_next;

    std::list<boost::shared_ptr<Glyph>> glyphs;

    std::list<Text>::const_iterator it = s->m_text.begin();
    for(; it != s->m_text.end(); ++it)
    {
      Text t = *it;

      LOGFONT lf;
      memset(&lf, 0, sizeof(lf));
      lf.lfCharSet = DEFAULT_CHARSET;
      _tcscpy_s(lf.lfFaceName, CStdString(t.style.font.face));
      lf.lfHeight = (LONG)(t.style.font.size * scale.cy + 0.5);
      lf.lfWeight = (LONG)(t.style.font.weight + 0.5);
      lf.lfItalic = !!t.style.font.italic;
      lf.lfUnderline = !!t.style.font.underline;
      lf.lfStrikeOut = !!t.style.font.strikethrough;
      lf.lfOutPrecision = OUT_TT_PRECIS;
      lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
      lf.lfQuality = ANTIALIASED_QUALITY;
      lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;

      FontWrapper* font;

      if(!(font = m_fc.Create(m_hDC, lf)))
      {
        _tcscpy_s(lf.lfFaceName, _T("Arial"));

        if(!(font = m_fc.Create(m_hDC, lf)))
        {
          ASSERT(0);
          continue;
        }
      }

      HFONT hOldFont = SelectFont(m_hDC, *font);

      const TEXTMETRIC& tm = font->GetTextMetric();

      for(LPCWSTR c = t.str; *c; c++)
      {
        boost::shared_ptr<Glyph> g(DNew Glyph());

        g->c = *c;
        g->style = t.style;
        g->scale = scale;
        g->vertical = vertical;
        g->font = font;

        c_next = !c[1] && it != s->m_text.end() ? c_next = (*it).str[0] : c[1];
        Arabic::Replace(g->c, c_prev, c_next);
        c_prev = c[0];

        Com::SmartSize extent;
        GetTextExtentPoint32W(m_hDC, &g->c, 1, &extent);
        ASSERT(extent.cx >= 0 && extent.cy >= 0);

        if(vertical) 
        {
          g->spacing = (int)(t.style.font.spacing * scale.cy + 0.5);
          g->ascent = extent.cx / 2;
          g->descent = extent.cx - g->ascent;
          g->width = extent.cy;

          // TESTME
          if(g->c == Text::SP)
          {
            g->width /= 2;
          }
        }
        else
        {
          g->spacing = (int)(t.style.font.spacing * scale.cx + 0.5);
          g->ascent = tm.tmAscent;
          g->descent = tm.tmDescent;
          g->width = extent.cx;
        }

        if(g->c == Text::LSEP)
        {
          g->spacing = 0;
          g->width = 0;
          g->ascent /= 2;
          g->descent /= 2;
        }
        else
        {
          GlyphPath* path = m_gpc.Create(m_hDC, font, g->c);
          if(!path) {ASSERT(0); continue;}
          g->path = *path;
        }

        glyphs.push_back(g);
      }

      SelectFont(m_hDC, hOldFont);
    }

    // break glyphs into rows

    std::list<boost::shared_ptr<Row>> rows;
    boost::shared_ptr<Row> row;

    std::list<boost::shared_ptr<Glyph>>::iterator pos = glyphs.begin();
    for(; pos != glyphs.end(); ++pos)
    {
      if(! row.get()) row.reset(DNew Row());
      WCHAR c = pos->get()->c;
      row->push_back(*pos);
      if(c == Text::LSEP || (pos == glyphs.end())) rows.push_back(row);
    }

    // kerning

    if(s->m_direction.primary == _T("right")) // || s->m_direction.primary == _T("left")
    {
      for(std::list<boost::shared_ptr<Row>>::iterator rpos = rows.begin(); rpos != rows.end(); ++rpos)
      {
        Row r = (*rpos->get());

        std::list<boost::shared_ptr<Glyph>>::iterator gpos = r.begin();
        for(; gpos != r.end(); ++gpos)
        {
          Glyph g1 = (*gpos->get());
          if(gpos == r.end()) break;

          gpos++;
          Glyph g2 = (*gpos->get());
          gpos--;
          if(g1.font != g2.font || !g1.style.font.kerning || !g2.style.font.kerning)
            continue;

          if(int size = g1.font->GetKernAmount(g1.c, g2.c))
          {
            g2.path.MovePoints(Com::SmartPoint(size, 0));
            g2.width += size;
          }
        }
      }        
    }

    // wrap rows

    if(s->m_wrap == _T("normal") || s->m_wrap == _T("even"))
    {
      int maxwidth = abs((int)(vertical ? frame.Height() : frame.Width()));
      int minwidth = 0;

      for(std::list<boost::shared_ptr<Row>>::iterator rpos = rows.begin(); rpos != rows.end(); ++rpos)
      {
        Row r = (*rpos->get());
        
        std::list<boost::shared_ptr<Glyph>>::iterator brpos = r.end();

        if(s->m_wrap == _T("even"))
        {
          int fullwidth = 0;

          for(std::list<boost::shared_ptr<Glyph>>::iterator gpos = r.begin(); gpos != r.end(); ++gpos)
          {
            Glyph g = (*gpos->get());
            fullwidth += g.width + g.spacing;
          }

          fullwidth = abs(fullwidth);
          
          if(fullwidth > maxwidth)
          {
            maxwidth = fullwidth / ((fullwidth / maxwidth) + 1);
            minwidth = maxwidth;
          }
        }

        int width = 0;

        for(std::list<boost::shared_ptr<Glyph>>::iterator gpos = r.begin(); gpos != r.end(); ++gpos)
        {
          Glyph g = (*gpos->get());

          width += g.width + g.spacing;

          if((brpos != r.end()) && abs(width) > maxwidth && g.c != Text::SP)
          {
            row.reset(DNew Row());
            std::list<boost::shared_ptr<Glyph>>::iterator next = brpos;
            next++;
            do {row->push_front(*(brpos--));} while(brpos != r.begin());
            rows.insert(rpos, row); // TODO: Insert before
            while(!r.empty() && r.begin() != next) r.pop_front();
            gpos = next;
            g = (*gpos->get());
            width = g.width + g.spacing;
          }

          if(abs(width) >= minwidth)
          {
            if(g.style.linebreak == _T("char")
            || g.style.linebreak == _T("word") && g.c == Text::SP)
            {
              brpos = gpos;
            }
          }
        }
      }
    }

    // trim rows

    for(std::list<boost::shared_ptr<Row>>::iterator pos = rows.begin(); pos != rows.end(); ++pos)
    {
      Row r = (*pos->get());

      while(!r.empty() && r.front()->c == Text::SP)
        r.pop_front();

      while(!r.empty() && r.back()->c == Text::SP)
        r.pop_back();
    }

    // calc fill width for each glyph

    std::list<Glyph*> glypsh2fill;
    int fill_id = 0;
    int fill_width = 0;

    for(std::list<boost::shared_ptr<Row>>::iterator pos = rows.begin(); pos != rows.end(); ++pos)
    {
      Row r = (*pos->get());

      std::list<boost::shared_ptr<Glyph>>::iterator gpos = r.begin();
      while(gpos != r.end())
      {
        Glyph g = (*gpos->get()); gpos++;

        if(!glypsh2fill.empty() && fill_id && (g.style.fill.id != fill_id || (pos == rows.end()) && (gpos == r.end())))
        {
          int w = (int)(g.style.fill.width * fill_width + 0.5);

          while(!glypsh2fill.empty())
          {
            Glyph* g = glypsh2fill.back();
            fill_width -= g->width;
            g->fill = w - fill_width;
          }

          ASSERT(glypsh2fill.empty());
          ASSERT(fill_width == 0);

          glypsh2fill.clear();
          fill_width = 0;
        }

        fill_id = g.style.fill.id;

        if(g.style.fill.id)
        {
          glypsh2fill.push_back(&g);
          fill_width += g.width;
        }
      }
    }

    // calc row sizes and total subtitle size

    Com::SmartSize size(0, 0);

    if(s->m_direction.secondary == _T("left") || s->m_direction.secondary == _T("up"))
      rows.reverse();

    for(std::list<boost::shared_ptr<Row>>::iterator pos = rows.begin(); pos != rows.end(); ++pos)
    {
      Row r = (*pos->get());

      if(s->m_direction.primary == _T("left") || s->m_direction.primary == _T("up"))
        r.reverse();

      int w = 0, h = 0;

      r.width = 0;

      for(std::list<boost::shared_ptr<Glyph>>::iterator gpos = r.begin(); gpos != r.end(); ++gpos)
      {
        Glyph g = (*gpos->get());

        w += g.width;
        if(gpos != r.end()) w += g.spacing;
        h = max(h, g.ascent + g.descent);

        r.width += g.width;
        if(gpos != r.end()) r.width += g.spacing;
        r.ascent = max(r.ascent, g.ascent);
        r.descent = max(r.descent, g.descent);
        r.border = max(r.border, g.GetBackgroundSize());
      }

      for(std::list<boost::shared_ptr<Glyph>>::iterator gpos = r.begin(); gpos != r.end(); ++gpos)
      {
        Glyph g = (*gpos->get());
        g.row_ascent = r.ascent;
        g.row_descent = r.descent;
      }

      if(vertical)
      {
        size.cx += h;
        size.cy = max(size.cy, w);
      }
      else
      {
        size.cx = max(size.cx, w);
        size.cy += h;
      }
    }

    // align rows and calc glyph positions

    rs = DNew RenderedSubtitle(spdrc, clip);

    Com::SmartPoint p = GetAlignPoint(style.placement, scale, frame, size);
    Com::SmartPoint org = GetAlignPoint(style.placement, scale, frame);

    // collision detection

    if(!s->m_animated)
    {
      int tlb = !rows.empty() ? rows.front()->border : 0;
      int brb = !rows.empty() ? rows.back()->border : 0;

      Com::SmartRect r(p, size);
      m_sra.GetRect(r, s, style.placement.align, tlb, brb);
      org += r.TopLeft() - p;
      p = r.TopLeft();
    }

    Com::SmartRect subrect(p, size);

    // continue positioning

    for(std::list<boost::shared_ptr<Row>>::iterator pos = rows.begin(); pos != rows.end(); ++pos)
    {
      Row* r = (pos->get());

      Com::SmartSize rsize;
      rsize.cx = rsize.cy = r->width;

      if(vertical)
      {
        p.y = GetAlignPoint(style.placement, scale, frame, rsize).y;

        for(std::list<boost::shared_ptr<Glyph>>::iterator gpos = r->begin(); gpos != r->end(); ++gpos)
        {
          boost::shared_ptr<Glyph> g = *gpos;
          g->tl.x = p.x + (int)(g->style.placement.offset.x * scale.cx + 0.5) + r->ascent - g->ascent;
          g->tl.y = p.y + (int)(g->style.placement.offset.y * scale.cy + 0.5);
          p.y += g->width + g->spacing;
          rs->m_glyphs.push_back(g);
        }

        p.x += r->ascent + r->descent;
      }
      else
      {
        p.x = GetAlignPoint(style.placement, scale, frame, rsize).x;

        for(std::list<boost::shared_ptr<Glyph>>::iterator gpos = r->begin(); gpos != r->end(); ++gpos)
        {
          boost::shared_ptr<Glyph> g = *gpos;
          g->tl.x = p.x + (int)(g->style.placement.offset.x * scale.cx + 0.5);
          g->tl.y = p.y + (int)(g->style.placement.offset.y * scale.cy + 0.5) + r->ascent - g->ascent;
          p.x += g->width + g->spacing;
          rs->m_glyphs.push_back(g);
        }

        p.y += r->ascent + r->descent;
      }
    }

    // bkg, precalc style.placement.path, transform

    pos = rs->m_glyphs.begin();
    while(pos != rs->m_glyphs.end())
    {
      Glyph* g = pos->get(); pos++;
      g->CreateBkg();
      g->CreateSplineCoeffs(spdrc);
      g->Transform(org, subrect);
    }

    // merge glyphs (TODO: merge 'fill' too)

    Glyph* g0 = NULL;

    pos = rs->m_glyphs.begin();
    while(pos != rs->m_glyphs.end())
    {
      std::list<boost::shared_ptr<Glyph>>::iterator cur = pos;

      Glyph* g = pos->get(); pos++;

      Com::SmartRect r = g->bbox + g->tl;

      int size = (int)(g->GetBackgroundSize() + 0.5);
      int depth = (int)(g->GetShadowDepth() + 0.5);

      r.InflateRect(size, size);
      r.InflateRect(depth, depth);

      r.left >>= 6;
      r.top >>= 6;
      r.right = (r.right + 32) >> 6;
      r.bottom = (r.bottom + 32) >> 6;

      if((r & clip).IsRectEmpty()) // clip
      {
        rs->m_glyphs.erase(cur);
      }
      else if(g0 && g0->style.IsSimilar(g->style)) // append
      {
        Com::SmartPoint o = g->tl - g0->tl;

        g->path.MovePoints(o);

        g0->path.types.insert(g0->path.types.end(), g->path.types.begin(), g->path.types.end());
        g0->path.points.insert(g0->path.points.end(), g->path.points.begin(), g->path.points.end());

        g->path_bkg.MovePoints(o);

        g0->path_bkg.types.insert(g0->path_bkg.types.end(), g->path_bkg.types.begin(), g->path_bkg.types.end());
        g0->path_bkg.points.insert(g0->path_bkg.points.end(), g->path_bkg.points.begin(), g->path_bkg.points.end());

        g0->bbox |= g->bbox + o;

        rs->m_glyphs.erase(cur);
      }
      else // leave alone
      {
        g0 = g;
      }
    }

    // rasterize

    pos = rs->m_glyphs.begin();
    while(pos != rs->m_glyphs.end()) { pos->get()->Rasterize(); pos++; }

    // cache

    m_rsc.Add(s->m_name, rs);

    m_fc.Flush();

    return rs;
  }

  //

  Com::SmartRect RenderedSubtitle::Draw(SubPicDesc& spd) const
  {
    Com::SmartRect bbox;
    bbox.SetRectEmpty();

    // shadow

    std::list<boost::shared_ptr<Glyph>>::const_iterator pos = m_glyphs.begin();
    while(pos != m_glyphs.end())
    {
      Glyph g = (*pos->get()); ++pos;

      if(g.style.shadow.depth <= 0) continue;

      DWORD c = g.style.shadow.color;
      DWORD sw[6] = {c, -1};

      bool outline = g.style.background.type == L"outline" && g.style.background.size > 0;

      bbox |= g.ras_shadow.Draw(spd, m_clip, g.tls.x, g.tls.y, sw, outline ? 1 : 0);
    }

    // background

    pos = m_glyphs.begin();
    while(pos != m_glyphs.end())
    {
      Glyph g = (*pos->get()); ++pos;

      DWORD c = g.style.background.color;
      DWORD sw[6] = {c, -1};

      if(g.style.background.type == L"outline" && g.style.background.size > 0)
      {
        bbox |= g.ras.Draw(spd, m_clip, g.tl.x, g.tl.y, sw, g.style.font.color.a < 255 ? 2 : 1);
      }
      else if(g.style.background.type == L"enlarge" && g.style.background.size > 0
      || g.style.background.type == L"box" && g.style.background.size >= 0)
      {
        bbox |= g.ras_bkg.Draw(spd, m_clip, g.tl.x, g.tl.y, sw, 0);
      }
    }

    // body

    pos = m_glyphs.begin();
    while(pos != m_glyphs.end())
    {
      Glyph g = (*pos->get());

      DWORD c = g.style.font.color;
      DWORD sw[6] = {c, -1}; // TODO: fill

      bbox |= g.ras.Draw(spd, m_clip, g.tl.x, g.tl.y, sw, 0);
    }

    return bbox;
  }

  //

  void SubRectAllocator::UpdateTarget(const Com::SmartSize& s, const Com::SmartRect& r)
  {
    if(vs != s || vr != r) clear();
    vs = s;
    vr = r;
  }
  
  void SubRectAllocator::GetRect(Com::SmartRect& rect, const Subtitle* s, const Align& align, int tlb, int brb)
  {
    SubRect sr(rect, s->m_layer);
    sr.rect.InflateRect(tlb, tlb, brb, brb);

    StringMapW<SubRect>::iterator pPair = find(s->m_name);

    if((pPair != end()) && pPair->second.rect != sr.rect)
    {
      erase(pPair);
      pPair = end();
    }

    if(pPair == end())
    {
      bool vertical = s->m_direction.primary == _T("down") || s->m_direction.primary == _T("up");

      bool fOK = false;

      while(!fOK)
      {
        fOK = true;

        StringMapW<SubRect>::iterator pos = begin();
        while(pos != end())
        {
          SubRect sr2 = (pos->second); ++pos;

          if(sr.layer == sr2.layer && !(sr.rect & sr2.rect).IsRectEmpty())
          {
            if(vertical)
            {
              if(align.h < 0.5)
              {
                sr.rect.right = sr2.rect.right + sr.rect.Width();
                sr.rect.left = sr2.rect.right;
              }
              else
              {
                sr.rect.left = sr2.rect.left - sr.rect.Width();
                sr.rect.right = sr2.rect.left;
              }
            }
            else
            {
              if(align.v < 0.5)
              {
                sr.rect.bottom = sr2.rect.bottom + sr.rect.Height();
                sr.rect.top = sr2.rect.bottom;
              }
              else
              {
                sr.rect.top = sr2.rect.top - sr.rect.Height();
                sr.rect.bottom = sr2.rect.top;
              }
            }

            fOK = false;
          }
        }
      }

      (*this)[s->m_name] = sr;

      rect = sr.rect;
      rect.DeflateRect(tlb, tlb, brb, brb);
    }
  }

  //

  FontWrapper* FontCache::Create(HDC hDC, const LOGFONT& lf)
  {
    CStdStringW key;

    key.Format(L"%s,%d,%d,%d", 
      CStdStringW(lf.lfFaceName), lf.lfHeight, lf.lfWeight, 
      ((lf.lfItalic&1)<<2) | ((lf.lfUnderline&1)<<1) | ((lf.lfStrikeOut&1)<<0));

    FontWrapper* pFW = NULL;

    StringMapW<FontWrapper *>::iterator it = m_key2obj.find(key);
    if(it != m_key2obj.end())
    {
      return it->second;
    }

    HFONT hFont;

    if(!(hFont = CreateFontIndirect(&lf)))
    {
      ASSERT(0);
      return NULL;
    }

    pFW = DNew FontWrapper(hDC, hFont, key);

    Add(key, pFW, false);

    return pFW;
  }

  //

  GlyphPath* GlyphPathCache::Create(HDC hDC, const FontWrapper* f, WCHAR c)
  {
    CStdStringW key = CStdStringW((LPCWSTR)*f) + c;

    GlyphPath* path = NULL;

    StringMapW<GlyphPath *>::iterator it = m_key2obj.find(key);
    if(it != m_key2obj.end())
    {
      return it->second;
    }

    BeginPath(hDC);
    TextOutW(hDC, 0, 0, &c, 1);
    CloseFigure(hDC);
    if(!EndPath(hDC)) {AbortPath(hDC); ASSERT(0); return NULL;}

    path = DNew GlyphPath();

    int count = GetPath(hDC, NULL, NULL, 0);

    if(count > 0)
    {
      path->points.resize(count);
      path->types.resize(count);

      if(count != GetPath(hDC, &path->points[0], &path->types[0], count))
      {
        ASSERT(0);
        delete path;
        return NULL;
      }
    }

    Add(key, path);

    return path;
  }
}