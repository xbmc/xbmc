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
#include <math.h>
#include <time.h>
#include <emmintrin.h>
#include "RTS.h"

// WARNING: this isn't very thread safe, use only one RTS a time. We should use TLS in future.
static HDC g_hDC;
static int g_hDC_refcnt = 0;
#define RTSTraceLevel 0

static long revcolor(long c)
{
  return ((c&0xff0000)>>16) + (c&0xff00) + ((c&0xff)<<16);
}

//////////////////////////////////////////////////////////////////////////////////////////////

// CMyFont

CMyFont::CMyFont(STSStyle& style)
{
  hFont = NULL;
  LOGFONT lf;
  memset(&lf, 0, sizeof(lf));
  lf <<= style;
  lf.lfHeight = (LONG)(style.fontSize + 0.5);
  lf.lfOutPrecision = OUT_TT_PRECIS;
  lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lf.lfQuality = ANTIALIASED_QUALITY;
  lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
#ifdef _VSMOD // patch m007. symbol rotating
  lf.lfOrientation = (LONG)style.mod_fontOrient;
#endif

  if(! (hFont = CreateFontIndirect(&lf)))
  {
    _tcscpy(lf.lfFaceName, _T("Arial"));
    if (! (hFont = CreateFontIndirect(&lf)))
      ASSERT(0);
  }

  HFONT hOldFont = SelectFont(g_hDC, this->hFont);
  TEXTMETRIC tm;
  GetTextMetrics(g_hDC, &tm);
  m_ascent = ((tm.tmAscent + 4) >> 3);
  m_descent = ((tm.tmDescent + 4) >> 3);
  SelectFont(g_hDC, hOldFont);
}

CMyFont::~CMyFont()
{
  if (hFont)
  {
    DeleteObject(hFont);
    hFont = 0;
  }
}

// CWord

CWord::CWord(STSStyle& style, CStdStringW str, int ktype, int kstart, int kend) 
  : m_style(style), m_str(str)
  , m_width(0), m_ascent(0), m_descent(0)
  , m_ktype(ktype), m_kstart(kstart), m_kend(kend)
  , m_fDrawn(false), m_p(INT_MAX, INT_MAX)
  , m_fLineBreak(false), m_fWhiteSpaceChar(false)
  , m_pOpaqueBox(NULL)
{
  if(str.IsEmpty()) 
  {
    m_fWhiteSpaceChar = m_fLineBreak = true; 
  }

  CMyFont font(m_style);
  m_ascent = (int)(m_style.fontScaleY/100*font.m_ascent);
  m_descent = (int)(m_style.fontScaleY/100*font.m_descent);
  m_width = 0;
}

CWord::~CWord()
{
  if(m_pOpaqueBox) delete m_pOpaqueBox;
}

bool CWord::Append(CWord* w)
{
  if(!(m_style == w->m_style)
  || m_fLineBreak || w->m_fLineBreak
  || w->m_kstart != w->m_kend || m_ktype != w->m_ktype) return(false);

  m_fWhiteSpaceChar = m_fWhiteSpaceChar && w->m_fWhiteSpaceChar;
  m_str += w->m_str;
  m_width += w->m_width;

  m_fDrawn = false;
  m_p = Com::SmartPoint(INT_MAX, INT_MAX);

  return(true);
}

void CWord::Paint(Com::SmartPoint p, Com::SmartPoint org)
{
  if(!m_str) return;

  if(!m_fDrawn)
  {
    if(!CreatePath()) return;

    Transform(Com::SmartPoint((org.x-p.x)*8, (org.y-p.y)*8));

    if(!ScanConvert()) return;

    if(m_style.borderStyle == 0 && (m_style.outlineWidthX+m_style.outlineWidthY > 0))
    {
      if(!CreateWidenedRegion((int)(m_style.outlineWidthX+0.5), (int)(m_style.outlineWidthY+0.5))) return;
    }
    else if(m_style.borderStyle == 1)
    {
      if(!CreateOpaqueBox()) return;
    }

    m_fDrawn = true;

    if(!Rasterize(p.x&7, p.y&7, m_style.fBlur, m_style.fGaussianBlur)) return;
  }
  else if((m_p.x&7) != (p.x&7) || (m_p.y&7) != (p.y&7))
  {
    Rasterize(p.x&7, p.y&7, m_style.fBlur, m_style.fGaussianBlur);
  }

  m_p = p;

  if(m_pOpaqueBox)
    m_pOpaqueBox->Paint(p, org);
}

void CWord::Transform(Com::SmartPoint org)
{
#ifdef _VSMOD
  // CPUID from VDub
  bool fSSE2 = !!(g_cpuid.m_flags & CCpuID::sse2);

  if(fSSE2) // SSE code
    Transform_SSE2(org);
  else // C-code
#endif
    Transform_C(org);
}

bool CWord::CreateOpaqueBox()
{
  if(m_pOpaqueBox) return(true);

  STSStyle style = m_style;
  style.borderStyle = 0;
  style.outlineWidthX = style.outlineWidthY = 0;
  style.colors[0] = m_style.colors[2];
  style.alpha[0] = m_style.alpha[2];

  int w = (int)(m_style.outlineWidthX + 0.5);
  int h = (int)(m_style.outlineWidthY + 0.5);

  CStdStringW str;
  str.Format(L"m %d %d l %d %d %d %d %d %d", 
    -w, -h, 
    m_width+w, -h, 
    m_width+w, m_ascent+m_descent+h, 
    -w, m_ascent+m_descent+h);

  m_pOpaqueBox = DNew CPolygon(style, str, 0, 0, 0, 1.0/8, 1.0/8, 0);

  return(!!m_pOpaqueBox);
}

void CWord::Transform_C( Com::SmartPoint &org )
{
  double scalex = m_style.fontScaleX/100;
  double scaley = m_style.fontScaleY/100;

  double caz = cos((3.1415/180)*m_style.fontAngleZ);
  double saz = sin((3.1415/180)*m_style.fontAngleZ);
  double cax = cos((3.1415/180)*m_style.fontAngleX);
  double sax = sin((3.1415/180)*m_style.fontAngleX);
  double cay = cos((3.1415/180)*m_style.fontAngleY);
  double say = sin((3.1415/180)*m_style.fontAngleY);

#ifdef _VSMOD
  // patch m003. random text points
  double xrnd = m_style.mod_rand.X*100;
  double yrnd = m_style.mod_rand.Y*100;
  double zrnd = m_style.mod_rand.Z*100;

  srand(m_style.mod_rand.Seed);

  // patch m008. distort
  int xsz,ysz;
  double dst1x,dst1y,dst2x,dst2y,dst3x,dst3y;
  int minx = INT_MAX, miny = INT_MAX, maxx = -INT_MAX, maxy = -INT_MAX;

  bool is_dist = m_style.mod_distort.enabled;
  if (is_dist)
  {
    for(int i = 0; i < mPathPoints; i++)
    {
      if(minx > mpPathPoints[i].x) minx = mpPathPoints[i].x;
      if(miny > mpPathPoints[i].y) miny = mpPathPoints[i].y;
      if(maxx < mpPathPoints[i].x) maxx = mpPathPoints[i].x;
      if(maxy < mpPathPoints[i].y) maxy = mpPathPoints[i].y;
    }

    xsz = max(maxx - minx, 0);
    ysz = max(maxy - miny, 0);

    dst1x = m_style.mod_distort.pointsx[0];
    dst1y = m_style.mod_distort.pointsy[0];
    dst2x = m_style.mod_distort.pointsx[1];
    dst2y = m_style.mod_distort.pointsy[1];
    dst3x = m_style.mod_distort.pointsx[2];
    dst3y = m_style.mod_distort.pointsy[2];
  }
#endif

  for(ptrdiff_t i = 0; i < mPathPoints; i++)
  {
    double x, y, z, xx, yy, zz;

    x = mpPathPoints[i].x;
    y = mpPathPoints[i].y;
#ifdef _VSMOD
    // patch m002. Z-coord
    z = m_style.mod_z;

    double u, v;
    if (is_dist)
    {
      u = (x-minx) / xsz;
      v = (y-miny) / ysz;

      x = minx+(0 + (dst1x - 0)*u + (dst3x-0)*v+(0+dst2x-dst1x-dst3x)*u*v)*xsz;
      y = miny+(0 + (dst1y - 0)*u + (dst3y-0)*v+(0+dst2y-dst1y-dst3y)*u*v)*ysz;
      //P = P0 + (P1 - P0)u + (P3 - P0)v + (P0 + P2 - P1 - P3)uv
    }

    // patch m003. random text points
    x = xrnd > 0 ? (xrnd - rand() % (int)(xrnd * 2 + 1)) / 100.0 + x : x;
    y = yrnd > 0 ? (yrnd - rand() % (int)(yrnd * 2 + 1)) / 100.0 + y : y;
    z = zrnd > 0 ? (zrnd - rand() % (int)(zrnd * 2 + 1)) / 100.0 + z : z;
#else
    z = 0;
#endif
    double _x = x;
    x = scalex * (x + m_style.fontShiftX * y) - org.x;
    y = scaley * (y + m_style.fontShiftY * _x) - org.y;

    xx = x*caz + y*saz;
    yy = -(x*saz - y*caz);
    zz = z;

    x = xx;
    y = yy*cax + zz*sax;
    z = yy*sax - zz*cax;

    xx = x*cay + z*say;
    yy = y;
    zz = x*say - z*cay;

    zz = max(zz, -19000);

    x = (xx * 20000) / (zz + 20000);
    y = (yy * 20000) / (zz + 20000);

    mpPathPoints[i].x = (LONG)(x + org.x + 0.5);
    mpPathPoints[i].y = (LONG)(y + org.y + 0.5);
  }
}

