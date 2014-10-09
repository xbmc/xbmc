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

#include "stdafx.h"
#include "Subtitle.h"
#include "Split.h"
#include <math.h>
#include "utils/StdString.h"


namespace ssf
{
  struct Subtitle::n2n_t Subtitle::m_n2n;

  Subtitle::Subtitle(File* pFile) 
    : m_pFile(pFile)
    , m_animated(false)
  {
    if(m_n2n.align[0].empty())
    {
      m_n2n.align[0][L"left"] = 0;
      m_n2n.align[0][L"center"] = 0.5;
      m_n2n.align[0][L"middle"] = 0.5;
      m_n2n.align[0][L"right"] = 1;
    }
    
    if(m_n2n.align[1].empty())
    {
      m_n2n.align[1][L"top"] = 0;
      m_n2n.align[1][L"middle"] = 0.5;
      m_n2n.align[1][L"center"] = 0.5;
      m_n2n.align[1][L"bottom"] = 1;
    }
    
    if(m_n2n.weight.empty())
    {
      m_n2n.weight[L"thin"] = FW_THIN;
      m_n2n.weight[L"normal"] = FW_NORMAL;
      m_n2n.weight[L"bold"] = FW_BOLD;
    }

    if(m_n2n.transition.empty())
    {
      m_n2n.transition[L"start"] = 0;
      m_n2n.transition[L"stop"] = 1e10;
      m_n2n.transition[L"linear"] = 1;
    }
  }

  Subtitle::~Subtitle() 
  {
  }

  bool Subtitle::Parse(Definition* pDef, float start, float stop, float at)
  {
    ASSERT(m_pFile && pDef);

    m_name = pDef->m_name;

    m_text.clear();

    m_time.start = start;
    m_time.stop = stop;

    at -= start;

    Fill::gen_id = 0;

    m_pFile->Commit();

    try
    {
      Definition& frame = (*pDef)[L"frame"];

      m_frame.reference = frame[L"reference"];
      m_frame.resolution.cx = frame[L"resolution"][L"cx"];
      m_frame.resolution.cy = frame[L"resolution"][L"cy"];

      Definition& direction = (*pDef)[L"direction"];

      m_direction.primary = direction[L"primary"];
      m_direction.secondary = direction[L"secondary"];

      m_wrap = (*pDef)[L"wrap"];

      m_layer = (*pDef)[L"layer"];

      Style style;
      GetStyle(&(*pDef)[L"style"], style);

      StringMapW<float> offset;
      Definition& block = (*pDef)[L"@"];
      Parse(WCharInputStream((CStdStringW)block), style, at, offset, dynamic_cast<Reference*>(block.m_parent));

      // TODO: trimming should be done by the renderer later, after breaking the words into lines

      while(!m_text.empty() && (m_text.front().str[0] == Text::SP || m_text.front().str[0] == Text::LSEP))
        m_text.pop_front();

      while(!m_text.empty() && (m_text.back().str[0] == Text::SP || m_text.back().str[0] == Text::LSEP))
        m_text.pop_back();

      for(std::list<Text>::iterator it = m_text.begin(); 
        it != m_text.end(); ++it)
      {
        if((*it).str[0] == Text::LSEP)
        {
          std::list<Text>::iterator prev = it;
          --prev;

          while(prev != m_text.begin() && (*prev).str[0] == Text::SP)
          {
            std::list<Text>::iterator tmp = prev;
            --prev;
            m_text.erase(tmp);
          }

          std::list<Text>::iterator next = it;
          ++next;

          while(next != m_text.end() && (*next).str[0] == Text::SP)
          {
            std::list<Text>::iterator tmp = next;
            ++next;
            m_text.erase(tmp);
          }
        }
      }
    }
    catch(Exception& e)
    {
      UNREFERENCED_PARAMETER(e);
      return false;
    }

    m_pFile->Rollback();

    return true;
  }

