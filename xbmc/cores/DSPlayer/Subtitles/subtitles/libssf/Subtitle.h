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

#include "File.h"
#include "utils/StdString.h"

namespace ssf
{
  struct Point {float x, y;};
  struct PointAuto : public Point {bool auto_x, auto_y;};
  struct Size {float cx, cy;};
  struct Rect {float t, r, b, l;};
  struct Align {float v, h;};
  struct Angle {float x, y, z;};
  struct Color {float a, r, g, b; operator DWORD(); void operator = (DWORD c);};
  struct Frame {CStdStringW reference; Size resolution;};
  struct Direction {CStdStringW primary, secondary;};
  struct Time {float start, stop;};
  struct Background {Color color; float size, blur; CStdStringW type;};
  struct Shadow {Color color; float depth, angle, blur;};

  class Path : public std::vector<Point>
  {
  public: 
    Path() {} 
    Path(const Path& path) {*this = path;} 
    Path& operator = (const Path& path) {assign(path.begin(), path.end()); return *this;} 
    Path(LPCWSTR str) {*this = str;} 
    Path& operator = (LPCWSTR str);
    CStdStringW ToString();
  };

  struct Placement 
  {
    Rect clip, margin; 
    Align align; 
    PointAuto pos; 
    Point offset;
    Angle angle; 
    PointAuto org;
    Path path;
  };

  struct Font
  {
    CStdStringW face;
    float size, weight;
    Color color;
    bool underline, strikethrough, italic;
    float spacing;
    Size scale;
    bool kerning;
  };

  struct Fill
  {
    static unsigned int gen_id;
    int id;
    Color color;
    float width;
    struct Fill() : id(0) {}
  };

  struct Style
  {
    CStdStringW linebreak;
    Placement placement;
    Font font;
    Background background;
    Shadow shadow;
    Fill fill;

    bool IsSimilar(const Style& s);
  };

  struct Text
  {
    enum {SP = 0x20, NBSP = 0xa0, LSEP = 0x0a};
    Style style;
    CStdStringW str;
  };

  class Subtitle
  {
    static struct n2n_t {StringMapW<float> align[2], weight, transition;} m_n2n;

    File* m_pFile;

    void GetStyle(Definition* pDef, Style& style);
    float GetMixWeight(Definition* pDef, float at, StringMapW<float>& offset, int default_id);
    template<class T> bool MixValue(Definition& def, T& value, float t);
    template<> bool MixValue(Definition& def, float& value, float t);
    template<class T> bool MixValue(Definition& def, T& value, float t, StringMapW<T>* n2n);
    template<> bool MixValue(Definition& def, float& value, float t, StringMapW<float>* n2n);
    template<> bool MixValue(Definition& def, Path& value, float t);
    void MixStyle(Definition* pDef, Style& dst, float t);

    void Parse(InputStream& s, Style style, float at, StringMapW<float> offset, Reference* pParentRef);

    void AddChar(Text& t, WCHAR c);
    void AddText(Text& t);

  public:
    CStdStringW m_name;
    bool m_animated;
    Frame m_frame;
    Direction m_direction;
    CStdStringW m_wrap;
    float m_layer;
    Time m_time;
    std::list<Text> m_text;

  public:
    Subtitle(File* pFile);
    virtual ~Subtitle();

    bool Parse(Definition* pDef, float start, float stop, float at);
  };
};