void CWord::Transform_SSE2( Com::SmartPoint &org )
{
  // __m128 union data type currently not supported with Intel C++ Compiler, so just call C version
#ifdef __ICL
  Transform_C(org);
#else
  // SSE code
  // speed up ~1.5-1.7x
  double scalex = m_style.fontScaleX/100;
  double scaley = m_style.fontScaleY/100;

  double caz = cos((3.1415/180)*m_style.fontAngleZ);
  double saz = sin((3.1415/180)*m_style.fontAngleZ);
  double cax = cos((3.1415/180)*m_style.fontAngleX);
  double sax = sin((3.1415/180)*m_style.fontAngleX);
  double cay = cos((3.1415/180)*m_style.fontAngleY);
  double say = sin((3.1415/180)*m_style.fontAngleY);

  __m128 __xshift = _mm_set_ps1(m_style.fontShiftX);
  __m128 __yshift = _mm_set_ps1(m_style.fontShiftY);

  __m128 __xorg = _mm_set_ps1(org.x);
  __m128 __yorg = _mm_set_ps1(org.y);

  __m128 __xscale = _mm_set_ps1(scalex);
  __m128 __yscale = _mm_set_ps1(scaley);

#ifdef _VSMOD
  // patch m003. random text points
  double xrnd = m_style.mod_rand.X*100;
  double yrnd = m_style.mod_rand.Y*100;
  double zrnd = m_style.mod_rand.Z*100;

  srand(m_style.mod_rand.Seed);

  __m128 __xsz = _mm_setzero_ps();
  __m128 __ysz = _mm_setzero_ps();

  __m128 __dst1x, __dst1y, __dst213x, __dst213y, __dst3x, __dst3y;

  __m128 __miny;
  __m128 __minx = _mm_set_ps(INT_MAX, INT_MAX, 0, 0);
  __m128 __max = _mm_set_ps(-INT_MAX, -INT_MAX, 1, 1);

  bool is_dist = m_style.mod_distort.enabled;
  if(is_dist)
  {
    for(int i = 0; i < mPathPoints; i++)
    {
      __m128 __point = _mm_set_ps(mpPathPoints[i].x, mpPathPoints[i].y, 0, 0);
      __minx = _mm_min_ps(__minx, __point);
      __max = _mm_max_ps(__max, __point);
    }

    __m128 __zero = _mm_setzero_ps();
    __max = _mm_sub_ps(__max, __minx); // xsz, ysz, 1, 1
    __max = _mm_max_ps(__max, __zero);

    __xsz = _mm_shuffle_ps(__max, __max, _MM_SHUFFLE(3,3,3,3));
    __ysz = _mm_shuffle_ps(__max, __max, _MM_SHUFFLE(2,2,2,2));

    __miny = _mm_shuffle_ps(__minx, __minx, _MM_SHUFFLE(2,2,2,2));
    __minx = _mm_shuffle_ps(__minx, __minx, _MM_SHUFFLE(3,3,3,3));

    __dst1x = _mm_set_ps1(m_style.mod_distort.pointsx[0]);
    __dst1y = _mm_set_ps1(m_style.mod_distort.pointsy[0]);
    __dst3x = _mm_set_ps1(m_style.mod_distort.pointsx[2]);
    __dst3y = _mm_set_ps1(m_style.mod_distort.pointsy[2]);
    __dst213x = _mm_set_ps1(m_style.mod_distort.pointsx[1]); // 2 - 1 - 3
    __dst213x = _mm_sub_ps(__dst213x, __dst1x);
    __dst213x = _mm_sub_ps(__dst213x, __dst3x);

    __dst213y = _mm_set_ps1(m_style.mod_distort.pointsy[1]);
    __dst213x = _mm_sub_ps(__dst213y, __dst1y);
    __dst213x = _mm_sub_ps(__dst213y, __dst3y); 
  }
#endif

  __m128 __caz = _mm_set_ps1(caz);
  __m128 __saz = _mm_set_ps1(saz);
  __m128 __cax = _mm_set_ps1(cax);
  __m128 __sax = _mm_set_ps1(sax);
  __m128 __cay = _mm_set_ps1(cay);
  __m128 __say = _mm_set_ps1(say);

  // this can be paralleled for openmp
  int mPathPointsD4 = mPathPoints / 4;
  int mPathPointsM4 = mPathPoints % 4;

  for(ptrdiff_t i = 0; i < mPathPointsD4 + 1; i++)
  {
    __m128 __pointx, __pointy;
    // we can't use load .-.
    if(i != mPathPointsD4)
    {
      __pointx = _mm_set_ps(mpPathPoints[4 * i + 0].x, mpPathPoints[4 * i + 1].x, mpPathPoints[4 * i + 2].x, mpPathPoints[4 * i + 3].x);
      __pointy = _mm_set_ps(mpPathPoints[4 * i + 0].y, mpPathPoints[4 * i + 1].y, mpPathPoints[4 * i + 2].y, mpPathPoints[4 * i + 3].y);
    }
    else // last cycle
    {
      switch(mPathPointsM4)
      {
      default:
      case 0: continue;
      case 1:
        __pointx = _mm_set_ps(mpPathPoints[4 * i + 0].x, 0, 0, 0);
        __pointy = _mm_set_ps(mpPathPoints[4 * i + 0].y, 0, 0, 0);
        break;
      case 2:
        __pointx = _mm_set_ps(mpPathPoints[4 * i + 0].x, mpPathPoints[4 * i + 1].x, 0, 0);
        __pointy = _mm_set_ps(mpPathPoints[4 * i + 0].y, mpPathPoints[4 * i + 1].y, 0, 0);
        break;
      case 3:
        __pointx = _mm_set_ps(mpPathPoints[4 * i + 0].x, mpPathPoints[4 * i + 1].x, mpPathPoints[4 * i + 2].x, 0);
        __pointy = _mm_set_ps(mpPathPoints[4 * i + 0].y, mpPathPoints[4 * i + 1].y, mpPathPoints[4 * i + 2].y, 0);
        break;
      }
    }

#ifdef _VSMOD
    __m128 __pointz = _mm_set_ps1(m_style.mod_z);

    // distort
    if(is_dist)
    {
      //P = P0 + (P1 - P0)u + (P3 - P0)v + (P0 + P2 - P1 - P3)uv
      __m128 __u = _mm_sub_ps(__pointx, __minx);
      __m128 __v = _mm_sub_ps(__pointy, __miny);
      __m128 __1_xsz = _mm_rcp_ps(__xsz);
      __m128 __1_ysz = _mm_rcp_ps(__ysz);
      __u = _mm_mul_ps(__u, __1_xsz);
      __v = _mm_mul_ps(__v, __1_ysz);

      // x
      __pointx = _mm_mul_ps(__dst213x, __u);
      __pointx = _mm_mul_ps(__pointx, __v);

      __m128 __tmpx = _mm_mul_ps(__dst3x, __v);
      __pointx = _mm_add_ps(__pointx, __tmpx);
      __tmpx = _mm_mul_ps(__dst1x, __u);
      __pointx = _mm_add_ps(__pointx, __tmpx);

      __pointx = _mm_mul_ps(__pointx, __xsz);
      __pointx = _mm_add_ps(__pointx, __minx);

      // y
      __pointy = _mm_mul_ps(__dst213y, __u);
      __pointy = _mm_mul_ps(__pointy, __v);

      __m128 __tmpy = _mm_mul_ps(__dst3y, __v);
      __pointy = _mm_add_ps(__pointy, __tmpy);
      __tmpy = _mm_mul_ps(__dst1y, __u);
      __pointy = _mm_add_ps(__pointy, __tmpy);

      __pointy = _mm_mul_ps(__pointy, __ysz);
      __pointy = _mm_add_ps(__pointy, __miny);
    }

    // randomize
    if(xrnd!=0 || yrnd!=0 || zrnd!=0)
    {
      __declspec(align(16)) float rx[4], ry[4], rz[4]; 
      for(int k=0;k<4;k++)
      {
        rx[k] = xrnd > 0 ? (xrnd - rand() % (int)(xrnd * 2 + 1)) : 0;
        ry[k] = yrnd > 0 ? (yrnd - rand() % (int)(yrnd * 2 + 1)) : 0;
        rz[k] = zrnd > 0 ? (zrnd - rand() % (int)(zrnd * 2 + 1)) : 0;
      }
      __m128 __001 = _mm_set_ps1(0.01f);

      if(xrnd!=0)
      {
        __m128 __rx = _mm_load_ps(rx);
        __rx = _mm_mul_ps(__rx, __001);
        __pointx = _mm_add_ps(__pointx, __rx);
      }

      if(yrnd!=0)
      {
        __m128 __ry = _mm_load_ps(ry);
        __ry = _mm_mul_ps(__ry, __001);
        __pointy = _mm_add_ps(__pointy, __ry);
      }

      if(zrnd!=0)
      {
        __m128 __rz = _mm_load_ps(rz);
        __rz = _mm_mul_ps(__rz, __001);
        __pointz = _mm_add_ps(__pointz, __rz);
      }
    }
#else
    __m128 __pointz = _mm_set_ps1(0);
#endif

    // scale and shift
    __m128 __tmpx;
    if(m_style.fontShiftX!=0)
    {
      __tmpx = _mm_mul_ps(__xshift, __pointy);
      __tmpx = _mm_add_ps(__tmpx, __pointx);
    }
    else
    {
      __tmpx = __pointx;
    }
    __tmpx = _mm_mul_ps(__tmpx, __xscale);
    __tmpx = _mm_sub_ps(__tmpx, __xorg);

    __m128 __tmpy;
    if(m_style.fontShiftY!=0)
    {
      __tmpy = _mm_mul_ps(__yshift, __pointx);
      __tmpy = _mm_add_ps(__tmpy, __pointy);
    }
    else
    {
      __tmpy = __pointy;
    }
    __tmpy = _mm_mul_ps(__tmpy, __yscale);
    __tmpy = _mm_sub_ps(__tmpy, __yorg);

    // rotate
    __m128 __xx = _mm_mul_ps(__tmpx, __caz);
    __m128 __yy = _mm_mul_ps(__tmpy, __saz);
    __pointx = _mm_add_ps(__xx, __yy);
    __xx = _mm_mul_ps(__tmpx, __saz);
    __yy = _mm_mul_ps(__tmpy, __caz);
    __pointy = _mm_sub_ps(__yy, __xx);

    __m128 __zz = _mm_mul_ps(__pointz, __sax);
    __yy = _mm_mul_ps(__pointy, __cax);
    __pointy = _mm_add_ps(__yy, __zz);
    __zz = _mm_mul_ps(__pointz, __cax);
    __yy = _mm_mul_ps(__pointy, __sax);
    __pointz = _mm_sub_ps(__zz, __yy);

    __xx = _mm_mul_ps(__pointx, __cay);
    __zz = _mm_mul_ps(__pointz, __say);
    __pointx = _mm_add_ps(__xx, __zz);
    __xx = _mm_mul_ps(__pointx, __say);
    __zz = _mm_mul_ps(__pointz, __cay);
    __pointz = _mm_sub_ps(__xx, __zz);

    __zz = _mm_set_ps1(-19000);
    __pointz = _mm_max_ps(__pointz, __zz);

    __m128 __20000 = _mm_set_ps1(20000);
    __zz = _mm_add_ps(__pointz, __20000);
    __zz = _mm_rcp_ps(__zz);

    __pointx = _mm_mul_ps(__pointx, __20000);
    __pointx = _mm_mul_ps(__pointx, __zz);

    __pointy = _mm_mul_ps(__pointy, __20000);
    __pointy = _mm_mul_ps(__pointy, __zz);

    __pointx = _mm_add_ps(__pointx, __xorg);
    __pointy = _mm_add_ps(__pointy, __yorg);

    __m128 __05 = _mm_set_ps1(0.5);

    __pointx = _mm_add_ps(__pointx, __05);
    __pointy = _mm_add_ps(__pointy, __05);

    if(i == mPathPointsD4) // last cycle
    {
      for(int k=0; k<mPathPointsM4; k++)
      {
        mpPathPoints[i*4+k].x = static_cast<LONG>(__pointx.m128_f32[3-k]);
        mpPathPoints[i*4+k].y = static_cast<LONG>(__pointy.m128_f32[3-k]);
      }
    }
    else
    {
      for(int k=0; k<4; k++)
      {
        mpPathPoints[i*4+k].x = static_cast<LONG>(__pointx.m128_f32[3-k]);
        mpPathPoints[i*4+k].y = static_cast<LONG>(__pointy.m128_f32[3-k]);
      }
    }
  }
#endif // __ICL
}

// CText

CText::CText(STSStyle& style, CStdStringW str, int ktype, int kstart, int kend)
  : CWord(style, str, ktype, kstart, kend)
{
  if(m_str == L" ")
  {
    m_fWhiteSpaceChar = true;
  }

  CMyFont font(m_style);

  HFONT hOldFont = SelectFont(g_hDC, font.hFont);

#ifdef _VSMOD // patch m007. symbol rotating
  double t = (double)m_style.mod_fontOrient * 3.1415926 / 1800;
#endif
  if(m_style.fontSpacing || (long)GetVersion() < 0)
  {
    for(LPCWSTR s = m_str; *s; s++)
    {
      Com::SmartSize extent;
      if(!GetTextExtentPoint32W(g_hDC, s, 1, &extent))
      {
        SelectFont(g_hDC, hOldFont);
        ASSERT(0);
        return;
      }
#ifdef _VSMOD // patch m007. symbol rotating
      m_width += (int)(extent.cx*abs(cos(t)) + extent.cy*abs(sin(t)) + m_style.fontSpacing);
#else
      m_width += extent.cx + (int)m_style.fontSpacing;
#endif
    }
//      m_width -= (int)m_style.fontSpacing; // TODO: subtract only at the end of the line
  }
  else
  {
    Com::SmartSize extent;
    if(!GetTextExtentPoint32W(g_hDC, m_str, wcslen(str), &extent))
    {
       SelectFont(g_hDC, hOldFont);
       ASSERT(0);
       return;
    }
#ifdef _VSMOD // patch m007. symbol rotating
    m_width += (int)(extent.cx*abs(cos(t)) + extent.cy*abs(sin(t)));
#else
    m_width += extent.cx;
#endif
  }

  m_width = (int)(m_style.fontScaleX/100*m_width + 4) >> 3;

  SelectFont(g_hDC, hOldFont);
}

CWord* CText::Copy()
{
  return(DNew CText(m_style, m_str, m_ktype, m_kstart, m_kend));
}

bool CText::Append(CWord* w)
{
  return(dynamic_cast<CText*>(w) && CWord::Append(w));
}

bool CText::CreatePath()
{
  CMyFont font(m_style);

  HFONT hOldFont = SelectFont(g_hDC, font.hFont);

  int width = 0;

  if(m_style.fontSpacing || (long)GetVersion() < 0)
  {
    bool bFirstPath = true;

    for(LPCWSTR s = m_str; *s; s++)
    {
      Com::SmartSize extent;
      if(!GetTextExtentPoint32W(g_hDC, s, 1, &extent))
      {
        SelectFont(g_hDC, hOldFont);
        ASSERT(0);
        return(false);
      }

      PartialBeginPath(g_hDC, bFirstPath);
      bFirstPath = false;
      TextOutW(g_hDC, 0, 0, s, 1);
      PartialEndPath(g_hDC, width, 0);

      width += extent.cx + (int)m_style.fontSpacing;
    }
  }
  else
  {
    Com::SmartSize extent;
    if(!GetTextExtentPoint32W(g_hDC, m_str, m_str.GetLength(), &extent))
    {
      SelectFont(g_hDC, hOldFont);
      ASSERT(0);
      return(false);
    }

    BeginPath(g_hDC);
    TextOutW(g_hDC, 0, 0, m_str, m_str.GetLength());
    EndPath(g_hDC);
  }

  SelectFont(g_hDC, hOldFont);

  return(true);
}

// CPolygon

CPolygon::CPolygon(STSStyle& style, CStdStringW str, int ktype, int kstart, int kend, double scalex, double scaley, int baseline) 
  : CWord(style, str, ktype, kstart, kend)
  , m_scalex(scalex), m_scaley(scaley), m_baseline(baseline)
{
  ParseStr();
}

CPolygon::~CPolygon()
{
}

CWord* CPolygon::Copy()
{
  return(DNew CPolygon(m_style, m_str, m_ktype, m_kstart, m_kend, m_scalex, m_scaley, m_baseline));
}

bool CPolygon::Append(CWord* w)
{
  CPolygon* p = dynamic_cast<CPolygon*>(w);
  if(!p) return(false);

  // TODO
  return(false);
}

bool CPolygon::GetLONG(CStdStringW& str, LONG& ret)
{
  LPWSTR s = (LPWSTR)(LPCWSTR)str, e = s;
  ret = wcstol(str, &e, 10);
  str = str.Mid(e - s);
  return(e > s);
}

bool CPolygon::GetPOINT(CStdStringW& str, POINT& ret)
{
  return(GetLONG(str, ret.x) && GetLONG(str, ret.y));
}

bool CPolygon::ParseStr()
{
  if(m_pathTypesOrg.size() > 0) return(true);

  Com::SmartPoint p;
  int i, j, lastsplinestart = -1, firstmoveto = -1, lastmoveto = -1;

  CStdStringW str = m_str;
  str.SpanIncluding(L"mnlbspc 0123456789");
  str.Replace(L"m", L"*m");
  str.Replace(L"n", L"*n");
  str.Replace(L"l", L"*l");
  str.Replace(L"b", L"*b");
  str.Replace(L"s", L"*s");
  str.Replace(L"p", L"*p");
  str.Replace(L"c", L"*c");

  int k = 0;
  std::vector<CStdStringW> tokens; str.Tokenize(L"*", tokens);
  CStdStringW s;
  while (k < tokens.size())
  {
    s = tokens[k++];
    WCHAR c = s[0];
    s.TrimLeft(L"mnlbspc ");
    switch(c)
    {
    case 'm': 
      lastmoveto = m_pathTypesOrg.size();
      if(firstmoveto == -1) firstmoveto = lastmoveto;
      while(GetPOINT(s, p))
      {
        m_pathTypesOrg.push_back(PT_MOVETO);
        m_pathPointsOrg.push_back(p);
      }
      break;
    case 'n':
      while(GetPOINT(s, p))
      {
        m_pathTypesOrg.push_back(PT_MOVETONC);
        m_pathPointsOrg.push_back(p);
      }
      break;
    case 'l':
      if (m_pathPointsOrg.size() < 1) break;
      while(GetPOINT(s, p))
      {
        m_pathTypesOrg.push_back(PT_LINETO);
        m_pathPointsOrg.push_back(p);
      }
      break;
    case 'b':
      j = m_pathTypesOrg.size();
      while(GetPOINT(s, p))
      {
        m_pathTypesOrg.push_back(PT_BEZIERTO);
        m_pathPointsOrg.push_back(p);
        j++;
      }
      j = m_pathTypesOrg.size() - ((m_pathTypesOrg.size() - j) % 3);
      m_pathTypesOrg.resize(j);
      m_pathPointsOrg.resize(j);
      break;
    case 's':
      if (m_pathPointsOrg.size() < 1) break;
      j = lastsplinestart = m_pathTypesOrg.size();
      i = 3;
      while(i-- && GetPOINT(s, p))
      {
        m_pathTypesOrg.push_back(PT_BSPLINETO);
        m_pathPointsOrg.push_back(p);
        j++;
      }
      if(m_pathTypesOrg.size() - lastsplinestart < 3)
      {
        m_pathTypesOrg.resize(lastsplinestart);
        m_pathPointsOrg.resize(lastsplinestart);
        lastsplinestart = -1;
      }
      // no break here
    case 'p':
      if (m_pathPointsOrg.size() < 3) break;
      while(GetPOINT(s, p))
      {
        m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
        m_pathPointsOrg.push_back(p);
      }
      break;
    case 'c':
      if(lastsplinestart > 0)
      {
        m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
        m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
        m_pathTypesOrg.push_back(PT_BSPLINEPATCHTO);
        p = m_pathPointsOrg[lastsplinestart-1]; // we need p for temp storage, because operator [] will return a reference to CPoint and Add() may reallocate its internal buffer (this is true for MFC 7.0 but not for 6.0, hehe)
        m_pathPointsOrg.push_back(p);
        p = m_pathPointsOrg[lastsplinestart];
        m_pathPointsOrg.push_back(p);
        p = m_pathPointsOrg[lastsplinestart+1];
        m_pathPointsOrg.push_back(p);
        lastsplinestart = -1;
      }
      break;
    default:
      break;
    }
  }

  if(lastmoveto == -1 || firstmoveto > 0) 
  {
    m_pathTypesOrg.clear();
    m_pathPointsOrg.clear();
    return(false);
  }

  int minx = INT_MAX, miny = INT_MAX, maxx = -INT_MAX, maxy = -INT_MAX;

  for(i = 0; i < m_pathTypesOrg.size(); i++)
  {
    m_pathPointsOrg[i].x = (int)(64 * m_scalex * m_pathPointsOrg[i].x);
    m_pathPointsOrg[i].y = (int)(64 * m_scaley * m_pathPointsOrg[i].y);
    if(minx > m_pathPointsOrg[i].x) minx = m_pathPointsOrg[i].x;
    if(miny > m_pathPointsOrg[i].y) miny = m_pathPointsOrg[i].y;
    if(maxx < m_pathPointsOrg[i].x) maxx = m_pathPointsOrg[i].x;
    if(maxy < m_pathPointsOrg[i].y) maxy = m_pathPointsOrg[i].y;
  }

  m_width = max(maxx - minx, 0);
  m_ascent = max(maxy - miny, 0);

  int baseline = (int)(64 * m_scaley * m_baseline);
  m_descent = baseline;
  m_ascent -= baseline;

  m_width = ((int)(m_style.fontScaleX/100 * m_width) + 4) >> 3;
  m_ascent = ((int)(m_style.fontScaleY/100 * m_ascent) + 4) >> 3;
  m_descent = ((int)(m_style.fontScaleY/100 * m_descent) + 4) >> 3;

  return(true);
}