  void Subtitle::GetStyle(Definition* pDef, Style& style)
  {
    style.placement.pos.x = 0;
    style.placement.pos.y = 0;
    style.placement.pos.auto_x = true;
    style.placement.pos.auto_y = true;

    style.placement.org.x = 0;
    style.placement.org.y = 0;
    style.placement.org.auto_x = true;
    style.placement.org.auto_y = true;

    Rect frame = {0, m_frame.resolution.cx, m_frame.resolution.cy, 0};

    style.placement.clip.t = -1;
    style.placement.clip.r = -1;
    style.placement.clip.b = -1;
    style.placement.clip.l = -1;

    //

    style.linebreak = (*pDef)[L"linebreak"];

    Definition& placement = (*pDef)[L"placement"];

    Definition& clip = placement[L"clip"];

    if(clip.IsValue(Definition::string))
    {
      CStdStringW str = clip;

      if(str == L"frame") style.placement.clip = frame;
      // else ?
    }
    else
    {
      if(clip[L"t"].IsValue()) style.placement.clip.t = clip[L"t"];
      if(clip[L"r"].IsValue()) style.placement.clip.r = clip[L"r"];
      if(clip[L"b"].IsValue()) style.placement.clip.b = clip[L"b"];
      if(clip[L"l"].IsValue()) style.placement.clip.l = clip[L"l"];
    }

    StringMapW<float> n2n_margin[2];

    n2n_margin[0][L"top"] = 0;
    n2n_margin[0][L"right"] = 0;
    n2n_margin[0][L"bottom"] = frame.b - frame.t;
    n2n_margin[0][L"left"] = frame.r - frame.l;
    n2n_margin[1][L"top"] = frame.b - frame.t;
    n2n_margin[1][L"right"] = frame.r - frame.l;
    n2n_margin[1][L"bottom"] = 0;
    n2n_margin[1][L"left"] = 0;

    placement[L"margin"][L"t"].GetAsNumber(style.placement.margin.t, &n2n_margin[0]);
    placement[L"margin"][L"r"].GetAsNumber(style.placement.margin.r, &n2n_margin[0]);
    placement[L"margin"][L"b"].GetAsNumber(style.placement.margin.b, &n2n_margin[1]);
    placement[L"margin"][L"l"].GetAsNumber(style.placement.margin.l, &n2n_margin[1]);

    placement[L"align"][L"h"].GetAsNumber(style.placement.align.h, &m_n2n.align[0]);
    placement[L"align"][L"v"].GetAsNumber(style.placement.align.v, &m_n2n.align[1]);

    if(placement[L"pos"][L"x"].IsValue()) {style.placement.pos.x = placement[L"pos"][L"x"]; style.placement.pos.auto_x = false;}
    if(placement[L"pos"][L"y"].IsValue()) {style.placement.pos.y = placement[L"pos"][L"y"]; style.placement.pos.auto_y = false;}

    placement[L"offset"][L"x"].GetAsNumber(style.placement.offset.x);
    placement[L"offset"][L"y"].GetAsNumber(style.placement.offset.y);

    style.placement.angle.x = placement[L"angle"][L"x"];
    style.placement.angle.y = placement[L"angle"][L"y"];
    style.placement.angle.z = placement[L"angle"][L"z"];

    if(placement[L"org"][L"x"].IsValue()) {style.placement.org.x = placement[L"org"][L"x"]; style.placement.org.auto_x = false;}
    if(placement[L"org"][L"y"].IsValue()) {style.placement.org.y = placement[L"org"][L"y"]; style.placement.org.auto_y = false;}

    style.placement.path = placement[L"path"];

    Definition& font = (*pDef)[L"font"];

    style.font.face = font[L"face"];
    style.font.size = font[L"size"];
    font[L"weight"].GetAsNumber(style.font.weight, &m_n2n.weight);
    style.font.color.a = font[L"color"][L"a"];
    style.font.color.r = font[L"color"][L"r"];
    style.font.color.g = font[L"color"][L"g"];
    style.font.color.b = font[L"color"][L"b"];
    style.font.underline = font[L"underline"];
    style.font.strikethrough = font[L"strikethrough"];
    style.font.italic = font[L"italic"];
    style.font.spacing = font[L"spacing"];
    style.font.scale.cx = font[L"scale"][L"cx"];
    style.font.scale.cy = font[L"scale"][L"cy"];
    style.font.kerning = font[L"kerning"];

    Definition& background = (*pDef)[L"background"];

    style.background.color.a = background[L"color"][L"a"];
    style.background.color.r = background[L"color"][L"r"];
    style.background.color.g = background[L"color"][L"g"];
    style.background.color.b = background[L"color"][L"b"];
    style.background.size = background[L"size"];
    style.background.type = background[L"type"];
    style.background.blur = background[L"blur"];

    Definition& shadow = (*pDef)[L"shadow"];

    style.shadow.color.a = shadow[L"color"][L"a"];
    style.shadow.color.r = shadow[L"color"][L"r"];
    style.shadow.color.g = shadow[L"color"][L"g"];
    style.shadow.color.b = shadow[L"color"][L"b"];
    style.shadow.depth = shadow[L"depth"];
    style.shadow.angle = shadow[L"angle"];
    style.shadow.blur = shadow[L"blur"];

    Definition& fill = (*pDef)[L"fill"];

    style.fill.color.a = fill[L"color"][L"a"];
    style.fill.color.r = fill[L"color"][L"r"];
    style.fill.color.g = fill[L"color"][L"g"];
    style.fill.color.b = fill[L"color"][L"b"];
    style.fill.width = fill[L"width"];
  }

