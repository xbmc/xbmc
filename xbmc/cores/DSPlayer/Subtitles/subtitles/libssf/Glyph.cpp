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
#include "Glyph.h"
#include "Split.h"

#define deg2rad(d) (float)(M_PI/180*(d))

namespace ssf
{
  Glyph::Glyph()
  {
    c = 0;
    font = NULL;
    ascent = descent = width = spacing = fill = 0;
    tl.x = tl.y = tls.x = tls.y = 0;
  }

  float Glyph::GetBackgroundSize() const
  {
    return style.background.size * (scale.cx + scale.cy) / 2;
  }

  float Glyph::GetShadowDepth() const
  {
    return style.shadow.depth * (scale.cx + scale.cy) / 2;
  }

  Com::SmartRect Glyph::GetClipRect() const
  {
    Com::SmartRect r = bbox + tl;

    int size = (int)(GetBackgroundSize() + 0.5);
    int depth = (int)(GetShadowDepth() + 0.5);

    r.InflateRect(size, size);
    r.InflateRect(depth, depth);

    r.left >>= 6;
    r.top >>= 6;
    r.right = (r.right + 32) >> 6;
    r.bottom = (r.bottom + 32) >> 6;

    return r;
  }

  void Glyph::CreateBkg()
  {
    path_bkg.clear();

    if(style.background.type == L"enlarge" && style.background.size > 0)
    {
      path_bkg.Enlarge(path, GetBackgroundSize());
    }
    else if(style.background.type == L"box" && style.background.size >= 0)
    {
      if(c != ssf::Text::LSEP)
      {
        int s = (int)(GetBackgroundSize() + 0.5);
        int x0 = (!vertical ? -spacing/2 : ascent - row_ascent);
        int y0 = (!vertical ? ascent - row_ascent : -spacing/2);
        int x1 = x0 + (!vertical ? width + spacing : row_ascent + row_descent);
        int y1 = y0 + (!vertical ? row_ascent + row_descent : width + spacing);
        path_bkg.types.resize(4);
        path_bkg.types[0] = PT_MOVETO;
        path_bkg.types[1] = PT_LINETO;
        path_bkg.types[2] = PT_LINETO;
        path_bkg.types[3] = PT_LINETO;
        path_bkg.points.resize(4);
        path_bkg.points[0] = Com::SmartPoint(x0-s, y0-s);
        path_bkg.points[1] = Com::SmartPoint(x1+s, y0-s);
        path_bkg.points[2] = Com::SmartPoint(x1+s, y1+s);
        path_bkg.points[3] = Com::SmartPoint(x0-s, y1+s);
      }
    }
  }

  void Glyph::CreateSplineCoeffs(const Com::SmartRect& spdrc)
  {
    spline.clear();

    if(style.placement.path.empty())
      return;

    size_t i = 0, j = style.placement.path.size();

    std::vector<Point> pts;
    pts.resize(j + 2);

    Point p;

    while(i < j)
    {
      p.x = style.placement.path[i].x * scale.cx + spdrc.left * 64;
      p.y = style.placement.path[i].y * scale.cy + spdrc.top * 64;
      pts[++i] = p;
    }

    if(pts.size() >= 4)
    {
      if(pts[1].x == pts[j].x && pts[1].y == pts[j].y)
      {
        pts[0] = pts[j-1];
        pts[j+1] = pts[2];
      }
      else
      {
        p.x = pts[1].x*2 - pts[2].x;
        p.y = pts[1].y*2 - pts[2].y;
        pts[0] = p;

        p.x = pts[j].x*2 - pts[j-1].x;
        p.y = pts[j].y*2 - pts[j-1].y;
        pts[j+1] = p;
      }

      spline.resize(pts.size()-3);

      for(size_t i = 0, j = pts.size()-4; i <= j; i++)
      {
        static const float _1div6 = 1.0f / 6;

        SplineCoeffs sc;

        sc.cx[3] = _1div6*(-  pts[i+0].x + 3*pts[i+1].x - 3*pts[i+2].x + pts[i+3].x);
        sc.cx[2] = _1div6*( 3*pts[i+0].x - 6*pts[i+1].x + 3*pts[i+2].x);
        sc.cx[1] = _1div6*(-3*pts[i+0].x              + 3*pts[i+2].x);
        sc.cx[0] = _1div6*(   pts[i+0].x + 4*pts[i+1].x + 1*pts[i+2].x);

        sc.cy[3] = _1div6*(-  pts[i+0].y + 3*pts[i+1].y - 3*pts[i+2].y + pts[i+3].y);
        sc.cy[2] = _1div6*( 3*pts[i+0].y - 6*pts[i+1].y + 3*pts[i+2].y);
        sc.cy[1] = _1div6*(-3*pts[i+0].y                + 3*pts[i+2].y);
        sc.cy[0] = _1div6*(   pts[i+0].y + 4*pts[i+1].y + 1*pts[i+2].y);

        spline[i] = sc;
      }
    }
  }