bool CPolygon::CreatePath()
{
  int len = m_pathTypesOrg.size();
  if(len == 0) return(false);

  if(mPathPoints != len)
  {
    mpPathTypes = (BYTE*)realloc(mpPathTypes, len*sizeof(BYTE));
    mpPathPoints = (POINT*)realloc(mpPathPoints, len*sizeof(POINT));
    if(!mpPathTypes || !mpPathPoints) return(false);
    mPathPoints = len;
  }

  memcpy(mpPathTypes, &m_pathTypesOrg[0], len*sizeof(BYTE));
  memcpy(mpPathPoints, &m_pathPointsOrg[0], len*sizeof(POINT));

  return(true);
}

// CClipper

CClipper::CClipper(CStdStringW str, Com::SmartSize size, double scalex, double scaley, bool inverse) 
  : CPolygon(STSStyle(), str, 0, 0, 0, scalex, scaley, 0)
{
  m_size.cx = m_size.cy = 0;
  m_pAlphaMask = NULL;

  if(size.cx < 0 || size.cy < 0)
    return;

  m_pAlphaMask = DNew BYTE[size.cx*size.cy];
  if (!m_pAlphaMask)
    return;

  m_size = size;
  m_inverse = inverse;

  memset(m_pAlphaMask, 0, size.cx*size.cy);

  Paint(Com::SmartPoint(0, 0), Com::SmartPoint(0, 0));

  int w = mOverlayWidth, h = mOverlayHeight;

  int x = (mOffsetX+4)>>3, y = (mOffsetY+4)>>3;
  int xo = 0, yo = 0;

  if(x < 0) {xo = -x; w -= -x; x = 0;}
  if(y < 0) {yo = -y; h -= -y; y = 0;}
  if(x+w > m_size.cx) w = m_size.cx-x;
  if(y+h > m_size.cy) h = m_size.cy-y;

  if(w <= 0 || h <= 0) return;

  const BYTE* src = mpOverlayBuffer + 2*(mOverlayWidth * yo + xo);
  BYTE* dst = m_pAlphaMask + m_size.cx * y + x;

  while(h--)
  {
    for(ptrdiff_t wt=0; wt<w; ++wt)
      dst[wt] = src[wt*2];

    src += 2*mOverlayWidth;
    dst += m_size.cx;
  }

  if(inverse)
  {
    BYTE* dst = m_pAlphaMask;
    for(ptrdiff_t i = size.cx*size.cy; i>0; --i, ++dst)
      *dst = 0x40 - *dst; // mask is 6 bit
  }
}

CClipper::~CClipper()
{
  if(m_pAlphaMask) delete [] m_pAlphaMask;
  m_pAlphaMask = NULL;
}

CWord* CClipper::Copy()
{
  return(DNew CClipper(m_str, m_size, m_scalex, m_scaley, m_inverse));
}

bool CClipper::Append(CWord* w)
{
  return(false);
}

// CLine

CLine::~CLine()
{
  while (! empty())
  {
    delete back();
    pop_back();
  }
}

void CLine::Compact()
{
  while (! empty())
  {
    CWord* w = front();
    if(!w->m_fWhiteSpaceChar) break;

    m_width -= w->m_width;
    delete w;
    pop_front();
  }

  while (! empty())
  {
    CWord* w = back();
    if(!w->m_fWhiteSpaceChar) break;

    m_width -= w->m_width;
    delete w;
    pop_back();
  }

  if(empty()) return;

  CLine l;
  l.insert(l.begin(), begin(), end());
  clear();

  CWord* last = NULL;

  std::list<CWord *>::iterator it = l.begin();
  while (it != l.end())
  {
    CWord* w = *it; it++;

    if(!last || !last->Append(w))
      push_back(last = w->Copy());
  }

  m_ascent = m_descent = m_borderX = m_borderY = 0;

  it = l.begin();
  while (it != l.end())
  {
    CWord* w = *it; it++;

    if(m_ascent < w->m_ascent) m_ascent = w->m_ascent;
    if(m_descent < w->m_descent) m_descent = w->m_descent;
    if(m_borderX < w->m_style.outlineWidthX) m_borderX = (int)(w->m_style.outlineWidthX+0.5);
    if(m_borderY < w->m_style.outlineWidthY) m_borderY = (int)(w->m_style.outlineWidthY+0.5);
  }
}

#ifdef _VSMOD // patch m006. moveable vector clip
Com::SmartRect CLine::PaintShadow(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha, MOD_MOVEVC& mod_vc, REFERENCE_TIME rt)
#else
Com::SmartRect CLine::PaintShadow(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha)
#endif
{
  Com::SmartRect bbox(0, 0, 0, 0);

  std::list<CWord *>::iterator it = begin();
  for(; it != end(); ++it)
  {
    CWord* w = *it;

    if(w->m_fLineBreak) return(bbox); // should not happen since this class is just a line of text without any breaks

    if(w->m_style.shadowDepthX != 0 || w->m_style.shadowDepthY != 0)
    {
      int x = p.x + (int)(w->m_style.shadowDepthX+0.5);
#ifdef _VSMOD // patch m001. Vertical fontspacing
      int y = p.y - w->m_style.mod_verticalSpace + m_ascent - w->m_ascent + (int)(w->m_style.shadowDepthY+0.5);
#else
      int y = p.y + m_ascent - w->m_ascent + (int)(w->m_style.shadowDepthY+0.5);
#endif
      DWORD a = 0xff - w->m_style.alpha[3];
      if(alpha > 0) a = MulDiv(a, 0xff - alpha, 0xff);
      COLORREF shadow = revcolor(w->m_style.colors[3]) | (a<<24);
      DWORD sw[6] = {shadow, -1};

#ifdef _VSMOD // patch m011. jitter
      CPoint mod_jitter = w->m_style.mod_jitter.getOffset(rt);
      x += mod_jitter.x;
      y += mod_jitter.y;
      // patch m010. png background
      // subpixel positioning
      w->m_style.mod_grad.subpixx = x&7;
      w->m_style.mod_grad.subpixy = y&7;
      w->m_style.mod_grad.fadalpha = alpha;
#endif
      w->Paint(Com::SmartPoint(x, y), org);

      if(w->m_style.borderStyle == 0)
      {
#ifdef _VSMOD // patch m004. gradient colors
        bbox |= w->Draw(spd, clipRect, pAlphaMask, x, y, sw, 
          w->m_ktype > 0 || w->m_style.alpha[0] < 0xff, 
          (w->m_style.outlineWidthX+w->m_style.outlineWidthY > 0) && !(w->m_ktype == 2 && time < w->m_kstart),3,w->m_style.mod_grad, mod_vc);
#else
        bbox |= w->Draw(spd, clipRect, pAlphaMask, x, y, sw, 
          w->m_ktype > 0 || w->m_style.alpha[0] < 0xff, 
          (w->m_style.outlineWidthX+w->m_style.outlineWidthY > 0) && !(w->m_ktype == 2 && time < w->m_kstart));
#endif
      }
      else if(w->m_style.borderStyle == 1 && w->m_pOpaqueBox)
      {
#ifdef _VSMOD // patch m004. gradient colors
        bbox |= w->m_pOpaqueBox->Draw(spd, clipRect, pAlphaMask, x, y, sw, true, false,3,w->m_style.mod_grad, mod_vc);
#else
        bbox |= w->m_pOpaqueBox->Draw(spd, clipRect, pAlphaMask, x, y, sw, true, false);
#endif
      }
    }

    p.x += w->m_width;
  }

  return(bbox);
}

#ifdef _VSMOD // patch m006. moveable vector clip
Com::SmartRect CLine::PaintOutline(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha, MOD_MOVEVC& mod_vc, REFERENCE_TIME rt)
#else
Com::SmartRect CLine::PaintOutline(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha)
#endif
{
  Com::SmartRect bbox(0, 0, 0, 0);

  std::list<CWord *>::iterator it = begin();
  for(; it != end(); ++it)
  {
    CWord* w = *it;

    if(w->m_fLineBreak) return(bbox); // should not happen since this class is just a line of text without any breaks

    if(w->m_style.outlineWidthX+w->m_style.outlineWidthY > 0 && !(w->m_ktype == 2 && time < w->m_kstart))
    {
      int x = p.x;
#ifdef _VSMOD // patch m001. Vertical fontspacing
      int y = p.y - w->m_style.mod_verticalSpace + m_ascent - w->m_ascent;
#else
      int y = p.y + m_ascent - w->m_ascent;
#endif
      DWORD aoutline = w->m_style.alpha[2];
      if(alpha > 0) aoutline += MulDiv(alpha, 0xff - w->m_style.alpha[2], 0xff);
      COLORREF outline = revcolor(w->m_style.colors[2]) | ((0xff-aoutline)<<24);
      DWORD sw[6] = {outline, -1};

#ifdef _VSMOD // patch m011. jitter
      Com::SmartPoint mod_jitter = w->m_style.mod_jitter.getOffset(rt);
      x += mod_jitter.x;
      y += mod_jitter.y;
      // patch m010. png background
      // subpixel positioning
      w->m_style.mod_grad.subpixx = x&7;
      w->m_style.mod_grad.subpixy = y&7;
      w->m_style.mod_grad.fadalpha = alpha;
#endif

      w->Paint(Com::SmartPoint(x, y), org);

      if(w->m_style.borderStyle == 0)
      {
#ifdef _VSMOD // patch m004. gradient colors
        bbox |= w->Draw(spd, clipRect, pAlphaMask, x, y, sw, !w->m_style.alpha[0] && !w->m_style.alpha[1] && !alpha, true,2,w->m_style.mod_grad, mod_vc);
#else
        bbox |= w->Draw(spd, clipRect, pAlphaMask, x, y, sw, !w->m_style.alpha[0] && !w->m_style.alpha[1] && !alpha, true);
#endif
      }
      else if(w->m_style.borderStyle == 1 && w->m_pOpaqueBox)
      {
#ifdef _VSMOD // patch m004. gradient colors
        bbox |= w->m_pOpaqueBox->Draw(spd, clipRect, pAlphaMask, x, y, sw, true, false,2,w->m_style.mod_grad, mod_vc);
#else
        bbox |= w->m_pOpaqueBox->Draw(spd, clipRect, pAlphaMask, x, y, sw, true, false);
#endif
      }
    }

    p.x += w->m_width;
  }

  return(bbox);
}

#ifdef _VSMOD // patch m006. moveable vector clip
Com::SmartRect CLine::PaintBody(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha, MOD_MOVEVC& mod_vc, REFERENCE_TIME rt)
#else
Com::SmartRect CLine::PaintBody(SubPicDesc& spd, Com::SmartRect& clipRect, BYTE* pAlphaMask, Com::SmartPoint p, Com::SmartPoint org, int time, int alpha)
#endif
{
  Com::SmartRect bbox(0, 0, 0, 0);

  std::list<CWord *>::iterator it = begin();
  for(; it != end(); ++it)
  {
    CWord* w = *it;

    if(w->m_fLineBreak) return(bbox); // should not happen since this class is just a line of text without any breaks

    int x = p.x;
#ifdef _VSMOD // patch m001. Vertical fontspacing
    int y = p.y - w->m_style.mod_verticalSpace + m_ascent - w->m_ascent;
#else
    int y = p.y + m_ascent - w->m_ascent;
#endif
    // colors

    DWORD aprimary = w->m_style.alpha[0];
    if(alpha > 0) aprimary += MulDiv(alpha, 0xff - w->m_style.alpha[0], 0xff);
    COLORREF primary = revcolor(w->m_style.colors[0]) | ((0xff-aprimary)<<24);

    DWORD asecondary = w->m_style.alpha[1];
    if(alpha > 0) asecondary += MulDiv(alpha, 0xff - w->m_style.alpha[1], 0xff);
    COLORREF secondary = revcolor(w->m_style.colors[1]) | ((0xff-asecondary)<<24);

    DWORD sw[6] = {primary, 0, secondary};

    // karaoke

    double t;

    if(w->m_ktype == 0 || w->m_ktype == 2)
    {
      t = time < w->m_kstart ? 0 : 1;
    }
    else if(w->m_ktype == 1)
    {
      if(time < w->m_kstart) t = 0;
      else if(time < w->m_kend) 
      {
        t = 1.0 * (time - w->m_kstart) / (w->m_kend - w->m_kstart);

        double angle = fmod(w->m_style.fontAngleZ, 360.0);
        if(angle > 90 && angle < 270) 
        {
          t = 1-t;
          COLORREF tmp = sw[0];
          sw[0] = sw[2];
          sw[2] = tmp;
        }
      }
      else t = 1.0;
    }

    if(t >= 1)
    {
      sw[1] = 0xFFFFFFF;
    }

    // move dividerpoint
    int bluradjust = 0;
    if (w->m_style.fGaussianBlur > 0)
      bluradjust += (int)(w->m_style.fGaussianBlur*3*8 + 0.5) | 1;
    if (w->m_style.fBlur)
    bluradjust += 8;
    double tx = w->m_style.fontAngleZ;
    sw[4] = sw[2];
    sw[5] = 0x00ffffff;

#ifdef _VSMOD // patch m011. jitter
    Com::SmartPoint mod_jitter = w->m_style.mod_jitter.getOffset(rt);
    x += mod_jitter.x;
    y += mod_jitter.y;
    // patch m010. png background
    // subpixel positioning
    w->m_style.mod_grad.subpixx = x&7;
    w->m_style.mod_grad.subpixy = y&7;
    w->m_style.mod_grad.fadalpha = alpha;
#endif

    w->Paint(Com::SmartPoint(x, y), org);

    sw[3] = (int)(w->m_style.outlineWidthX + t*w->getOverlayWidth() + t*bluradjust) >> 3;

#ifdef _VSMOD // patch m004. gradient colors
    bbox |= w->Draw(spd, clipRect, pAlphaMask, x, y, sw, true, false,0,w->m_style.mod_grad, mod_vc);
#else
    bbox |= w->Draw(spd, clipRect, pAlphaMask, x, y, sw, true, false);
#endif
    p.x += w->m_width;
  }

  return(bbox);
}