  float Subtitle::GetMixWeight(Definition* pDef, float at, StringMapW<float>& offset, int default_id)
  {
    float t = 1;

    try
    {
      StringMapW<float> n2n;

      n2n[L"start"] = 0;
      n2n[L"stop"] = m_time.stop - m_time.start;

      Definition::Time time;
      if(pDef->GetAsTime(time, offset, &n2n, default_id) && time.start.value < time.stop.value)
      {
        t = (at - time.start.value) / (time.stop.value - time.start.value);

        float u = t;

        if(t < 0) t = 0;
        else if(t >= 1) t = 0.99999f; // doh

        if((*pDef)[L"loop"].IsValue()) t *= (float)(*pDef)[L"loop"];

        CStdStringW direction = (*pDef)[L"direction"].IsValue() ? (*pDef)[L"direction"] : L"fw";
        if(direction == L"fwbw" || direction == L"bwfw") t *= 2;

        float n;
        t = modf(t, &n);

        if(direction == L"bw" 
        || direction == L"fwbw" && ((int)n & 1)
        || direction == L"bwfw" && !((int)n & 1)) 
          t = 1 - t;

        float accel = 1;

        if((*pDef)[L"transition"].IsValue())
        {
          Definition::Number<float> n;
          (*pDef)[L"transition"].GetAsNumber(n, &m_n2n.transition);
          if(n.value >= 0) accel = n.value;
        }

        if(t == 0.99999f) t = 1;

        if(u >= 0 && u < 1)
        {
          t = accel == 0 ? 1 : 
            accel == 1 ? t : 
            accel >= 1e10 ? 0 :
            pow(t, accel);
        }
      }
    }
    catch(Exception&)
    {
    }

    return t;
  }

  template<class T> 
  bool Subtitle::MixValue(Definition& def, T& value, float t)
  {
    StringMapW<T> n2n;
    return MixValue(def, value, t, &n2n);
  }

  template<> 
  bool Subtitle::MixValue(Definition& def, float& value, float t)
  {
    StringMapW<float> n2n;
    return MixValue(def, value, t, &n2n);
  }

