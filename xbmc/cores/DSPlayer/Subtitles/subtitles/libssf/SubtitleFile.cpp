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
#include "SubtitleFile.h"

namespace ssf
{
  SubtitleFile::SubtitleFile()
  {
  }

  SubtitleFile::~SubtitleFile()
  {
  }

  void SubtitleFile::Parse(InputStream& s)
  {
    m_segments.clear();

    __super::Parse(s, s_predef);

    // TODO: check file.format == "ssf" and file.version == 1

    std::list<Definition*> defs;
    GetRootRef()->GetChildDefs(defs, L"subtitle");
    
    StringMapW<float> offset;

    std::list<Definition*>::iterator it = defs.begin();
    for(; it != defs.end(); ++it)
    {
      Definition* pDef = *it;

      try
      {
        Definition::Time time;

        if(pDef->GetAsTime(time, offset) && (*pDef)[L"@"].IsValue())
        {
          m_segments.Insert(time.start.value, time.stop.value, pDef);
        }
      }
      catch(Exception&)
      {
      }
    }
  }

  void SubtitleFile::Append(InputStream& s, float start, float stop, bool fSetTime)
  {
    Reference* pRootRef = GetRootRef();

    ParseDefs(s, pRootRef);

    std::list<Definition*> defs;
    GetNewDefs(defs);

    std::list<Definition*>::iterator it = defs.begin();
    for(; it != defs.end(); ++it)
    {
      Definition* pDef = *it;

      if(pDef->m_parent == pRootRef && pDef->m_type == L"subtitle" && (*pDef)[L"@"].IsValue())
      {
        m_segments.Insert(start, stop, pDef);

        if(fSetTime) 
        {
          try
          {
            Definition::Time time;
            StringMapW<float> offset;
            pDef->GetAsTime(time, offset);
            if(time.start.value == start && time.stop.value == stop)
              continue;
          }
          catch(Exception&)
          {
          }

          CStdStringW str;
          str.Format(L"%.3f", start);
          pDef->SetChildAsNumber(L"time.start", str, L"s");
          str.Format(L"%.3f", stop);
          pDef->SetChildAsNumber(L"time.stop", str, L"s");
        }
      }
    }

    Commit();
  }

  bool SubtitleFile::Lookup(float at, std::list<boost::shared_ptr<Subtitle>>& subs)
  {
    if(!subs.empty()) {ASSERT(0); return false;}

    std::list<SegmentItem> sis;
    m_segments.Lookup(at, sis);

    std::list<SegmentItem>::iterator it = sis.begin();
    for(; it != sis.end(); ++it)
    {
      SegmentItem si = *it;

      boost::shared_ptr<Subtitle> s(DNew Subtitle(this));

      if(s->Parse(si.pDef, si.start, si.stop, at))
      {
        for(std::list<boost::shared_ptr<Subtitle>>::iterator it = subs.begin();
          it != subs.end(); ++it)
        {
          if(s->m_layer < it->get()->m_layer)
          {
            subs.insert(it, s); // TODO: Item must be inserted before it
            break;
          }
        }

        if(s)
        {
          subs.push_back(s);
        }
      }
    }

    return !subs.empty();
  }

  //

  SubtitleFile::Segment::Segment(float start, float stop, const SegmentItem* si)
  {
    m_start = start;
    m_stop = stop;
    if(si) push_back(*si);
  }

  SubtitleFile::Segment::Segment(const Segment& s)
  {
    *this = s;
  }

  void SubtitleFile::Segment::operator = (const Segment& s)
  {
    m_start = s.m_start; 
    m_stop = s.m_stop; 
    clear(); 
    insert(end(), s.begin(), s.end());
  }

  //

  void SubtitleFile::SegmentList::clear()
  {
    __super::clear();
    m_index.clear();
  }

  void SubtitleFile::SegmentList::Insert(float start, float stop, Definition* pDef)
  {
    if(start >= stop) {ASSERT(0); return;}

    m_index.clear();

    SegmentItem si = {pDef, start, stop};

    if(empty())
    {
      push_back(Segment(start, stop, &si));
      return;
    }
    
    Segment& head = front();
    Segment& tail = back();
    
    if(start >= tail.m_stop && stop > tail.m_stop)
    {
      if(start > tail.m_stop) push_back(Segment(tail.m_stop, start));
      push_back(Segment(start, stop, &si));
    }
    else if(start < head.m_start && stop <= head.m_start)
    {
      if(stop < head.m_start) push_front(Segment(stop, head.m_start));
      push_front(Segment(start, stop, &si));
    }
    else 
    {
      if(start < head.m_start)
      {
        push_front(Segment(start, head.m_start, &si));
        start = head.m_start;
      }

      if(stop > tail.m_stop)
      {
        push_back(Segment(tail.m_stop, stop, &si));
        stop = tail.m_stop;
      }

      for(std::list<Segment>::iterator it = begin();
        it != end(); ++it)
      {
        Segment& s = *it;

        if(start >= s.m_stop) continue;
        if(stop <= s.m_start) break;

        if(s.m_start < start && start < s.m_stop)
        {
          Segment s2 = s;
          s2.m_start = start;
          insert(it, s2); //TODO: Item must be inserted after it
          s.m_stop = start;
        }
        else if(s.m_start == start)
        {
          if(stop > s.m_stop)
          {
            start = s.m_stop;
          }
          else if(stop < s.m_stop)
          {
            Segment s2 = s;
            s2.m_start = stop;
            insert(it, s2); //TODO: Item must be inserted after it
            s.m_stop = stop;
          }

          s.push_back(si);
        }
      }
    }
  }