// CSubtitle

CSubtitle::CSubtitle()
{
  memset(m_effects, 0, sizeof(Effect*)*EF_NUMBEROFEFFECTS);
  m_pClipper = NULL;
  m_clipInverse = false;
  m_scalex = m_scaley = 1;
}

CSubtitle::~CSubtitle()
{
  Empty();
}

void CSubtitle::Empty()
{
  while (!empty())
  {
    delete back();
    pop_back();
  }

  while (! m_words.empty())
  {
    delete m_words.back();
    m_words.pop_back();
  }

  for(int i = 0; i < EF_NUMBEROFEFFECTS; i++) {if(m_effects[i]) delete m_effects[i];}
  memset(m_effects, 0, sizeof(Effect*)*EF_NUMBEROFEFFECTS);

  if(m_pClipper) delete m_pClipper;
  m_pClipper = NULL;
}

int CSubtitle::GetFullWidth()
{
  int width = 0;

  std::list<CWord *>::const_iterator it = m_words.begin();
  for (; it != m_words.end(); ++it)
    width += (*it)->m_width;

  return(width);
}

int CSubtitle::GetFullLineWidth(std::list<CWord *>::iterator it)
{
  int width = 0;

  for(; it != m_words.end(); ++it) 
  {
    CWord* w = *it;
    if(w->m_fLineBreak) break;
    width += w->m_width;
  }

  return(width);
}

int CSubtitle::GetWrapWidth(std::list<CWord *>::iterator it, int maxwidth)
{
  if(m_wrapStyle == 0 || m_wrapStyle == 3)
  {
    if(maxwidth > 0) 
    {
//      int fullwidth = GetFullWidth();
      int fullwidth = GetFullLineWidth(it);

      int minwidth = fullwidth / ((abs(fullwidth) / maxwidth) + 1);

      int width = 0, wordwidth = 0;

      for(; m_words.end() != it && width < minwidth; ++it)
      {
        CWord* w = *it;
        wordwidth = w->m_width;
        if(abs(width + wordwidth) < abs(maxwidth)) width += wordwidth;
      }

      maxwidth = width;

      if(m_wrapStyle == 3 && it != m_words.end()) maxwidth -= wordwidth;
    }
  }
  else if(m_wrapStyle == 1)
  {
//    maxwidth = maxwidth;
  }
  else if(m_wrapStyle == 2)
  {
    maxwidth = INT_MAX;
  }

  return(maxwidth);
}

CLine* CSubtitle::GetNextLine(std::list<CWord *>::iterator& it, int maxwidth)
{
  CLine* ret = DNew CLine();
  if(!ret) return(NULL);

  ret->m_width = ret->m_ascent = ret->m_descent = ret->m_borderX = ret->m_borderY = 0;

  maxwidth = GetWrapWidth(it, maxwidth);

  bool fEmptyLine = true;

  while(it != m_words.end())
  {
    CWord* w = *it; it++;

    if(ret->m_ascent < w->m_ascent) ret->m_ascent = w->m_ascent;
    if(ret->m_descent < w->m_descent) ret->m_descent = w->m_descent;
    if(ret->m_borderX < w->m_style.outlineWidthX) ret->m_borderX = (int)(w->m_style.outlineWidthX+0.5);
    if(ret->m_borderY < w->m_style.outlineWidthY) ret->m_borderY = (int)(w->m_style.outlineWidthY+0.5);

    if(w->m_fLineBreak) 
    {
            if(fEmptyLine)
            {
                ret->m_ascent /= 2;
                ret->m_descent /= 2;
                ret->m_borderX = ret->m_borderY = 0;
            }

      ret->Compact();

      return(ret);
    }

    fEmptyLine = false;

    bool fWSC = w->m_fWhiteSpaceChar;

    int width = w->m_width;
    std::list<CWord *>::iterator it2 = it;
    while(it2 != m_words.end())
    {
      if((*it2)->m_fWhiteSpaceChar != fWSC
      || (*it2)->m_fLineBreak) break;

      CWord* w2 = *it2; it2++;
      width += w2->m_width;
    }

    if((ret->m_width += width) <= maxwidth || ret->empty()) 
    {
      ret->push_back(w->Copy());
      
      while(it != it2)
      {
        ret->push_back((*it)->Copy()); it++;
      }

      it = it2;
    }
    else
    {
      if(it != m_words.end()) it--;
      else it = m_words.end();

      ret->m_width -= width;

      break;
    }
  }

  ret->Compact();

  return(ret);
}

void CSubtitle::CreateClippers(Com::SmartSize size)
{
  size.cx >>= 3;
  size.cy >>= 3;

  if(m_effects[EF_BANNER] && m_effects[EF_BANNER]->param[2])
  {
    int width = m_effects[EF_BANNER]->param[2];

    int w = size.cx, h = size.cy;

    if(!m_pClipper) 
    {
      CStdStringW str;
      str.Format(L"m %d %d l %d %d %d %d %d %d", 0, 0, w, 0, w, h, 0, h);
      m_pClipper = DNew CClipper(str, size, 1, 1, false);
      if(!m_pClipper) return;
    }

    int da = (64<<8)/width;
    BYTE* am = m_pClipper->m_pAlphaMask;

    for(ptrdiff_t j = 0; j < h; j++, am += w)
    {
      int a = 0;
      int k = min(width, w);
      
      for(ptrdiff_t i = 0; i < k; i++, a += da)
        am[i] = (am[i]*a)>>14;

      a = 0x40<<8;
      k = w-width;

      if(k < 0) {a -= -k*da; k = 0;}
            
      for(int i = k; i < w; i++, a -= da)
        am[i] = (am[i]*a)>>14;
    }
  }
  else if(m_effects[EF_SCROLL] && m_effects[EF_SCROLL]->param[4])
  {
    int height = m_effects[EF_SCROLL]->param[4];

    int w = size.cx, h = size.cy;

    if(!m_pClipper) 
    {
      CStdStringW str;
      str.Format(L"m %d %d l %d %d %d %d %d %d", 0, 0, w, 0, w, h, 0, h);
      m_pClipper = DNew CClipper(str, size, 1, 1, false);
      if(!m_pClipper) return;
    }

    int da = (64<<8)/height;
    int a = 0;
    int k = m_effects[EF_SCROLL]->param[0]>>3;
    int l = k+height;
    if(k < 0) {a += -k*da; k = 0;}
    if(l > h) {l = h;}

    if(k < h)
    {
      BYTE* am = &m_pClipper->m_pAlphaMask[k*w];

      memset(m_pClipper->m_pAlphaMask, 0, am - m_pClipper->m_pAlphaMask);

      for(ptrdiff_t j = k; j < l; j++, a += da)
      {
        for(ptrdiff_t i = 0; i < w; i++, am++)
          *am = ((*am)*a)>>14;
      }
    }

    da = -(64<<8)/height;
    a = 0x40<<8;
    l = m_effects[EF_SCROLL]->param[1]>>3;
    k = l-height;
    if(k < 0) {a += -k*da; k = 0;}
    if(l > h) {l = h;}

    if(k < h)
    {
      BYTE* am = &m_pClipper->m_pAlphaMask[k*w];

      int j = k;
      for(; j < l; j++, a += da)
      {
        for(ptrdiff_t i = 0; i < w; i++, am++)
          *am = ((*am)*a)>>14;
      }

      memset(am, 0, (h-j)*w);
    }
  }
}

void CSubtitle::MakeLines(Com::SmartSize size, Com::SmartRect marginRect)
{
  Com::SmartSize spaceNeeded(0, 0);

  bool fFirstLine = true;

  m_topborder = m_bottomborder = 0;

  CLine* l = NULL;
  
  std::list<CWord *>::iterator it = m_words.begin();
  while(it != m_words.end())
  {
    l = GetNextLine(it, size.cx - marginRect.left - marginRect.right);
    if(!l) break;

        if(fFirstLine)
        {
            m_topborder = l->m_borderY;
            fFirstLine = false;
        }

    spaceNeeded.cx = max(l->m_width+l->m_borderX, spaceNeeded.cx);
    spaceNeeded.cy += l->m_ascent + l->m_descent;

    push_back(l);
  }

  if(l) m_bottomborder = l->m_borderY;

  m_rect = Com::SmartRect(
    Com::SmartPoint((m_scrAlignment%3) == 1 ? marginRect.left
        : (m_scrAlignment%3) == 2 ? (marginRect.left + (size.cx - marginRect.right) - spaceNeeded.cx + 1) / 2
        : (size.cx - marginRect.right - spaceNeeded.cx),
        m_scrAlignment <= 3 ? (size.cy - marginRect.bottom - spaceNeeded.cy)
        : m_scrAlignment <= 6 ? (marginRect.top + (size.cy - marginRect.bottom) - spaceNeeded.cy + 1) / 2
        : marginRect.top),
    spaceNeeded);
}

// CScreenLayoutAllocator

void CScreenLayoutAllocator::Empty()
{
  m_subrects.clear();
}

void CScreenLayoutAllocator::AdvanceToSegment(int segment, const std::vector<int>& sa)
{
  std::list<SubRect>::iterator it = m_subrects.begin();
  while (it != m_subrects.end())
  {
    std::list<SubRect>::iterator prev = it;

    SubRect& sr = *it; it++;

    bool fFound = false;

    if(abs(sr.segment - segment) <= 1) // using abs() makes it possible to play the subs backwards, too :)
    {
      for(ptrdiff_t i = 0; i < sa.size() && !fFound; i++)
      {
        if(sa[i] == sr.entry) 
        {
          sr.segment = segment;
          fFound = true;
        }
      }
    }

    if(!fFound) m_subrects.erase(prev);
  }
}

Com::SmartRect CScreenLayoutAllocator::AllocRect(CSubtitle* s, int segment, int entry, int layer, int collisions)
{
  // TODO: handle collisions == 1 (reversed collisions)

  std::list<SubRect>::iterator it = m_subrects.begin();
  while (it != m_subrects.end())
  {
    SubRect& sr = *it; it++;
    if(sr.segment == segment && sr.entry == entry) 
    {
      return(sr.r + Com::SmartRect(0, -s->m_topborder, 0, -s->m_bottomborder));
    }
  }

  Com::SmartRect r = s->m_rect + Com::SmartRect(0, s->m_topborder, 0, s->m_bottomborder);

  bool fSearchDown = s->m_scrAlignment > 3;

  bool fOK;

  do
  {
    fOK = true;

    it = m_subrects.begin();
    while (it != m_subrects.end())
    {
      SubRect& sr = *it; it++;

      if(layer == sr.layer && !(r & sr.r).IsRectEmpty())
      {
        if(fSearchDown)
        {
          r.bottom = sr.r.bottom + r.Height();
          r.top = sr.r.bottom;
        }
        else
        {
          r.top = sr.r.top - r.Height();
          r.bottom = sr.r.top;
        }
        
        fOK = false;
      }
    }
  }
  while(!fOK);

  SubRect sr;
  sr.r = r;
  sr.segment = segment;
  sr.entry = entry;
  sr.layer = layer;
  m_subrects.push_back(sr);
  
  return(sr.r + Com::SmartRect(0, -s->m_topborder, 0, -s->m_bottomborder));
}

// CRenderedTextSubtitle

CRenderedTextSubtitle::CRenderedTextSubtitle(CCritSec* pLock, STSStyle *styleOverride, bool doOverride)
  : ISubPicProviderImpl(pLock), m_pStyleOverride(styleOverride), m_doOverrideStyle(doOverride)
{
  m_size = Com::SmartSize(0, 0);

  if(g_hDC_refcnt == 0) 
  {
    g_hDC = CreateCompatibleDC(NULL);
#ifdef _VSMOD // patch m007. symbol rotating
    SetGraphicsMode(g_hDC, GM_ADVANCED); // patch for lfOrientation
#endif
    SetBkMode(g_hDC, TRANSPARENT); 
    SetTextColor(g_hDC, 0xffffff); 
    SetMapMode(g_hDC, MM_TEXT);
  }

  g_hDC_refcnt++;
}

CRenderedTextSubtitle::~CRenderedTextSubtitle()
{
  Deinit();

  g_hDC_refcnt--;
  if(g_hDC_refcnt == 0) DeleteDC(g_hDC);
}

void CRenderedTextSubtitle::assign(CSimpleTextSubtitle& sts)
{
  __super::assign(sts);

  m_size = Com::SmartSize(0, 0);

  if(CRenderedTextSubtitle* pRTS = dynamic_cast<CRenderedTextSubtitle*>(&sts))
  {
    m_size = pRTS->m_size;
  }
}

void CRenderedTextSubtitle::Empty()
{
  Deinit();

  __super::Empty();
}

void CRenderedTextSubtitle::OnChanged()
{
  __super::OnChanged();

  std::map<int, CSubtitle*>::const_iterator it = m_subtitleCache.begin();
  for (; it != m_subtitleCache.end(); ++it)
  {
    delete it->second;
  }

  m_subtitleCache.clear();

  m_sla.Empty();
}

bool CRenderedTextSubtitle::Init(Com::SmartSize size, Com::SmartRect vidrect)
{
  Deinit();

  m_size = Com::SmartSize(size.cx*8, size.cy*8);
  m_vidrect = Com::SmartRect(vidrect.left*8, vidrect.top*8, vidrect.right*8, vidrect.bottom*8);

  m_sla.Empty();

  return(true);
}

void CRenderedTextSubtitle::Deinit()
{
  std::map<int, CSubtitle*>::const_iterator it = m_subtitleCache.begin();
  for (; it != m_subtitleCache.end(); ++it)
  {
    delete it->second;
  }

  m_subtitleCache.clear();

  m_sla.Empty();

  m_size = Com::SmartSize(0, 0);
  m_vidrect.SetRectEmpty();
}

void CRenderedTextSubtitle::ParseEffect(CSubtitle* sub, CStdString str)
{
  str.Trim();
  if(!sub || str.IsEmpty()) return;

  const TCHAR* s = _tcschr(str, ';');
  if(!s) {s = (LPTSTR)(LPCTSTR)str; s += str.GetLength()-1;}
  s++;
  CStdString effect = str.Left(s - str);

  if(!effect.CompareNoCase(_T("Banner;")))
  {
    int delay, lefttoright = 0, fadeawaywidth = 0;
    if(_stscanf(s, _T("%d;%d;%d"), &delay, &lefttoright, &fadeawaywidth) < 1) return;

    Effect* e = DNew Effect;
    if(!e) return;

    sub->m_effects[e->type = EF_BANNER] = e;
    e->param[0] = (int)(max(1.0*delay/sub->m_scalex, 1));
    e->param[1] = lefttoright;
    e->param[2] = (int)(sub->m_scalex*fadeawaywidth);

    sub->m_wrapStyle = 2;
  }
  else if(!effect.CompareNoCase(_T("Scroll up;")) || !effect.CompareNoCase(_T("Scroll down;")))
  {
    int top, bottom, delay, fadeawayheight = 0;
    if(_stscanf(s, _T("%d;%d;%d;%d"), &top, &bottom, &delay, &fadeawayheight) < 3) return;

        if(top > bottom)
        {
            int tmp = top;
            top = bottom;
            bottom = tmp;
        }

    Effect* e = DNew Effect;
    if(!e) return;

    sub->m_effects[e->type = EF_SCROLL] = e;
    e->param[0] = (int)(sub->m_scaley*top*8);
    e->param[1] = (int)(sub->m_scaley*bottom*8);
    e->param[2] = (int)(max(1.0*delay/sub->m_scaley, 1));
    e->param[3] = (effect.GetLength() == 12);
    e->param[4] = (int)(sub->m_scaley*fadeawayheight);
  }
}

