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

#include "SubtitleFile.h"
#include "Array.h"
#include "GlyphPath.h"
#include "..\..\SubPic\ISubPic.h"

namespace ssf
{
  class Rasterizer
  {
    bool fFirstSet;
    Com::SmartPoint firstp, lastp;

  private:
    int mWidth, mHeight;

    union Span
    {
      struct {int x1, y1, x2, y2;};
      struct {unsigned __int64 first, second;};
      union Span() {}
      union Span(int _x1, int _y1, int _x2, int _y2) {x1 = _x1; y1 = _y1; x2 = _x2; y2 = _y2;}
      union Span(unsigned __int64 _first, unsigned __int64 _second) {first = _first; second = _second;}
    };

    Array<Span> mOutline, mWideOutline, mWideOutlineTmp;
    int mWideBorder;

    struct Edge {int next, posandflag;}* mpEdgeBuffer;
    unsigned int mEdgeHeapSize;
    unsigned int mEdgeNext;
    unsigned int* mpScanBuffer;

  protected:
    BYTE* mpOverlayBuffer;
    int mOverlayWidth, mOverlayHeight;
    int mPathOffsetX, mPathOffsetY;
    int mOffsetX, mOffsetY;

  private:
    void _TrashOverlay();
    void _ReallocEdgeBuffer(int edges);
    void _EvaluateBezier(const Com::SmartPoint& p0, const Com::SmartPoint& p1, const Com::SmartPoint& p2, const Com::SmartPoint& p3);
    void _EvaluateLine(Com::SmartPoint p0, Com::SmartPoint p1);
    void _OverlapRegion(Array<Span>& dst, Array<Span>& src, int dx, int dy);

  public:
    Rasterizer();
    virtual ~Rasterizer();

    bool ScanConvert(GlyphPath& path, const Com::SmartRect& bbox);
    bool CreateWidenedRegion(int r);
    bool Rasterize(int xsub, int ysub);
    void Reuse(Rasterizer& r);

    void Blur(float n, int plane);
    Com::SmartRect Draw(const SubPicDesc& spd, const Com::SmartRect& clip, int xsub, int ysub, const DWORD* switchpts, int plane);
  };
}