  template<class T> 
  bool Subtitle::MixValue(Definition& def, T& value, float t, StringMapW<T>* n2n)
  {
    if(!def.IsValue()) return false;

    if(t >= 0.5)
    {
      if(n2n && def.IsValue(Definition::string))
      {
        StringMapW<T>::iterator it = n2n->find((CStdStringW) def);
        if(it != n2n->end())
        {
          value = it->second;
          return true;
        }
      }

      value = (T)def;
    }

    return true;
  }

  template<> 
  bool Subtitle::MixValue(Definition& def, float& value, float t, StringMapW<float>* n2n)
  {
    if(!def.IsValue()) return false;

    if(t > 0)
    {
      if(n2n && def.IsValue(Definition::string))
      {
        StringMap<float, CStdStringW>::iterator it = n2n->find(CStdStringW(def));
        if (it != n2n->end())
        {
          value += (it->second - value) * t;
          return true;
        }
      }

      value += ((float)def - value) * t;
    }

    return true;
  }

  template<>
  bool Subtitle::MixValue(Definition& def, Path& src, float t)
  {
    if(!def.IsValue(Definition::string)) return false;

    if(t >= 1)
    {
      src = (LPCWSTR)def;
    }
    else if(t > 0)
    {
      Path dst = (LPCWSTR)def;

      if(src.size() == dst.size())
      {
        for(size_t i = 0, j = src.size(); i < j; i++)
        {
          Point& s = src[i];
          const Point& d = dst[i];
          s.x += (d.x - s.x) * t;
          s.y += (d.y - s.y) * t;
        }
      }
    }

    return true;
  }