void CRenderedTextSubtitle::ParseString(CSubtitle* sub, CStdStringW str, STSStyle& style)
{
  if(!sub) return;

  str.Replace(L"\\N", L"\n");
  str.Replace(L"\\n", (sub->m_wrapStyle < 2 || sub->m_wrapStyle == 3) ? L" " : L"\n");
  str.Replace(L"\\h", L"\x00A0");

  for(int i = 0, j = 0, len = str.GetLength(); j < len; j++)
  {
    wchar_t c = str[j];

    if (j == len - 1)
      j = len;
    else if(c != L'\n' && c != ' ' && c != L'\x00A0')
      continue;

    if(i < j)
    {
      if(CWord* w = DNew CText(style, str.Mid(i, j-i), m_ktype, m_kstart, m_kend))
      {
        sub->m_words.push_back(w); 
        m_kstart = m_kend;
      }
    }

    if(c == L'\n')
    {
      if(CWord* w = DNew CText(style, CStdStringW(), m_ktype, m_kstart, m_kend))
      {
        sub->m_words.push_back(w); 
        m_kstart = m_kend;
      }
    }
    else if(c == L' ' || c == L'\x00A0')
    {
      wchar_t pszCh[2] = { c , 0 };
      if(CWord* w = DNew CText(style, CStdStringW((wchar_t*) &pszCh), m_ktype, m_kstart, m_kend))
      {
        sub->m_words.push_back(w); 
        m_kstart = m_kend;
      }
    }

    i = j+1;
  }

  return;
}

void CRenderedTextSubtitle::ParsePolygon(CSubtitle* sub, CStdStringW str, STSStyle& style)
{
  if(!sub || !str.GetLength() || !m_nPolygon) return;

  if(CWord* w = DNew CPolygon(style, str, m_ktype, m_kstart, m_kend, sub->m_scalex/(1<<(m_nPolygon-1)), sub->m_scaley/(1<<(m_nPolygon-1)), m_polygonBaselineOffset))
  {
    sub->m_words.push_back(w); 
    m_kstart = m_kend;
  }
}

bool CRenderedTextSubtitle::ParseSSATag(CSubtitle* sub, CStdStringW str, STSStyle& style, STSStyle& org, bool fAnimate)
{
  if(!sub) return(false);

  int nTags = 0, nUnrecognizedTags = 0;
  bool lastone = false;
  for(int i = 0, j; (j = str.Find('\\', i)) >= 0; i = j)
  {
    size_t pos = str.find_first_of(L"(\\", j + 1);

    CStdStringW cmd;
    
    if (pos == std::string::npos)
    {
      cmd = str.substr(j + 1);
      lastone = true;
    }
    else
      cmd = str.substr(j + 1, pos - (j + 1));
    
    cmd.Trim();
    if(cmd.IsEmpty())
    {
      if (i==j)
        break;
      continue;
    }

    j = pos;

    std::vector<CStdStringW> params;

    if(j != std::string::npos && str[j] == L'(')
    {
      pos = str.find_first_of(L')', j + 1);
      
      CStdStringW param;
      if (pos == std::string::npos)
        param = str.substr(j + 1);
      else
        param = str.substr(j + 1, pos - (j + 1));

      param.Trim();

      while(!param.IsEmpty())
      {
        int i = param.Find(L','), j = param.Find(L'\\');

        if(i >= 0 && (j < 0 || i < j))
        {
          CStdStringW s = param.Left(i).Trim();
          if(!s.IsEmpty()) params.push_back(s);
          param = i+1 < param.GetLength() ? param.Mid(i+1) : L"";
        }
        else
        {
          param.Trim();
          if(!param.IsEmpty()) params.push_back(param);
          param.Empty();
        }
      }
    }

    if(!cmd.Find(L"1c") || !cmd.Find(L"2c") || !cmd.Find(L"3c") || !cmd.Find(L"4c"))
      params.push_back(cmd.Mid(2).Trim(L"&H")), cmd = cmd.Left(2);
    else if(!cmd.Find(L"1a") || !cmd.Find(L"2a") || !cmd.Find(L"3a") || !cmd.Find(L"4a"))
      params.push_back(cmd.Mid(2).Trim(L"&H")), cmd = cmd.Left(2);
    else if(!cmd.Find(L"alpha"))
      params.push_back(cmd.Mid(5).Trim(L"&H")), cmd = cmd.Left(5);
    else if(!cmd.Find(L"an"))
      params.push_back(cmd.Mid(2)), cmd = cmd.Left(2);
    else if(!cmd.Find(L"a"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"blur"))
      params.push_back(cmd.Mid(4)), cmd = cmd.Left(4);
    else if(!cmd.Find(L"bord"))
      params.push_back(cmd.Mid(4)), cmd = cmd.Left(4);
    else if(!cmd.Find(L"be"))
      params.push_back(cmd.Mid(2)), cmd = cmd.Left(2);
    else if(!cmd.Find(L"b"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"clip"))
      ;
    else if(!cmd.Find(L"c"))
      params.push_back(cmd.Mid(1).Trim(L"&H")), cmd = cmd.Left(1);
    else if(!cmd.Find(L"fade"))
      ;
    else if(!cmd.Find(L"fe"))
      params.push_back(cmd.Mid(2)), cmd = cmd.Left(2);
    else if(!cmd.Find(L"fn"))
      params.push_back(cmd.Mid(2)), cmd = cmd.Left(2);
    else if(!cmd.Find(L"frx") || !cmd.Find(L"fry") || !cmd.Find(L"frz"))
      params.push_back(cmd.Mid(3)), cmd = cmd.Left(3);
    else if(!cmd.Find(L"fax") || !cmd.Find(L"fay"))
      params.push_back(cmd.Mid(3)), cmd = cmd.Left(3);
    else if(!cmd.Find(L"fr"))
      params.push_back(cmd.Mid(2)), cmd = cmd.Left(2);
    else if(!cmd.Find(L"fscx") || !cmd.Find(L"fscy"))
      params.push_back(cmd.Mid(4)), cmd = cmd.Left(4);
    else if(!cmd.Find(L"fsc"))
      params.push_back(cmd.Mid(3)), cmd = cmd.Left(3);
    else if(!cmd.Find(L"fsp"))
      params.push_back(cmd.Mid(3)), cmd = cmd.Left(3);
    else if(!cmd.Find(L"fs"))
      params.push_back(cmd.Mid(2)), cmd = cmd.Left(2);
    else if(!cmd.Find(L"iclip"))
      ;
    else if(!cmd.Find(L"i"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"kt") || !cmd.Find(L"kf") || !cmd.Find(L"ko"))
      params.push_back(cmd.Mid(2)), cmd = cmd.Left(2);
    else if(!cmd.Find(L"k") || !cmd.Find(L"K"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"move"))
      ;
    else if(!cmd.Find(L"org"))
      ;
    else if(!cmd.Find(L"pbo"))
      params.push_back(cmd.Mid(3)), cmd = cmd.Left(3);
    else if(!cmd.Find(L"pos"))
      ;
    else if(!cmd.Find(L"p"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"q"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"r"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"shad"))
      params.push_back(cmd.Mid(4)), cmd = cmd.Left(4);
    else if(!cmd.Find(L"s"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"t"))
      ;
    else if(!cmd.Find(L"u"))
      params.push_back(cmd.Mid(1)), cmd = cmd.Left(1);
    else if(!cmd.Find(L"xbord"))
      params.push_back(cmd.Mid(5)), cmd = cmd.Left(5);
    else if(!cmd.Find(L"xshad"))
      params.push_back(cmd.Mid(5)), cmd = cmd.Left(5);
    else if(!cmd.Find(L"ybord"))
      params.push_back(cmd.Mid(5)), cmd = cmd.Left(5);
    else if(!cmd.Find(L"yshad"))
      params.push_back(cmd.Mid(5)), cmd = cmd.Left(5);
    else
      nUnrecognizedTags++;

    nTags++;

    // TODO: call ParseStyleModifier(cmd, params, ..) and move the rest there

    CStdStringW p = params.size() > 0 ? params[0] : L"";

    if(cmd == L"1c" || cmd == L"2c" || cmd == L"3c" || cmd == L"4c")
    {
      int i = cmd[0] - '1';

      DWORD c = wcstol(p, NULL, 16);
      style.colors[i] = !p.IsEmpty()
        ? (((int)CalcAnimation(c&0xff, style.colors[i]&0xff, fAnimate))&0xff
          |((int)CalcAnimation(c&0xff00, style.colors[i]&0xff00, fAnimate))&0xff00
          |((int)CalcAnimation(c&0xff0000, style.colors[i]&0xff0000, fAnimate))&0xff0000)
        : org.colors[i];
    }
    else if(cmd == L"1a" || cmd == L"2a" || cmd == L"3a" || cmd == L"4a")
    {
      DWORD al = wcstol(p, NULL, 16)&0xff;
      int i = cmd[0] - '1';

      style.alpha[i] = !p.IsEmpty()
        ? (BYTE)CalcAnimation(al, style.alpha[i], fAnimate)
        : org.alpha[i];
    }
    else if(cmd == L"alpha")
    {
      for(ptrdiff_t i = 0; i < 4; i++)
      {
        DWORD al = wcstol(p, NULL, 16)&0xff;
        style.alpha[i] = !p.IsEmpty()
          ? (BYTE)CalcAnimation(al, style.alpha[i], fAnimate)
          : org.alpha[i];
      }
    }
    else if(cmd == L"an")
    {
      int n = wcstol(p, NULL, 10);
      if(sub->m_scrAlignment < 0)
                sub->m_scrAlignment = (n > 0 && n < 10) ? n : org.scrAlignment;
    }
    else if(cmd == L"a")
    {
      int n = wcstol(p, NULL, 10);
      if(sub->m_scrAlignment < 0)
                sub->m_scrAlignment = (n > 0 && n < 12) ? ((((n-1)&3)+1)+((n&4)?6:0)+((n&8)?3:0)) : org.scrAlignment;
    }
    else if(cmd == L"blur")
    {
      double n = CalcAnimation(wcstod(p, NULL), style.fGaussianBlur, fAnimate);
      style.fGaussianBlur = !p.IsEmpty()
        ? (n < 0 ? 0 : n)
        : org.fGaussianBlur;
    }
    else if(cmd == L"bord")
    {
      double dst = wcstod(p, NULL);
      double nx = CalcAnimation(dst, style.outlineWidthX, fAnimate);
      style.outlineWidthX = !p.IsEmpty()
        ? (nx < 0 ? 0 : nx)
        : org.outlineWidthX;
      double ny = CalcAnimation(dst, style.outlineWidthY, fAnimate);
      style.outlineWidthY = !p.IsEmpty()
        ? (ny < 0 ? 0 : ny)
        : org.outlineWidthY;
    }
    else if(cmd == L"be")
    {
      int n = (int)(CalcAnimation(wcstol(p, NULL, 10), style.fBlur, fAnimate)+0.5);
      style.fBlur = !p.IsEmpty()
        ? n
        : org.fBlur;
    }
    else if(cmd == L"b")
    {
      int n = wcstol(p, NULL, 10);
      style.fontWeight = !p.IsEmpty()
        ? (n == 0 ? FW_NORMAL : n == 1 ? FW_BOLD : n >= 100 ? n : org.fontWeight)
        : org.fontWeight;
    }
    else if(cmd == L"clip" || cmd == L"iclip")
    {
      bool invert = (cmd == L"iclip");

      if(params.size() == 1 && !sub->m_pClipper)
      {
        sub->m_pClipper = DNew CClipper(params[0], Com::SmartSize(m_size.cx>>3, m_size.cy>>3), sub->m_scalex, sub->m_scaley, invert);
      }
      else if(params.size() == 2 && !sub->m_pClipper)
      {
        int scale = max(wcstol(p, NULL, 10), 1);
        sub->m_pClipper = DNew CClipper(params[1], Com::SmartSize(m_size.cx>>3, m_size.cy>>3), sub->m_scalex/(1<<(scale-1)), sub->m_scaley/(1<<(scale-1)), invert);
      }
      else if(params.size() == 4)
      {
        Com::SmartRect r;

        sub->m_clipInverse = invert;

        r.SetRect(
          wcstol(params[0], NULL, 10),
          wcstol(params[1], NULL, 10),
          wcstol(params[2], NULL, 10),
          wcstol(params[3], NULL, 10));

        Com::SmartPoint o(0, 0);

        if(sub->m_relativeTo == 1) // TODO: this should also apply to the other two clippings above
        {
          o.x = m_vidrect.left>>3;
          o.y = m_vidrect.top>>3;
        }

        sub->m_clip.SetRect(
          (int)CalcAnimation(sub->m_scalex*r.left + o.x, sub->m_clip.left, fAnimate),
          (int)CalcAnimation(sub->m_scaley*r.top + o.y, sub->m_clip.top, fAnimate),
          (int)CalcAnimation(sub->m_scalex*r.right + o.x, sub->m_clip.right, fAnimate),
          (int)CalcAnimation(sub->m_scaley*r.bottom + o.y, sub->m_clip.bottom, fAnimate));
      }
    }
    else if(cmd == L"c")
    {
      DWORD c = wcstol(p, NULL, 16);
      style.colors[0] = !p.IsEmpty()
        ? (((int)CalcAnimation(c&0xff, style.colors[0]&0xff, fAnimate))&0xff
          |((int)CalcAnimation(c&0xff00, style.colors[0]&0xff00, fAnimate))&0xff00
          |((int)CalcAnimation(c&0xff0000, style.colors[0]&0xff0000, fAnimate))&0xff0000)
        : org.colors[0];
    }
        else if(cmd == L"fade" || cmd == L"fad")
    {
      if(params.size() == 7 && !sub->m_effects[EF_FADE])// {\fade(a1=param[0], a2=param[1], a3=param[2], t1=t[0], t2=t[1], t3=t[2], t4=t[3])
      {
        if(Effect* e = DNew Effect)
        {
          for(ptrdiff_t i = 0; i < 3; i++)
            e->param[i] = wcstol(params[i], NULL, 10);
          for(ptrdiff_t i = 0; i < 4; i++)
            e->t[i] = wcstol(params[3+i], NULL, 10);
                  
          sub->m_effects[EF_FADE] = e;
        }
      }
      else if(params.size() == 2 && !sub->m_effects[EF_FADE]) // {\fad(t1=t[1], t2=t[2])
      {
        if(Effect* e = DNew Effect)
        {
          e->param[0] = e->param[2] = 0xff;
          e->param[1] = 0x00;
          for(ptrdiff_t i = 1; i < 3; i++) 
            e->t[i] = wcstol(params[i-1], NULL, 10);
          e->t[0] = e->t[3] = -1; // will be substituted with "start" and "end"

          sub->m_effects[EF_FADE] = e;
        }
      }
    }
    else if(cmd == L"fax")
    {
      style.fontShiftX = !p.IsEmpty()
        ? CalcAnimation(wcstod(p, NULL), style.fontShiftX, fAnimate)
        : org.fontShiftX;
    }
    else if(cmd == L"fay")
    {
      style.fontShiftY = !p.IsEmpty()
        ? CalcAnimation(wcstod(p, NULL), style.fontShiftY, fAnimate)
        : org.fontShiftY;
    }
    else if(cmd == L"fe")
    {
      int n = wcstol(p, NULL, 10);
      style.charSet = !p.IsEmpty()
        ? n
        : org.charSet;
    }
    else if(cmd == L"fn")
    {
      style.fontName = (!p.IsEmpty() && p != L"0")
        ? CStdString(p).Trim()
        : org.fontName;
    }
    else if(cmd == L"frx")
    {
      style.fontAngleX = !p.IsEmpty()
        ? CalcAnimation(wcstod(p, NULL), style.fontAngleX, fAnimate)
        : org.fontAngleX;
    }
    else if(cmd == L"fry")
    {
      style.fontAngleY = !p.IsEmpty()
        ? CalcAnimation(wcstod(p, NULL), style.fontAngleY, fAnimate)
        : org.fontAngleY;
    }
    else if(cmd == L"frz" || cmd == L"fr")
    {
      style.fontAngleZ = !p.IsEmpty()
        ? CalcAnimation(wcstod(p, NULL), style.fontAngleZ, fAnimate)
        : org.fontAngleZ;
    }
    else if(cmd == L"fscx")
    {
      double n = CalcAnimation(wcstol(p, NULL, 10), style.fontScaleX, fAnimate);
      style.fontScaleX = !p.IsEmpty()
        ? ((n < 0) ? 0 : n)
        : org.fontScaleX;
    }
    else if(cmd == L"fscy")
    {
      double n = CalcAnimation(wcstol(p, NULL, 10), style.fontScaleY, fAnimate);
      style.fontScaleY = !p.IsEmpty()
        ? ((n < 0) ? 0 : n)
        : org.fontScaleY;
    }
    else if(cmd == L"fsc")
    {
      style.fontScaleX = org.fontScaleX;
      style.fontScaleY = org.fontScaleY;
    }
    else if(cmd == L"fsp")
    {
      style.fontSpacing = !p.IsEmpty()
        ? CalcAnimation(wcstod(p, NULL), style.fontSpacing, fAnimate)
        : org.fontSpacing;
    }
    else if(cmd == L"fs")
    {
      if(!p.IsEmpty())
      {
        if(p[0] == L'-' || p[0] == L'+')
        {
          double n = CalcAnimation(style.fontSize + style.fontSize*wcstol(p, NULL, 10)/10, style.fontSize, fAnimate);
          style.fontSize = (n > 0) ? n : org.fontSize;
        }
        else
        {
          double n = CalcAnimation(wcstol(p, NULL, 10), style.fontSize, fAnimate);
          style.fontSize = (n > 0) ? n : org.fontSize;
        }
      }
      else
      {
        style.fontSize = org.fontSize;
      }
    }
    else if(cmd == L"i")
    {
      int n = wcstol(p, NULL, 10);
      style.fItalic = !p.IsEmpty()
        ? (n == 0 ? false : n == 1 ? true : org.fItalic)
        : org.fItalic;
    }
    else if(cmd == L"kt")
    {
      m_kstart = !p.IsEmpty() 
        ? wcstol(p, NULL, 10)*10
        : 0;
      m_kend = m_kstart;
    }
    else if(cmd == L"kf" || cmd == L"K")
    {
      m_ktype = 1;
      m_kstart = m_kend;
      m_kend += !p.IsEmpty() 
        ? wcstol(p, NULL, 10)*10
        : 1000;
    }
    else if(cmd == L"ko")
    {
      m_ktype = 2;
      m_kstart = m_kend;
      m_kend += !p.IsEmpty() 
        ? wcstol(p, NULL, 10)*10
        : 1000;
    }
    else if(cmd == L"k")
    {
      m_ktype = 0;
      m_kstart = m_kend;
      m_kend += !p.IsEmpty() 
        ? wcstol(p, NULL, 10)*10
        : 1000;
    }
    else if(cmd == L"move") // {\move(x1=param[0], y1=param[1], x2=param[2], y2=param[3][, t1=t[0], t2=t[1]])}
    {
      if((params.size() == 4 || params.size() == 6) && !sub->m_effects[EF_MOVE])
      {
        if(Effect* e = DNew Effect)
        {
          e->param[0] = (int)(sub->m_scalex*wcstod(params[0], NULL)*8);
          e->param[1] = (int)(sub->m_scaley*wcstod(params[1], NULL)*8);
          e->param[2] = (int)(sub->m_scalex*wcstod(params[2], NULL)*8);
          e->param[3] = (int)(sub->m_scaley*wcstod(params[3], NULL)*8);
          e->t[0] = e->t[1] = -1;

          if(params.size() == 6)
          {
            for(ptrdiff_t i = 0; i < 2; i++)
              e->t[i] = wcstol(params[4+i], NULL, 10);
          }

          sub->m_effects[EF_MOVE] = e;
        }
      }
    }
    else if(cmd == L"org") // {\org(x=param[0], y=param[1])}
    {
      if(params.size() == 2 && !sub->m_effects[EF_ORG])
      {
        if(Effect* e = DNew Effect)
        {
          e->param[0] = (int)(sub->m_scalex*wcstod(params[0], NULL)*8);
          e->param[1] = (int)(sub->m_scaley*wcstod(params[1], NULL)*8);

          sub->m_effects[EF_ORG] = e;
        }
      }
    }
    else if(cmd == L"pbo")
    {
      m_polygonBaselineOffset = wcstol(p, NULL, 10);
    }
    else if(cmd == L"pos")
    {
      if(params.size() == 2 && !sub->m_effects[EF_MOVE])
      {
        if(Effect* e = DNew Effect)
        {
          e->param[0] = e->param[2] = (int)(sub->m_scalex*wcstod(params[0], NULL)*8);
          e->param[1] = e->param[3] = (int)(sub->m_scaley*wcstod(params[1], NULL)*8);
          e->t[0] = e->t[1] = 0;

          sub->m_effects[EF_MOVE] = e;
        }
      }   
    }
    else if(cmd == L"p")
    {
      int n = wcstol(p, NULL, 10);
      m_nPolygon = (n <= 0 ? 0 : n);
    }
    else if(cmd == L"q")
    {
      int n = wcstol(p, NULL, 10);
      sub->m_wrapStyle = !p.IsEmpty() && (0 <= n && n <= 3)
        ? n
        : m_defaultWrapStyle;
    }
    else if(cmd == L"r")
    {
      STSStyle* val;
      CSTSStyleMap::iterator it = m_styles.find(CStdString(p));
      style = (!p.IsEmpty() && it != m_styles.end() && it->second) ? *(it->second) : org;
    }
    else if(cmd == L"shad")
    {
      double dst = wcstod(p, NULL);
      double nx = CalcAnimation(dst, style.shadowDepthX, fAnimate);
      style.shadowDepthX = !p.IsEmpty()
        ? (nx < 0 ? 0 : nx)
        : org.shadowDepthX;
      double ny = CalcAnimation(dst, style.shadowDepthY, fAnimate);
      style.shadowDepthY = !p.IsEmpty()
        ? (ny < 0 ? 0 : ny)
        : org.shadowDepthY;
    }
    else if(cmd == L"s")
    {
      int n = wcstol(p, NULL, 10);
      style.fStrikeOut = !p.IsEmpty()
        ? (n == 0 ? false : n == 1 ? true : org.fStrikeOut)
        : org.fStrikeOut;
    }
    else if(cmd == L"t") // \t([<t1>,<t2>,][<accel>,]<style modifiers>)
    {
      p.Empty();

      m_animStart = m_animEnd = 0;
      m_animAccel = 1;

      if(params.size() == 1)
      {
        p = params[0];
      }
      else if(params.size() == 2)
      {
        m_animAccel = wcstod(params[0], NULL);
        p = params[1];
      }
      else if(params.size() == 3)
      {
        m_animStart = (int)wcstod(params[0], NULL); 
        m_animEnd = (int)wcstod(params[1], NULL);
        p = params[2];
      }
      else if(params.size() == 4)
      {
        m_animStart = wcstol(params[0], NULL, 10); 
        m_animEnd = wcstol(params[1], NULL, 10);
        m_animAccel = wcstod(params[2], NULL);
        p = params[3];
      }

      ParseSSATag(sub, p, style, org, true);

      sub->m_fAnimated = true;
    }
    else if(cmd == L"u")
    {
      int n = wcstol(p, NULL, 10);
      style.fUnderline = !p.IsEmpty()
        ? (n == 0 ? false : n == 1 ? true : org.fUnderline)
        : org.fUnderline;
    }
    else if(cmd == L"xbord")
    {
      double dst = wcstod(p, NULL);
      double nx = CalcAnimation(dst, style.outlineWidthX, fAnimate);
      style.outlineWidthX = !p.IsEmpty()
        ? (nx < 0 ? 0 : nx)
        : org.outlineWidthX;
    }
    else if(cmd == L"xshad")
    {
      double dst = wcstod(p, NULL);
      double nx = CalcAnimation(dst, style.shadowDepthX, fAnimate);
      style.shadowDepthX = !p.IsEmpty()
        ? nx
        : org.shadowDepthX;
    }
    else if(cmd == L"ybord")
    {
      double dst = wcstod(p, NULL);
      double ny = CalcAnimation(dst, style.outlineWidthY, fAnimate);
      style.outlineWidthY = !p.IsEmpty()
        ? (ny < 0 ? 0 : ny)
        : org.outlineWidthY;
    }
    else if(cmd == L"yshad")
    {
      double dst = wcstod(p, NULL);
      double ny = CalcAnimation(dst, style.shadowDepthY, fAnimate);
      style.shadowDepthY = !p.IsEmpty()
        ? ny
        : org.shadowDepthY;
    }
    if (lastone)
      break;
  }

//  return(nUnrecognizedTags < nTags);
  return(true); // there are ppl keeping coments inside {}, lets make them happy now
}