  size_t SubtitleFile::SegmentList::Index(bool fForce)
  {
    if(m_index.empty() || fForce)
    {
      m_index.clear();
      std::list<Segment>::iterator it = begin();
      for(; it != end(); ++it) m_index.push_back(&(*it));
    }

    return m_index.size();
  }

  void SubtitleFile::SegmentList::Lookup(float at, std::list<SegmentItem>& sis)
  {
    sis.clear();

    size_t k;
    if(Lookup(at, k)) 
    {
      const Segment *s = GetSegment(k);
      sis.insert(sis.end(), s->begin(), s->end());
    }
  }

  bool SubtitleFile::SegmentList::Lookup(float at, size_t& k)
  {
    if(!Index()) return false;

    size_t i = 0, j = m_index.size()-1;

    if(m_index[i]->m_start <= at && at < m_index[j]->m_stop)
    do
    {
      k = (i+j)/2;
      if(m_index[k]->m_start <= at && at < m_index[k]->m_stop) {return true;}
      else if(at < m_index[k]->m_start) {if(j == k) k--; j = k;}
      else if(at >= m_index[k]->m_stop) {if(i == k) k++; i = k;}
    }
    while(i <= j);

    return false;
  }

  const SubtitleFile::Segment* SubtitleFile::SegmentList::GetSegment(size_t k)
  {
    return 0 <= k && k < m_index.size() ? m_index[k] : NULL;
  }

  // TODO: this should be overridable from outside

  LPCWSTR SubtitleFile::s_predef = 
    L"color#white {a: 255; r: 255; g: 255; b: 255;}; \n"
    L"color#black {a: 255; r: 0; g: 0; b: 0;}; \n"
    L"color#gray {a: 255; r: 128; g: 128; b: 128;}; \n" 
    L"color#red {a: 255; r: 255; g: 0; b: 0;}; \n"
    L"color#green {a: 255; r: 0; g: 255; b: 0;}; \n"
    L"color#blue {a: 255; r: 0; g: 0; b: 255;}; \n"
    L"color#cyan {a: 255; r: 0; g: 255; b: 255;}; \n"
    L"color#yellow {a: 255; r: 255; g: 255; b: 0;}; \n"
    L"color#magenta {a: 255; r: 255; g: 0; b: 255;}; \n"
    L" \n"
    L"align#topleft {v: \"top\"; h: \"left\";}; \n"
    L"align#topcenter {v: \"top\"; h: \"center\";}; \n"
    L"align#topright {v: \"top\"; h: \"right\";}; \n"
    L"align#middleleft {v: \"middle\"; h: \"left\";}; \n"
    L"align#middlecenter {v: \"middle\"; h: \"center\";}; \n"
    L"align#middleright {v: \"middle\"; h: \"right\";}; \n"
    L"align#bottomleft {v: \"bottom\"; h: \"left\";}; \n"
    L"align#bottomcenter {v: \"bottom\"; h: \"center\";}; \n"
    L"align#bottomright {v: \"bottom\"; h: \"right\";}; \n"
    L" \n"
    L"time#time {scale: 1;}; \n"
    L"time#startstop {start: \"start\"; stop: \"stop\";}; \n"
    L" \n"
    L"#b {font.weight: \"bold\"}; \n"
    L"#i {font.italic: \"true\"}; \n"
    L"#u {font.underline: \"true\"}; \n"
    L"#s {font.strikethrough: \"true\"}; \n"
    L" \n"
    L"#nobr {linebreak: \"none\"}; \n"
    L" \n"
    L"subtitle#subtitle \n"
    L"{ \n"
    L"  frame \n"
    L"  { \n"
    L"    reference: \"video\"; \n"
    L"    resolution: {cx: 640; cy: 480;}; \n"
    L"  }; \n"
    L" \n"
    L"  direction \n"
    L"  { \n"
    L"    primary: \"right\"; \n"
    L"    secondary: \"down\"; \n"
    L"  }; \n"
    L" \n"
    L"  wrap: \"normal\"; \n"
    L" \n"
    L"  layer: 0; \n"
    L" \n"
    L"  style \n"
    L"  { \n"
    L"    linebreak: \"word\"; \n"
    L" \n"    
    L"    placement \n"
    L"    { \n"
    L"      clip: \"none\"; \n"
    L"      margin: {t: 0; r: 0; b: 0; l: 0;}; \n"
    L"      align: bottomcenter; \n"
    L"      pos: \"auto\" \n"
    L"      offset: {x: 0; y: 0;}; \n"
    L"      angle: {x: 0; y: 0; z: 0;}; \n"
    L"      org: \"auto\" \n"
    L"      path: \"\"; \n"
    L"    }; \n"
    L" \n"
    L"    font \n"
    L"    { \n"
    L"      face: \"Arial\"; \n"
    L"      size: 20; \n"
    L"      weight: \"bold\"; \n"
    L"      color: white; \n"
    L"      underline: \"false\"; \n"
    L"      strikethrough: \"false\"; \n"
    L"      italic: \"false\"; \n"
    L"      spacing: 0; \n"
    L"      scale: {cx: 1; cy: 1;}; \n"
    L"      kerning: \"true\"; \n"
    L"    }; \n"
    L" \n"
    L"    background \n"
    L"    { \n"
    L"      color: black; \n"
    L"      size: 2; \n"
    L"      type: \"outline\"; \n"
    L"      blur: 0; \n"
    L"    }; \n"
    L" \n"
    L"    shadow \n"
    L"    { \n"
    L"      color: black {a: 128;}; \n"
    L"      depth: 2; \n"
    L"      angle: -45; \n"
    L"      blur: 0; \n"
    L"    }; \n"
    L" \n"
    L"    fill \n"
    L"    { \n"
    L"      color: yellow; \n"
    L"      width: 0; \n"
    L"    }; \n"
    L"  }; \n"
    L"}; \n";
}