  void Subtitle::MixStyle(Definition* pDef, Style& dst, float t)
  {
    const Style src = dst;

    if(t <= 0) return;
    else if(t > 1) t = 1;

    MixValue((*pDef)[L"linebreak"], dst.linebreak, t);

    Definition& placement = (*pDef)[L"placement"];

    MixValue(placement[L"clip"][L"t"], dst.placement.clip.t, t);
    MixValue(placement[L"clip"][L"r"], dst.placement.clip.r, t);
    MixValue(placement[L"clip"][L"b"], dst.placement.clip.b, t);
    MixValue(placement[L"clip"][L"l"], dst.placement.clip.l, t);
    MixValue(placement[L"align"][L"h"], dst.placement.align.h, t, &m_n2n.align[0]);
    MixValue(placement[L"align"][L"v"], dst.placement.align.v, t, &m_n2n.align[1]);
    dst.placement.pos.auto_x = !MixValue(placement[L"pos"][L"x"], dst.placement.pos.x, dst.placement.pos.auto_x ? 1 : t);
    dst.placement.pos.auto_y = !MixValue(placement[L"pos"][L"y"], dst.placement.pos.y, dst.placement.pos.auto_y ? 1 : t);
    MixValue(placement[L"offset"][L"x"], dst.placement.offset.x, t);
    MixValue(placement[L"offset"][L"y"], dst.placement.offset.y, t);
    MixValue(placement[L"angle"][L"x"], dst.placement.angle.x, t);
    MixValue(placement[L"angle"][L"y"], dst.placement.angle.y, t);
    MixValue(placement[L"angle"][L"z"], dst.placement.angle.z, t);
    dst.placement.org.auto_x = !MixValue(placement[L"org"][L"x"], dst.placement.org.x, dst.placement.org.auto_x ? 1 : t);
    dst.placement.org.auto_y = !MixValue(placement[L"org"][L"y"], dst.placement.org.y, dst.placement.org.auto_y ? 1 : t);
    MixValue(placement[L"path"], dst.placement.path, t);

    Definition& font = (*pDef)[L"font"];

    MixValue(font[L"face"], dst.font.face, t);
    MixValue(font[L"size"], dst.font.size, t);
    MixValue(font[L"weight"], dst.font.weight, t, &m_n2n.weight);
    MixValue(font[L"color"][L"a"], dst.font.color.a, t);
    MixValue(font[L"color"][L"r"], dst.font.color.r, t);
    MixValue(font[L"color"][L"g"], dst.font.color.g, t);
    MixValue(font[L"color"][L"b"], dst.font.color.b, t);
    MixValue(font[L"underline"], dst.font.underline, t);
    MixValue(font[L"strikethrough"], dst.font.strikethrough, t);
    MixValue(font[L"italic"], dst.font.italic, t);
    MixValue(font[L"spacing"], dst.font.spacing, t);
    MixValue(font[L"scale"][L"cx"], dst.font.scale.cx, t);
    MixValue(font[L"scale"][L"cy"], dst.font.scale.cy, t);
    MixValue(font[L"kerning"], dst.font.kerning, t);

    Definition& background = (*pDef)[L"background"];

    MixValue(background[L"color"][L"a"], dst.background.color.a, t);
    MixValue(background[L"color"][L"r"], dst.background.color.r, t);
    MixValue(background[L"color"][L"g"], dst.background.color.g, t);
    MixValue(background[L"color"][L"b"], dst.background.color.b, t);
    MixValue(background[L"size"], dst.background.size, t);
    MixValue(background[L"type"], dst.background.type, t);
    MixValue(background[L"blur"], dst.background.blur, t);

    Definition& shadow = (*pDef)[L"shadow"];

    MixValue(shadow[L"color"][L"a"], dst.shadow.color.a, t);
    MixValue(shadow[L"color"][L"r"], dst.shadow.color.r, t);
    MixValue(shadow[L"color"][L"g"], dst.shadow.color.g, t);
    MixValue(shadow[L"color"][L"b"], dst.shadow.color.b, t);
    MixValue(shadow[L"depth"], dst.shadow.depth, t);
    MixValue(shadow[L"angle"], dst.shadow.angle, t);
    MixValue(shadow[L"blur"], dst.shadow.blur, t);

    Definition& fill = (*pDef)[L"fill"];

    MixValue(fill[L"color"][L"a"], dst.fill.color.a, t);
    MixValue(fill[L"color"][L"r"], dst.fill.color.r, t);
    MixValue(fill[L"color"][L"g"], dst.fill.color.g, t);
    MixValue(fill[L"color"][L"b"], dst.fill.color.b, t);
    MixValue(fill[L"width"], dst.fill.width, t);

    if(fill.m_priority >= PNormal) // this assumes there is no way to set low priority inline overrides
    {
      if(dst.fill.id > 0) throw Exception(_T("cannot apply fill more than once on the same text"));
      dst.fill.id = ++Fill::gen_id;
    }
  }

  void Subtitle::Parse(InputStream& s, Style style, float at, StringMapW<float> offset, Reference* pParentRef)
  {
    Text text;
    text.style = style;

    for(int c = s.PeekChar(); c != Stream::EOS; c = s.PeekChar())
    {
      s.GetChar();

      if(c == '[')
      {
        AddText(text);

        style = text.style;

        StringMapW<float> inneroffset = offset;

        int default_id = 0;

        do
        {
          Definition* pDef = m_pFile->CreateDef(pParentRef);

          m_pFile->ParseRefs(s, pDef, L",;]");

          ASSERT(pDef->IsType(L"style") || pDef->IsTypeUnknown());

          if((*pDef)[L"time"][L"start"].IsValue() && (*pDef)[L"time"][L"stop"].IsValue())
          {
            m_animated = true;
          }

          float t = GetMixWeight(pDef, at, offset, ++default_id);
          MixStyle(pDef, style, t);

          if((*pDef)[L"@"].IsValue())
          {
            Parse(WCharInputStream((LPCWSTR)(*pDef)[L"@"]), style, at, offset, pParentRef);
          }

          s.SkipWhiteSpace();
          c = s.GetChar();
        }
        while(c == ',' || c == ';');

        if(c != ']') s.ThrowError(_T("unterminated override"));

        bool fWhiteSpaceNext = s.IsWhiteSpace(s.PeekChar());
        c = s.SkipWhiteSpace();

        if(c == '{') 
        {
          s.GetChar();
          Parse(s, style, at, inneroffset, pParentRef);
        }
        else 
        {
          if(fWhiteSpaceNext) text.str += (WCHAR)Text::SP;
          text.style = style;
        }
      }
      else if(c == ']')
      {
        s.ThrowError(_T("unexpected ] found"));
      }
      else if(c == '{')
      {
        AddText(text);
        Parse(s, text.style, at, offset, pParentRef);
      }
      else if(c == '}')
      {
        break;
      }
      else
      {
        if(c == '\\')
        {
          c = s.GetChar();
          if(c == Stream::EOS) break;
		  else if (c == 'n') { AddText(text); text.str = (PCWSTR)Text::LSEP; AddText(text); continue; }
          else if(c == 'h') c = Text::NBSP;
        }

        AddChar(text, (WCHAR)c);
      }
    }

    AddText(text);
  }