bool CRenderedTextSubtitle::ParseHtmlTag(CSubtitle* sub, CStdStringW str, STSStyle& style, STSStyle& org)
{
  if(str.Find(L"!--") == 0)
    return(true);

  bool fClosing = str[0] == '/';
  str.Trim(L" /");

  int i = str.Find(' ');
  if(i < 0) i = str.GetLength();

  CStdStringW tag = str.Left(i).MakeLower();
  str = str.Mid(i).Trim();

  std::vector<CStdStringW> attribs, params;
  while((i = str.Find('=')) > 0)
  {
    attribs.push_back(str.Left(i).Trim().MakeLower());
    str = str.Mid(i+1);
    for(i = 0; _istspace(str[i]); i++);
    str = str.Mid(i);
    if(str[0] == '\"') {str = str.Mid(1); i = str.Find('\"');}
    else i = str.Find(' ');
    if(i < 0) i = str.GetLength();
    params.push_back(str.Left(i).Trim().MakeLower());
    str = str.Mid(i+1);
  }

  if(tag == L"text")
    ;
  else if(tag == L"b" || tag == L"strong")
    style.fontWeight = !fClosing ? FW_BOLD : org.fontWeight;
  else if(tag == L"i" || tag == L"em")
    style.fItalic = !fClosing ? true : org.fItalic;
  else if(tag == L"u")
    style.fUnderline = !fClosing ? true : org.fUnderline;
  else if(tag == L"s" || tag == L"strike" || tag == L"del")
    style.fStrikeOut = !fClosing ? true : org.fStrikeOut;
  else if(tag == L"font")
  {
    if(!fClosing)
    {
      for(i = 0; i < attribs.size(); i++)
      {
        if(params[i].IsEmpty()) continue;

        int nColor = -1;

        if(attribs[i] == L"face")
        {
          style.fontName = params[i];
        }
        else if(attribs[i] == L"size")
        {
          if(params[i][0] == '+')
            style.fontSize += wcstol(params[i], NULL, 10);
          else if(params[i][0] == '-')
            style.fontSize -= wcstol(params[i], NULL, 10);
          else
            style.fontSize = wcstol(params[i], NULL, 10);
        }
        else if(attribs[i] == L"color")
        {
          nColor = 0;
        }
        else if(attribs[i] == L"outline-color")
        {
          nColor = 2;
        }
        else if(attribs[i] == L"outline-level")
        {
          style.outlineWidthX = style.outlineWidthY = wcstol(params[i], NULL, 10);
        }
        else if(attribs[i] == L"shadow-color")
        {
          nColor = 3;
        }
        else if(attribs[i] == L"shadow-level")
        {
          style.shadowDepthX = style.shadowDepthY = wcstol(params[i], NULL, 10);
        }

        if(nColor >= 0 && nColor < 4)
        {
          CStdString key = WToT(params[i]).TrimLeft('#');
          DWORD val;
          CHtmlColorMap::const_iterator it = g_colors.find(key);
          if(it != g_colors.end())
            style.colors[nColor] = it->second;
          else if((style.colors[nColor] = _tcstol(key, NULL, 16)) == 0)
            style.colors[nColor] = 0x00ffffff;  // default is white
          style.colors[nColor] = ((style.colors[nColor]>>16)&0xff)|((style.colors[nColor]&0xff)<<16)|(style.colors[nColor]&0x00ff00);
        }
      }
    }
    else
    {
      style.fontName = org.fontName;
      style.fontSize = org.fontSize;
      memcpy(style.colors, org.colors, sizeof(style.colors));
    }
  }
  else if(tag == L"k" && attribs.size() == 1 && attribs[0] == L"t")
  {
    m_ktype = 1;
    m_kstart = m_kend;
    m_kend += wcstol(params[0], NULL, 10);
  }
  else 
    return(false);

  return(true);
}

double CRenderedTextSubtitle::CalcAnimation(double dst, double src, bool fAnimate)
{
  int s = m_animStart ? m_animStart : 0;
  int e = m_animEnd ? m_animEnd : m_delay;

  if(fabs(dst-src) >= 0.0001 && fAnimate)
  {
    if(m_time < s) dst = src;
    else if(s <= m_time && m_time < e)
    {
      double t = pow(1.0 * (m_time - s) / (e - s), m_animAccel);
      dst = (1 - t) * src + t * dst;
    }
//    else dst = dst;
  }

  return(dst);
}