  void Glyph::Transform(GlyphPath& path, Com::SmartPoint org, const Com::SmartRect& subrect)
  {
    // TODO: add sse code path

    float sx = style.font.scale.cx;
    float sy = style.font.scale.cy;

    bool brotate = style.placement.angle.x || style.placement.angle.y || style.placement.angle.z;
    bool bspline = !spline.empty();
    bool bscale = brotate || bspline || sx != 1 || sy != 1;

    float caz = cos(deg2rad(style.placement.angle.z));
    float saz = sin(deg2rad(style.placement.angle.z));
    float cax = cos(deg2rad(style.placement.angle.x));
    float sax = sin(deg2rad(style.placement.angle.x));
    float cay = cos(deg2rad(style.placement.angle.y));
    float say = sin(deg2rad(style.placement.angle.y));

    for(size_t i = 0, j = path.types.size(); i < j; i++)
    {
      Com::SmartPoint p = path.points[i];

      if(bscale)
      {
        float x, y, z, xx, yy, zz;

        x = sx * (p.x - org.x);
        y = sy * (p.y - org.y);
        z = 0;

        if(bspline)
        {
          float pos = vertical ? y + org.y + tl.y - subrect.top : x + org.x + tl.x - subrect.left;
          float size = vertical ? subrect.Size().cy : subrect.Size().cx;
          float dist = vertical ? x : y;

          const SplineCoeffs* sc;
          float t;

          if(pos >= size)
          {
            sc = &spline[spline.size() - 1];
            t = 1;
          }
          else
          {
            float u = size / spline.size();
            sc = &spline[max((int)(pos / u), 0)];
            t = fmod(pos, u) / u;
          }

          float nx = sc->cx[1] + 2*t*sc->cx[2] + 3*t*t*sc->cx[3];
          float ny = sc->cy[1] + 2*t*sc->cy[2] + 3*t*t*sc->cy[3];
          float nl = 1.0f / sqrt(nx*nx + ny*ny);

          nx *= nl;
          ny *= nl;

          x = sc->cx[0] + t*(sc->cx[1] + t*(sc->cx[2] + t*sc->cx[3])) - ny * dist - org.x - tl.x;
          y = sc->cy[0] + t*(sc->cy[1] + t*(sc->cy[2] + t*sc->cy[3])) + nx * dist - org.y - tl.y;
        }

        if(brotate)
        {
          xx = x*caz + y*saz;
          yy = -(x*saz - y*caz);
          zz = z;

          x = xx;
          y = yy*cax + zz*sax;
          z = yy*sax - zz*cax;

          xx = x*cay + z*say;
          yy = y;
          zz = x*say - z*cay;

          zz = 1.0f / (max(zz, -19000) + 20000);

          x = (xx * 20000) * zz;
          y = (yy * 20000) * zz;
        }

        p.x = (int)(x + org.x + 0.5);
        p.y = (int)(y + org.y + 0.5);

        path.points[i] = p;
      }

      if(p.x < bbox.left) bbox.left = p.x;
      if(p.x > bbox.right) bbox.right = p.x;
      if(p.y < bbox.top) bbox.top = p.y;
      if(p.y > bbox.bottom) bbox.bottom = p.y;
    }
  }

  void Glyph::Transform(Com::SmartPoint org, const Com::SmartRect& subrect)
  {
    if(!style.placement.org.auto_x) org.x = style.placement.org.x * scale.cx;
    if(!style.placement.org.auto_y) org.y = style.placement.org.y * scale.cy;

    org -= tl;

    bbox.SetRect(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

    Transform(path_bkg, org, subrect);
    Transform(path, org, subrect);

    bbox |= Com::SmartRect(0, 0, 0, 0);
  }

  void Glyph::Rasterize()
  {
    if(!path_bkg.empty())
    {
      ras_bkg.ScanConvert(path_bkg, bbox);
      ras_bkg.Rasterize(tl.x, tl.y);
    }

    ras.ScanConvert(path, bbox);

    if(style.background.type == L"outline" && style.background.size > 0)
    {
      ras.CreateWidenedRegion((int)(GetBackgroundSize() + 0.5));
    }

    //

    Rasterizer* r = path_bkg.empty() ? &ras : &ras_bkg;
    int plane = path_bkg.empty() ? (style.font.color.a < 255 ? 2 : 1) : 0;
    
    ras.Rasterize(tl.x, tl.y);
    r->Blur(style.background.blur, plane);

    if(style.shadow.depth > 0)
    {
      ras_shadow.Reuse(*r);

      float depth = GetShadowDepth();

      tls.x = tl.x + (int)(depth * cos(deg2rad(style.shadow.angle)) + 0.5);
      tls.y = tl.y + (int)(depth * -sin(deg2rad(style.shadow.angle)) + 0.5);

      ras_shadow.Rasterize(tls.x, tls.y);
      ras_shadow.Blur(style.shadow.blur, plane ? 1 : 0);
    }
  }

}