  void Subtitle::AddChar(Text& t, WCHAR c)
  {
    bool f1 = !t.str.empty() && Stream::IsWhiteSpace(t.str[t.str.GetLength()-1]);
    bool f2 = Stream::IsWhiteSpace(c);
    if(f2) c = Text::SP;
    if(!f1 || !f2) t.str += (WCHAR)c;
  }

  void Subtitle::AddText(Text& t)
  {
    if(t.str.empty()) return;

    Split sa(' ', t.str, 0, Split::Max);

    for(size_t i = 0, n = sa; i < n; i++)
    {
      CStdStringW str = sa[i];

      if(!str.empty())
      {
        t.str = str;
        m_text.push_back(t);
      }

      if(i < n-1 && (m_text.empty() || m_text.back().str[0] != Text::SP))
      {
		  t.str = (PCWSTR)Text::SP;
        m_text.push_back(t);
      }
    }
    
    t.str.Empty();
  }

  //

  unsigned int Fill::gen_id = 0;

  Color::operator DWORD()
  {
    DWORD c = 
      (min(max((DWORD)b, 0), 255) <<  0) |
      (min(max((DWORD)g, 0), 255) <<  8) |
      (min(max((DWORD)r, 0), 255) << 16) |
      (min(max((DWORD)a, 0), 255) << 24);

    return c;
  }

  Path& Path::operator = (LPCWSTR str)
  {
    Split s(' ', str);

    resize(s/2);

    for(size_t i = 0, j = size(); i < j; i++)
    {
      Point p;
      p.x = s.GetAtFloat(i*2+0);
      p.y = s.GetAtFloat(i*2+1);
      (*this)[i] = p;
    }

    return *this;
  }

  CStdStringW Path::ToString()
  {
    CStdStringW ret;

    for(size_t i = 0, j = size(); i < j; i++)
    {
      const Point& p = at(i);

      CStdStringW str;
      str.Format(L"%f %f ", p.x, p.y);
      ret += str;
    }

    return ret;
  }

  bool Style::IsSimilar(const Style& s)
  {
    return 
       font.color.r == s.font.color.r
    && font.color.g == s.font.color.g
    && font.color.b == s.font.color.b
    && font.color.a == s.font.color.a
    && background.color.r == s.background.color.r
    && background.color.g == s.background.color.g
    && background.color.b == s.background.color.b
    && background.color.a == s.background.color.a
    && background.size == s.background.size
    && background.type == s.background.type
    && background.blur == s.background.blur
    && shadow.color.r == s.shadow.color.r
    && shadow.color.g == s.shadow.color.g
    && shadow.color.b == s.shadow.color.b
    && shadow.color.a == s.shadow.color.a
    && shadow.depth == s.shadow.depth
    && shadow.angle == s.shadow.angle
    && shadow.blur == s.shadow.blur
    && fill.id == s.fill.id;
  }
}