CSubtitle* CRenderedTextSubtitle::GetSubtitle(int entry)
{
  CSubtitle* sub;
  std::map<int, CSubtitle*>::iterator it = m_subtitleCache.find(entry);
  if(it != m_subtitleCache.end()) 
  {
    if(it->second->m_fAnimated) {delete it->second; it->second = NULL;}
    else return(it->second);
  }

  sub = DNew CSubtitle();
  if(!sub) return(NULL);

  CStdStringW str = GetStrW(entry, true);

  STSStyle stss, orgstss;
  if(m_doOverrideStyle && m_pStyleOverride != NULL)
  {
    // this RTS has been signaled to ignore embedded styles, use the built-in one
    stss = *m_pStyleOverride;
  }
  else
  {
    // find the appropriate embedded style
    GetStyle(entry, stss);
  }
  if (m_ePARCompensationType == EPCTUpscale)
  {
    if (stss.fontScaleX / stss.fontScaleY == 1.0 && m_dPARCompensation != 1.0)
    {
      if (m_dPARCompensation < 1.0)
        stss.fontScaleY /= m_dPARCompensation;
      else
        stss.fontScaleX *= m_dPARCompensation;
    }
  }
  else if (m_ePARCompensationType == EPCTDownscale)
  {
    if (stss.fontScaleX / stss.fontScaleY == 1.0 && m_dPARCompensation != 1.0)
    {
      if (m_dPARCompensation < 1.0)
        stss.fontScaleX *= m_dPARCompensation;
      else
        stss.fontScaleY /= m_dPARCompensation;
    }
  }
  else if (m_ePARCompensationType == EPCTAccurateSize)
  {
    if (stss.fontScaleX / stss.fontScaleY == 1.0 && m_dPARCompensation != 1.0)
    {
      stss.fontScaleX *= m_dPARCompensation;
    }
  }

  orgstss = stss;

  sub->m_clip.SetRect(0, 0, m_size.cx>>3, m_size.cy>>3);
  sub->m_scrAlignment = -stss.scrAlignment;
  sub->m_wrapStyle = m_defaultWrapStyle;
  sub->m_fAnimated = false;
  sub->m_relativeTo = stss.relativeTo;
  // this whole conditional is a work-around for what happens in STS.cpp:
  // in CSimpleTextSubtitle::Open, we have m_dstScreenSize = CSize(384, 288)
  // now, files containing embedded subtitles (and with styles) set m_dstScreenSize to a correct value
  // but where no style is given, those defaults are taken - 384, 288
  if(m_doOverrideStyle && m_pStyleOverride != NULL)
  {
    // so mind the default values, stated here to increase comprehension
    sub->m_scalex = (stss.relativeTo == 1 ? m_vidrect.Width() : m_size.cx) / (384 * 8);
    sub->m_scaley = (stss.relativeTo == 1 ? m_vidrect.Height() : m_size.cy) / (288 * 8);
  }
  else
  {
    sub->m_scalex = m_dstScreenSize.cx > 0 ? 1.0 * (stss.relativeTo == 1 ? m_vidrect.Width() : m_size.cx) / (m_dstScreenSize.cx*8) : 1.0;
    sub->m_scaley = m_dstScreenSize.cy > 0 ? 1.0 * (stss.relativeTo == 1 ? m_vidrect.Height() : m_size.cy) / (m_dstScreenSize.cy*8) : 1.0;
  }

  m_animStart = m_animEnd = 0;
  m_animAccel = 1;
  m_ktype = m_kstart = m_kend = 0;
  m_nPolygon = 0;
  m_polygonBaselineOffset = 0;
#ifdef _VSMOD // patch m004. gradient colors
  // allow init gradient without \$vc \$va
  for (int i=0;i<4;i++)
    for (int j=0;j<4;j++)
    {
      stss.mod_grad.alpha[i][j] = stss.alpha[i];
      stss.mod_grad.color[i][j] = stss.colors[i];
    }
#endif
    ParseEffect(sub, this->at(entry).effect);

  while(!str.IsEmpty())
  {
    bool fParsed = false;

    int i;

    if(str[0] == '{' && (i = str.Find(L'}')) > 0)
    {
      if(fParsed = ParseSSATag(sub, str.Mid(1, i-1), stss, orgstss))
        str = str.Mid(i+1);
    }
    else if(str[0] == '<' && (i = str.Find(L'>')) > 0)
    {
      if(fParsed = ParseHtmlTag(sub, str.Mid(1, i-1), stss, orgstss))
        str = str.Mid(i+1);
    }

    if(fParsed)
    {
      i = str.FindOneOf(L"{<");
      if(i < 0) i = str.GetLength();
      if(i == 0) continue;
    }
    else
    {
      i = str.Mid(1).FindOneOf(L"{<");
      if(i < 0) i = str.GetLength()-1;
      i++;
    }

    STSStyle tmp = stss;

    tmp.fontSize = sub->m_scaley*tmp.fontSize*64;
    tmp.fontSpacing = sub->m_scalex*tmp.fontSpacing*64;
    tmp.outlineWidthX *= (m_fScaledBAS ? sub->m_scalex : 1) * 8;
    tmp.outlineWidthY *= (m_fScaledBAS ? sub->m_scaley : 1) * 8;
    tmp.shadowDepthX *= (m_fScaledBAS ? sub->m_scalex : 1) * 8;
    tmp.shadowDepthY *= (m_fScaledBAS ? sub->m_scaley : 1) * 8;

    if(m_nPolygon)
    {
      ParsePolygon(sub, str.Left(i), tmp);
    }
    else
    {
      ParseString(sub, str.Left(i), tmp);
    }

    str = str.Mid(i);
  }

  // just a "work-around" solution... in most cases nobody will want to use \org together with moving but without rotating the subs
  if(sub->m_effects[EF_ORG] && (sub->m_effects[EF_MOVE] || sub->m_effects[EF_BANNER] || sub->m_effects[EF_SCROLL]))
    sub->m_fAnimated = true;

  sub->m_scrAlignment = abs(sub->m_scrAlignment);

  STSEntry stse = at(entry);
  Com::SmartRect marginRect = stse.marginRect;
  if(marginRect.left == 0) marginRect.left = orgstss.marginRect.left;
  if(marginRect.top == 0) marginRect.top = orgstss.marginRect.top;
  if(marginRect.right == 0) marginRect.right = orgstss.marginRect.right;
  if(marginRect.bottom == 0) marginRect.bottom = orgstss.marginRect.bottom;
  marginRect.left = (int)(sub->m_scalex*marginRect.left*8);
  marginRect.top = (int)(sub->m_scaley*marginRect.top*8);
  marginRect.right = (int)(sub->m_scalex*marginRect.right*8);
  marginRect.bottom = (int)(sub->m_scaley*marginRect.bottom*8);
  
  if(stss.relativeTo == 1)
  {
    marginRect.left += m_vidrect.left;
    marginRect.top += m_vidrect.top;
    marginRect.right += m_size.cx - m_vidrect.right;
    marginRect.bottom += m_size.cy - m_vidrect.bottom;
  }

  sub->CreateClippers(m_size);

  sub->MakeLines(m_size, marginRect);

  m_subtitleCache[entry] = sub;

  return(sub);
}

//

STDMETHODIMP CRenderedTextSubtitle::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);
    *ppv = NULL;

    return 
    QI(IPersist)
    QI(ISubStream)
    QI(ISubPicProvider)
    __super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicProvider

STDMETHODIMP_(__w64 int) CRenderedTextSubtitle::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  int iSegment = -1;
  SearchSubs((int)(rt/10000), fps, &iSegment, NULL);
  
  if(iSegment < 0) iSegment = 0;

  return(GetNext(iSegment));
}

STDMETHODIMP_(int) CRenderedTextSubtitle::GetNext(int pos)
{
  int iSegment = (int)pos;

  const STSSegment* stss;
  while((stss = GetSegment(iSegment)) && stss->subs.size() == 0)
    iSegment++;

  return(stss ? (iSegment+1) : NULL);
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedTextSubtitle::GetStart(int pos, double fps)
{
  return(10000i64 * TranslateSegmentStart((int)pos-1, fps));
}

STDMETHODIMP_(REFERENCE_TIME) CRenderedTextSubtitle::GetStop(int pos, double fps)
{
  return(10000i64 * TranslateSegmentEnd((int)pos-1, fps));
}

STDMETHODIMP_(bool) CRenderedTextSubtitle::IsAnimated(int pos)
{
  return(true);
}

struct LSub
{
    int idx, layer, readorder;
};

static bool lscomp(const LSub& ls1, const LSub& ls2)
{
  bool ret = ls1.layer == ls2.layer; // true if !=
  if (ret)
    return ls1.readorder < ls2.readorder;
  return ls1.layer == ls2.layer;
}

STDMETHODIMP CRenderedTextSubtitle::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
{
  Com::SmartRect bbox2(0,0,0,0);

  if(m_size != Com::SmartSize(spd.w*8, spd.h*8) || m_vidrect != Com::SmartRect(spd.vidrect.left*8, spd.vidrect.top*8, spd.vidrect.right*8, spd.vidrect.bottom*8))
    Init(Com::SmartSize(spd.w, spd.h), spd.vidrect);

  int t = (int)(rt / 10000);

  int segment;
  const STSSegment* stss = SearchSubs(t, fps, &segment);
  if(!stss) return S_FALSE;

  // clear any cached subs not in the range of +/-30secs measured from the segment's bounds
  {
    std::map<int, CSubtitle*>::iterator it = m_subtitleCache.begin();
    while (it != m_subtitleCache.end())
    {
      int key = it->first;
      CSubtitle* val = (*it).second; it++;

      STSEntry& stse = at(key);
      if(stse.end <= (t-30000) || stse.start > (t+30000)) 
      {
        delete val;
        m_subtitleCache.erase( --it );
        it = m_subtitleCache.begin();
      }
    }
  }

  m_sla.AdvanceToSegment(segment, stss->subs);

  std::vector<LSub> subs;

  for(int i = 0, j = stss->subs.size(); i < j; i++)
  {
    LSub ls;
    ls.idx = stss->subs[i];
    ls.layer = at(stss->subs[i]).layer;
    ls.readorder = at(stss->subs[i]).readorder;
    subs.push_back(ls);
  }

  std::sort(subs.begin(), subs.end(), lscomp);
  //qsort(subs.GetData(), subs.size(), sizeof(LSub), lscomp);

  for(int i = 0, j = subs.size(); i < j; i++)
  {
    int entry = subs[i].idx;

    STSEntry stse = at(entry);
#if RTSTraceLevel > 1
    TRACE(L"Rendering subtitle: %s", stse.str.c_str());
#endif

    {
      int start = TranslateStart(entry, fps);
      m_time = t - start;
      m_delay = TranslateEnd(entry, fps) - start;
    }

    CSubtitle* s = GetSubtitle(entry);
    if(!s) continue;

    Com::SmartRect clipRect = s->m_clip;
    Com::SmartRect r = s->m_rect;
    Com::SmartSize spaceNeeded = r.Size();

    // apply the effects

    bool fPosOverride = false, fOrgOverride = false;

    int alpha = 0x00;

    Com::SmartPoint org2;

    BYTE* pAlphaMask = s->m_pClipper?s->m_pClipper->m_pAlphaMask:NULL;

#ifdef _VSMOD // patch m006. moveable vector clip
    MOD_MOVEVC mod_vc;
    mod_vc.spd = CSize(spd.w,spd.h);
    //mod_vc.alphamask = pAlphaMask;
    mod_vc.size = s->m_pClipper?s->m_pClipper->m_size : CSize(0,0);
#endif

    for(int k = 0; k < EF_NUMBEROFEFFECTS; k++)
    {
      if(!s->m_effects[k]) continue;

      switch(k)
      {
      case EF_MOVE: // {\move(x1=param[0], y1=param[1], x2=param[2], y2=param[3], t1=t[0], t2=t[1])}
        {
          Com::SmartPoint p;
#ifdef _VSMOD // patch m005. add some move types
        if (s->m_effects[k]->param[0]==0)
          {
            Com::SmartPoint p1(s->m_effects[k]->param[1], s->m_effects[k]->param[2]);
            Com::SmartPoint p2(s->m_effects[k]->param[3], s->m_effects[k]->param[4]);
#else
            Com::SmartPoint p1(s->m_effects[k]->param[0], s->m_effects[k]->param[1]);
            Com::SmartPoint p2(s->m_effects[k]->param[2], s->m_effects[k]->param[3]);
#endif
          int t1 = s->m_effects[k]->t[0];
          int t2 = s->m_effects[k]->t[1];

          if(t2 < t1) {int t = t1; t1 = t2; t2 = t;}

          if(t1 <= 0 && t2 <= 0) {t1 = 0; t2 = m_delay;}

          if(m_time <= t1) p = p1;
          else if (p1 == p2) p = p1;
          else if(t1 < m_time && m_time < t2)
          {
            double t = 1.0*(m_time-t1)/(t2-t1);
            p.x = (int)((1-t)*p1.x + t*p2.x);
            p.y = (int)((1-t)*p1.y + t*p2.y);
          }
          else p = p2;
#ifdef _VSMOD // patch m005. add some move types
          }
          else if (s->m_effects[k]->param[0]==1) // radial move
          {
            CPoint p1(s->m_effects[k]->param[1], s->m_effects[k]->param[2]); // p0
            CPoint p2(s->m_effects[k]->param[3], s->m_effects[k]->param[4]); // p1
            CPoint r0(s->m_effects[k]->param[7], s->m_effects[k]->param[8]);
            double alp1 = s->m_effects[k]->param[5]*PI/1800000;  // alp1
            double alp2 = s->m_effects[k]->param[6]*PI/1800000;  // alp2
            int t1 = s->m_effects[k]->t[0];
            int t2 = s->m_effects[k]->t[1];

            CPoint pr1 = (p1.x + cos(alp1)*r0.x, p1.y + sin(alp1)*r0.x);
            CPoint pr2 = (p2.x + cos(alp2)*r0.y, p2.y + sin(alp2)*r0.y);

                        if(t2 < t1)
                        {
                            int t = t1;
                            t1 = t2;
                            t2 = t;
                        }

                        if(t1 <= 0 && t2 <= 0)
                        {
                            t1 = 0;
                            t2 = m_delay;
                        }

            if(m_time <= t1) p = pr1;
            //else if (p1 == p2) p = pr1; // jfs: avoid rounding error problems sometimes causing subtitles with \pos to jump around a bit
            else if((t1 < m_time) && (m_time < t2))
            {
              double t = (double)(m_time-t1)/(t2-t1);
              double alp = ((1-t)*alp1 + t*alp2);
              double rt = ((1-t)*r0.x + t*r0.y);

              p.x = (int)((1-t)*p1.x + t*p2.x); // origin point
              p.y = (int)((1-t)*p1.y + t*p2.y);
              p.x += (int)(cos(alp)*rt);
              p.y -= (int)(sin(alp)*rt);
            }
            else p = pr2;
          }
          else if (s->m_effects[k]->param[0]==2) // square spline
          {
            CPoint p1(s->m_effects[k]->param[1], s->m_effects[k]->param[2]);
            CPoint p2(s->m_effects[k]->param[3], s->m_effects[k]->param[4]);
            CPoint p3(s->m_effects[k]->param[5], s->m_effects[k]->param[6]);

            int t1 = s->m_effects[k]->t[0];
            int t2 = s->m_effects[k]->t[1];

                        if(t2 < t1)
                        {
                            int t = t1;
                            t1 = t2;
                            t2 = t;
                        }

                        if(t1 <= 0 && t2 <= 0)
                        {
                            t1 = 0;
                            t2 = m_delay;
                        }

            if(m_time <= t1) p = p1;
            else if (p1 == p2) p = p1; // jfs: avoid rounding error problems sometimes causing subtitles with \pos to jump around a bit
            else if(t1 < m_time && m_time < t2)
            {
              double t = (double)(m_time-t1)/(t2-t1);
              p.x = (int)((1-t)*(1-t)*p1.x + 2*t*(1-t)*p2.x + t*t*p3.x);
              p.y = (int)((1-t)*(1-t)*p1.y + 2*t*(1-t)*p2.y + t*t*p3.y);
            }
            else p = p3;
          }
          else if (s->m_effects[k]->param[0]==3) // cubic spline
          {
            CPoint p1(s->m_effects[k]->param[1], s->m_effects[k]->param[2]);
            CPoint p2(s->m_effects[k]->param[3], s->m_effects[k]->param[4]);
            CPoint p3(s->m_effects[k]->param[5], s->m_effects[k]->param[6]);
            CPoint p4(s->m_effects[k]->param[7], s->m_effects[k]->param[8]);

            int t1 = s->m_effects[k]->t[0];
            int t2 = s->m_effects[k]->t[1];

                        if(t2 < t1)
                        {
                            int t = t1;
                            t1 = t2;
                            t2 = t;
                        }

                        if(t1 <= 0 && t2 <= 0)
                        {
                            t1 = 0;
                            t2 = m_delay;
                        }

            if(m_time <= t1) p = p1;
            else if (p1 == p2) p = p1; // jfs: avoid rounding error problems sometimes causing subtitles with \pos to jump around a bit
            else if(t1 < m_time && m_time < t2)
            {
              double t = (double)(m_time-t1)/(t2-t1);
              p.x = (int)((1-t)*(1-t)*(1-t)*p1.x + 3*t*(1-t)*(1-t)*p2.x + 3*t*t*(1-t)*p3.x + t*t*t*p4.x);
              p.y = (int)((1-t)*(1-t)*(1-t)*p1.y + 3*t*(1-t)*(1-t)*p2.y + 3*t*t*(1-t)*p3.y + t*t*t*p4.y);
            }
            else p = p4;
          }
#endif
          r = Com::SmartRect(
              Com::SmartPoint((s->m_scrAlignment%3) == 1 ? p.x : (s->m_scrAlignment%3) == 0 ? p.x - spaceNeeded.cx : p.x - (spaceNeeded.cx+1)/2,
                  s->m_scrAlignment <= 3 ? p.y - spaceNeeded.cy : s->m_scrAlignment <= 6 ? p.y - (spaceNeeded.cy+1)/2 : p.y),
              spaceNeeded);

          if(s->m_relativeTo == 1)
            r.OffsetRect(m_vidrect.TopLeft());
          fPosOverride = true;
        }
        break;
      case EF_ORG: // {\org(x=param[0], y=param[1])}
        {
#ifdef _VSMOD // patch f003. moving \org for some karaoke effects
          CPoint orgA(s->m_effects[k]->param[0], s->m_effects[k]->param[1]);
          CPoint orgB(s->m_effects[k]->param[2], s->m_effects[k]->param[3]);
          int to1 = s->m_effects[k]->t[0];
          int to2 = s->m_effects[k]->t[1];
 
          if(to2 < to1)
          {
              int to = to1;
              to1 = to2;
              to2 = to;
          }

          if(to1 <= 0 && to2 <= 0)
          {
              to1 = 0;
              to2 = m_delay;
          }

          if(m_time <= to1) org2 = orgA;
          else if (to1 == to2) org2 = orgA; // jfs: avoid rounding error problems sometimes causing subtitles with \pos to jump around a bit
          else if(to1 < m_time && m_time < to2)
          {
            double t = 1.0*(m_time-to1)/(to2-to1);
            org2.x = (int)((1-t)*orgA.x + t*orgB.x);
            org2.y = (int)((1-t)*orgA.y + t*orgB.y);
          }
          else org2 = orgB;
#else
          org2 = Com::SmartPoint(s->m_effects[k]->param[0], s->m_effects[k]->param[1]);
#endif
          fOrgOverride = true;
        }
        break;
      case EF_FADE: // {\fade(a1=param[0], a2=param[1], a3=param[2], t1=t[0], t2=t[1], t3=t[2], t4=t[3]) or {\fad(t1=t[1], t2=t[2])
        {
          int t1 = s->m_effects[k]->t[0];
          int t2 = s->m_effects[k]->t[1];
          int t3 = s->m_effects[k]->t[2];
          int t4 = s->m_effects[k]->t[3];

          if(t1 == -1 && t4 == -1)
          {
              t1 = 0;
              t3 = m_delay - t3;
              t4 = m_delay;
          }

          if(m_time < t1) alpha = s->m_effects[k]->param[0];
          else if(m_time >= t1 && m_time < t2)
          {
            double t = 1.0 * (m_time - t1) / (t2 - t1);
            alpha = (int)(s->m_effects[k]->param[0]*(1-t) + s->m_effects[k]->param[1]*t);
          }
          else if(m_time >= t2 && m_time < t3) alpha = s->m_effects[k]->param[1];
          else if(m_time >= t3 && m_time < t4)
          {
            double t = 1.0 * (m_time - t3) / (t4 - t3);
            alpha = (int)(s->m_effects[k]->param[1]*(1-t) + s->m_effects[k]->param[2]*t);
          }
          else if(m_time >= t4) alpha = s->m_effects[k]->param[2];
        }
        break;
      case EF_BANNER: // Banner;delay=param[0][;leftoright=param[1];fadeawaywidth=param[2]]
        {
          int left = s->m_relativeTo == 1 ? m_vidrect.left : 0, 
            right = s->m_relativeTo == 1 ? m_vidrect.right : m_size.cx;

          r.left = !!s->m_effects[k]->param[1] 
            ? (left/*marginRect.left*/ - spaceNeeded.cx) + (int)(m_time*8.0/s->m_effects[k]->param[0])
            : (right /*- marginRect.right*/) - (int)(m_time*8.0/s->m_effects[k]->param[0]);

          r.right = r.left + spaceNeeded.cx;

          clipRect &= Com::SmartRect(left>>3, clipRect.top, right>>3, clipRect.bottom);

          fPosOverride = true;
        }
        break;
      case EF_SCROLL: // Scroll up/down(toptobottom=param[3]);top=param[0];bottom=param[1];delay=param[2][;fadeawayheight=param[4]]
        {
          r.top = !!s->m_effects[k]->param[3]
            ? s->m_effects[k]->param[0] + (int)(m_time*8.0/s->m_effects[k]->param[2]) - spaceNeeded.cy
            : s->m_effects[k]->param[1] - (int)(m_time*8.0/s->m_effects[k]->param[2]);

          r.bottom = r.top + spaceNeeded.cy;

          Com::SmartRect cr(0, (s->m_effects[k]->param[0] + 4) >> 3, spd.w, (s->m_effects[k]->param[1] + 4) >> 3);

          if(s->m_relativeTo == 1)
            r.top += m_vidrect.top, 
            r.bottom += m_vidrect.top, 
            cr.top += m_vidrect.top>>3, 
            cr.bottom += m_vidrect.top>>3;

          clipRect &= cr;

          fPosOverride = true;
        }
#ifdef _VSMOD // patch m006. moveable vector clip
        case EF_VECTCLP:
        {
          CPoint vcpos1(s->m_effects[k]->param[0], s->m_effects[k]->param[1]);
          CPoint vcpos2(s->m_effects[k]->param[2], s->m_effects[k]->param[3]);
          int to1 = s->m_effects[k]->t[0];
          int to2 = s->m_effects[k]->t[1];
 
          if(to2 < to1)
          {
              int to = to1;
              to1 = to2;
              to2 = to;
          }

          if(to1 <= 0 && to2 <= 0)
          {
              to1 = 0;
              to2 = m_delay;
          }

          if(m_time <= to1) mod_vc.pos = vcpos1;
          else if (to1 == to2) mod_vc.pos = vcpos1; // jfs: avoid rounding error problems sometimes causing subtitles with \pos to jump around a bit
          else if(to1 < m_time && m_time < to2)
          {
            double t = 1.0*(m_time-to1)/(to2-to1);
            mod_vc.pos.x = (int)((1-t)*vcpos1.x + t*vcpos2.x);
            mod_vc.pos.y = (int)((1-t)*vcpos1.y + t*vcpos2.y);
          }
          else mod_vc.pos = vcpos2;
          mod_vc.enable = true;
        }
#endif
        break;
      default:
        break;
      }
    }

    if(!fPosOverride && !fOrgOverride && !s->m_fAnimated) 
      r = m_sla.AllocRect(s, segment, entry, stse.layer, m_collisions);

    Com::SmartPoint org;
    org.x = (s->m_scrAlignment%3) == 1 ? r.left : (s->m_scrAlignment%3) == 2 ? r.CenterPoint().x : r.right;
    org.y = s->m_scrAlignment <= 3 ? r.bottom : s->m_scrAlignment <= 6 ? r.CenterPoint().y : r.top;

    if(!fOrgOverride) org2 = org;

    Com::SmartPoint p, p2(0, r.top);

    CSubtitle::iterator it;

    p = p2;

    // Rectangles for inverse clip
    Com::SmartRect iclipRect[4];
    iclipRect[0] = Com::SmartRect(0, 0, spd.w, clipRect.top);
    iclipRect[1] = Com::SmartRect(0, clipRect.top, clipRect.left, clipRect.bottom);
    iclipRect[2] = Com::SmartRect(clipRect.right, clipRect.top, spd.w, clipRect.bottom);
    iclipRect[3] = Com::SmartRect(0, clipRect.bottom, spd.w, spd.h);

    it = s->begin();
    for(; it != s->end(); ++it) 
    {
      CLine* l = *it;

      p.x = (s->m_scrAlignment%3) == 1 ? org.x
        : (s->m_scrAlignment%3) == 0 ? org.x - l->m_width
        :                 org.x - (l->m_width/2);

#ifdef _VSMOD // patch m006. moveable vector clip
      if (s->m_clipInverse)
      {
        bbox2 |= l->PaintShadow(spd, iclipRect[0], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintShadow(spd, iclipRect[1], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintShadow(spd, iclipRect[2], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintShadow(spd, iclipRect[3], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
      }
      else
      {
        bbox2 |= l->PaintShadow(spd, clipRect, pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
      }
#else
      if (s->m_clipInverse)
      {
        bbox2 |= l->PaintShadow(spd, iclipRect[0], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintShadow(spd, iclipRect[1], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintShadow(spd, iclipRect[2], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintShadow(spd, iclipRect[3], pAlphaMask, p, org2, m_time, alpha);
      }
      else
      {
        bbox2 |= l->PaintShadow(spd, clipRect, pAlphaMask, p, org2, m_time, alpha);
      }
#endif
      p.y += l->m_ascent + l->m_descent;
    }

    p = p2;

    it = s->begin();
    for(; it != s->end(); ++it) 
    {
      CLine* l = *it;

      p.x = (s->m_scrAlignment%3) == 1 ? org.x
        : (s->m_scrAlignment%3) == 0 ? org.x - l->m_width
        :                 org.x - (l->m_width/2);

#ifdef _VSMOD // patch m006. movable vector clip
      if (s->m_clipInverse)
      {
        bbox2 |= l->PaintOutline(spd, iclipRect[0], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintOutline(spd, iclipRect[1], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintOutline(spd, iclipRect[2], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintOutline(spd, iclipRect[3], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
      }
      else
      {
        bbox2 |= l->PaintOutline(spd, clipRect, pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
      }
#else
      if (s->m_clipInverse)
      {
        bbox2 |= l->PaintOutline(spd, iclipRect[0], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintOutline(spd, iclipRect[1], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintOutline(spd, iclipRect[2], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintOutline(spd, iclipRect[3], pAlphaMask, p, org2, m_time, alpha);
      }
      else
      {
        bbox2 |= l->PaintOutline(spd, clipRect, pAlphaMask, p, org2, m_time, alpha);
      }
#endif
      p.y += l->m_ascent + l->m_descent;
    }

    p = p2;

    it = s->begin();
    for(; it != s->end(); ++it) 
    {
      CLine* l = *it;

      p.x = (s->m_scrAlignment%3) == 1 ? org.x
        : (s->m_scrAlignment%3) == 0 ? org.x - l->m_width
        :                 org.x - (l->m_width/2);

#ifdef _VSMOD // patch m006. moveable vector clip
      if (s->m_clipInverse)
      {
        bbox2 |= l->PaintBody(spd, iclipRect[0], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintBody(spd, iclipRect[1], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintBody(spd, iclipRect[2], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
        bbox2 |= l->PaintBody(spd, iclipRect[3], pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
      }
      else
      {
        bbox2 |= l->PaintBody(spd, clipRect, pAlphaMask, p, org2, m_time, alpha, mod_vc, rt);
      }
#else
      if (s->m_clipInverse)
      {
        bbox2 |= l->PaintBody(spd, iclipRect[0], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintBody(spd, iclipRect[1], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintBody(spd, iclipRect[2], pAlphaMask, p, org2, m_time, alpha);
        bbox2 |= l->PaintBody(spd, iclipRect[3], pAlphaMask, p, org2, m_time, alpha);
      }
      else
      {
        bbox2 |= l->PaintBody(spd, clipRect, pAlphaMask, p, org2, m_time, alpha);
      }
#endif
      p.y += l->m_ascent + l->m_descent;
    }
  }

  bbox = bbox2;

  return (subs.size() && !bbox2.IsRectEmpty()) ? S_OK : S_FALSE;
}

// IPersist

STDMETHODIMP CRenderedTextSubtitle::GetClassID(CLSID* pClassID)
{
  return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
}

// ISubStream

STDMETHODIMP_(int) CRenderedTextSubtitle::GetStreamCount()
{
  return(1);
}

STDMETHODIMP CRenderedTextSubtitle::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
{
  USES_CONVERSION;
  if(iStream != 0) return E_INVALIDARG;

  if(ppName)
  {
    if(!(*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR))))
      return E_OUTOFMEMORY;

    wcscpy(*ppName, CStdStringW(m_name));

    if(pLCID)
    {
      *pLCID = ISO6391ToLcid (CStdStringA(*ppName).c_str());
      if (*pLCID == 0)
        *pLCID = ISO6392ToLcid (CStdStringA(*ppName).c_str());
    }
  }

  return S_OK;
}

STDMETHODIMP_(int) CRenderedTextSubtitle::GetStream()
{
  return(0);
}

STDMETHODIMP CRenderedTextSubtitle::SetStream(int iStream)
{
  return iStream == 0 ? S_OK : E_FAIL;
}

STDMETHODIMP CRenderedTextSubtitle::Reload()
{
  if(!ATL::CFile::FileExists(m_path)) return E_FAIL;
  return !m_path.IsEmpty() && Open(m_path, DEFAULT_CHARSET) ? S_OK : E_FAIL;
}