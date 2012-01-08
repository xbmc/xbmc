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
#include "MemSubPic.h"

// For CPUID usage
#include "../dsutil/vd.h"
#include <emmintrin.h>

// color conv

unsigned char Clip_base[256*3];
unsigned char* Clip = Clip_base + 256;

const int c2y_cyb = int(0.114*219/255*65536+0.5);
const int c2y_cyg = int(0.587*219/255*65536+0.5);
const int c2y_cyr = int(0.299*219/255*65536+0.5);
const int c2y_cu = int(1.0/2.018*1024+0.5);
const int c2y_cv = int(1.0/1.596*1024+0.5);

int c2y_yb[256];
int c2y_yg[256];
int c2y_yr[256];

const int y2c_cbu = int(2.018*65536+0.5);
const int y2c_cgu = int(0.391*65536+0.5);
const int y2c_cgv = int(0.813*65536+0.5);
const int y2c_crv = int(1.596*65536+0.5);
int y2c_bu[256];
int y2c_gu[256];
int y2c_gv[256];
int y2c_rv[256];

const int cy_cy = int(255.0/219.0*65536+0.5);
const int cy_cy2 = int(255.0/219.0*32768+0.5);

bool fColorConvInitOK = false;

void ColorConvInit()
{
  if(fColorConvInitOK) return;

  int i;

  for(i = 0; i < 256; i++) 
  {
    Clip_base[i] = 0;
    Clip_base[i+256] = i;
    Clip_base[i+512] = 255;
  }

  for(i = 0; i < 256; i++)
  {
    c2y_yb[i] = c2y_cyb*i;
    c2y_yg[i] = c2y_cyg*i;
    c2y_yr[i] = c2y_cyr*i;

    y2c_bu[i] = y2c_cbu*(i-128);
    y2c_gu[i] = y2c_cgu*(i-128);
    y2c_gv[i] = y2c_cgv*(i-128);
    y2c_rv[i] = y2c_crv*(i-128);
  }

  fColorConvInitOK = true;
}

#define rgb2yuv(r1,g1,b1,r2,g2,b2) \
  int y1 = (c2y_yb[b1] + c2y_yg[g1] + c2y_yr[r1] + 0x108000) >> 16; \
  int y2 = (c2y_yb[b2] + c2y_yg[g2] + c2y_yr[r2] + 0x108000) >> 16; \
\
  int scaled_y = (y1+y2-32) * cy_cy2; \
\
  unsigned char u = Clip[(((((b1+b2)<<15) - scaled_y) >> 10) * c2y_cu + 0x800000 + 0x8000) >> 16]; \
  unsigned char v = Clip[(((((r1+r2)<<15) - scaled_y) >> 10) * c2y_cv + 0x800000 + 0x8000) >> 16]; \

//
// CMemSubPic
//

CMemSubPic::CMemSubPic(SubPicDesc& spd)
  : m_spd(spd)
{
  m_maxsize.SetSize(spd.w, spd.h);
  m_rcDirty.SetRect(0, 0, spd.w, spd.h);
}

CMemSubPic::~CMemSubPic()
{
  delete [] m_spd.bits, m_spd.bits = NULL;
}

// ISubPic

STDMETHODIMP_(void*) CMemSubPic::GetObject()
{
  return (void*)&m_spd;
}

STDMETHODIMP CMemSubPic::GetDesc(SubPicDesc& spd)
{
  spd.type = m_spd.type;
  spd.w = m_size.cx;
  spd.h = m_size.cy;
  spd.bpp = m_spd.bpp;
  spd.pitch = m_spd.pitch;
  spd.bits = m_spd.bits;
  spd.bitsU = m_spd.bitsU;
  spd.bitsV = m_spd.bitsV;
  spd.vidrect = m_vidrect;

  return S_OK;
}

STDMETHODIMP CMemSubPic::CopyTo(ISubPic* pSubPic)
{
  HRESULT hr;
  if(FAILED(hr = __super::CopyTo(pSubPic)))
    return hr;

  SubPicDesc src, dst;
  if(FAILED(GetDesc(src)) || FAILED(pSubPic->GetDesc(dst)))
    return E_FAIL;

  int w = m_rcDirty.Width(), h = m_rcDirty.Height();

  BYTE* s = (BYTE*)src.bits + src.pitch*m_rcDirty.top + m_rcDirty.left*4;
  BYTE* d = (BYTE*)dst.bits + dst.pitch*m_rcDirty.top + m_rcDirty.left*4;

  for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
    memcpy(d, s, w*4);

  return S_OK;
}

STDMETHODIMP CMemSubPic::ClearDirtyRect(DWORD color)
{
  if(m_rcDirty.IsRectEmpty())
    return S_FALSE;

  BYTE* p = (BYTE*)m_spd.bits + m_spd.pitch*m_rcDirty.top + m_rcDirty.left*(m_spd.bpp>>3);
  for(ptrdiff_t j = 0, h = m_rcDirty.Height(); j < h; j++, p += m_spd.pitch)
  {
    int w = m_rcDirty.Width();
#ifdef _WIN64
    memsetd(p, color, w*4); // nya
#else
    __asm
    {
      mov eax, color
      mov ecx, w
      mov edi, p
      cld
      rep stosd
    }
#endif
  }
  
  m_rcDirty.SetRectEmpty();

  return S_OK;
}

STDMETHODIMP CMemSubPic::Lock(SubPicDesc& spd)
{
  return GetDesc(spd);
}

STDMETHODIMP CMemSubPic::Unlock(RECT* pDirtyRect)
{
  m_rcDirty = pDirtyRect ? *pDirtyRect : Com::SmartRect(0,0,m_spd.w,m_spd.h);

  if(m_rcDirty.IsRectEmpty())
    return S_OK;
  
  if(m_spd.type == MSP_YUY2 || m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV || m_spd.type == MSP_AYUV)
  {
    ColorConvInit();

    if(m_spd.type == MSP_YUY2 || m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV)
    {
      m_rcDirty.left &= ~1;
      m_rcDirty.right = (m_rcDirty.right+1)&~1;

      if(m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV)
      {
        m_rcDirty.top &= ~1;
        m_rcDirty.bottom = (m_rcDirty.bottom+1)&~1;
      }
    }
  }

  int w = m_rcDirty.Width(), h = m_rcDirty.Height();

  BYTE* top = (BYTE*)m_spd.bits + m_spd.pitch*m_rcDirty.top + m_rcDirty.left*4;
  BYTE* bottom = top + m_spd.pitch*h;

  if(m_spd.type == MSP_RGB16)
  {
    for(; top < bottom ; top += m_spd.pitch)
    {
      DWORD* s = (DWORD*)top;
      DWORD* e = s + w;
      for(; s < e; s++)
      {
        *s = ((*s>>3)&0x1f000000)|((*s>>8)&0xf800)|((*s>>5)&0x07e0)|((*s>>3)&0x001f);
      }
    }
  }
  else if(m_spd.type == MSP_RGB15)
  {
    for(; top < bottom; top += m_spd.pitch)
    {
      DWORD* s = (DWORD*)top;
      DWORD* e = s + w;
      for(; s < e; s++)
      {
        *s = ((*s>>3)&0x1f000000)|((*s>>9)&0x7c00)|((*s>>6)&0x03e0)|((*s>>3)&0x001f);
      }
    }
  }
  else if(m_spd.type == MSP_YUY2 || m_spd.type == MSP_YV12 || m_spd.type == MSP_IYUV)
  {
    for(; top < bottom ; top += m_spd.pitch)
    {
      BYTE* s = top;
      BYTE* e = s + w*4;
      for(; s < e; s+=8) // ARGB ARGB -> AxYU AxYV
      {
        if((s[3]+s[7]) < 0x1fe)
        {
          s[1] = (c2y_yb[s[0]] + c2y_yg[s[1]] + c2y_yr[s[2]] + 0x108000) >> 16;
          s[5] = (c2y_yb[s[4]] + c2y_yg[s[5]] + c2y_yr[s[6]] + 0x108000) >> 16;

          int scaled_y = (s[1]+s[5]-32) * cy_cy2;
          
          s[0] = Clip[(((((s[0]+s[4])<<15) - scaled_y) >> 10) * c2y_cu + 0x800000 + 0x8000) >> 16];
          s[4] = Clip[(((((s[2]+s[6])<<15) - scaled_y) >> 10) * c2y_cv + 0x800000 + 0x8000) >> 16];
        }
        else
        {
          s[1] = s[5] = 0x10;
          s[0] = s[4] = 0x80;
        }
      }
    }
  }
  else if(m_spd.type == MSP_AYUV)
  {
    for(; top < bottom ; top += m_spd.pitch)
    {
      BYTE* s = top;
      BYTE* e = s + w*4;
      for(; s < e; s+=4) // ARGB -> AYUV
      {
        if(s[3] < 0xff)
        {
          int y = (c2y_yb[s[0]] + c2y_yg[s[1]] + c2y_yr[s[2]] + 0x108000) >> 16;
          int scaled_y = (y-32) * cy_cy;
          s[1] = Clip[((((s[0]<<16) - scaled_y) >> 10) * c2y_cu + 0x800000 + 0x8000) >> 16];
          s[0] = Clip[((((s[2]<<16) - scaled_y) >> 10) * c2y_cv + 0x800000 + 0x8000) >> 16];
          s[2] = y;
        }
        else
        {
          s[0] = s[1] = 0x80;
          s[2] = 0x10;
        }
      }
    }
  }

  return S_OK;
}

#ifdef _WIN64
void AlphaBlt_YUY2_SSE2(int w, int h, BYTE* d, int dstpitch, BYTE* s, int srcpitch)
{
  unsigned int ia, c;
  DWORD* d2 = (DWORD*)d;

  BYTE* s2 = s;
  BYTE* s2end = s2 + w*4;
  static const __int64 _8181 = 0x0080001000800010i64;

  for(ptrdiff_t j = 0; j < h; j++, s += srcpitch, d += dstpitch)
  {  
    for(; s2 < s2end; s2 += 8, d2++)
    {
      ia = (s2[3]+s2[7])>>1;
      if(ia < 0xff)
      {
        c = (s2[4]<<24)|(s2[5]<<16)|(s2[0]<<8)|s2[1]; // (v<<24)|(y2<<16)|(u<<8)|y1;

        ia = (ia<<24)|(s2[7]<<16)|(ia<<8)|s2[3];
        // SSE2
        __m128i mm_zero = _mm_setzero_si128();
        __m128i mm_8181 = _mm_move_epi64(_mm_cvtsi64_si128(_8181));
        __m128i mm_c = _mm_cvtsi32_si128(c);
        mm_c = _mm_unpacklo_epi8(mm_c, mm_zero);
        __m128i mm_d = _mm_cvtsi32_si128(*d2);
        mm_d = _mm_unpacklo_epi8(mm_d, mm_zero);
        __m128i mm_a = _mm_cvtsi32_si128(ia);
        mm_a = _mm_unpacklo_epi8(mm_a, mm_zero);
        mm_a = _mm_srli_epi16(mm_a,1);
        mm_d = _mm_sub_epi16(mm_d,mm_8181);
        mm_d = _mm_mullo_epi16(mm_d,mm_a);
        mm_d = _mm_srai_epi16(mm_d,7);
        mm_d = _mm_adds_epi16(mm_d,mm_c);
        mm_d = _mm_packus_epi16(mm_d,mm_d);
        *d2 = (DWORD)_mm_cvtsi128_si32(mm_d);
      }
    }
  }
}
#endif

#ifndef _WIN64
void AlphaBlt_YUY2_MMX(int w, int h, BYTE* d, int dstpitch, BYTE* s, int srcpitch)
{
  unsigned int ia, c;
  DWORD* d2 = (DWORD*)d;

  BYTE* s2 = s;
  BYTE* s2end = s2 + w*4;
  static const __int64 _8181 = 0x0080001000800010i64;


  for(ptrdiff_t j = 0; j < h; j++, s += srcpitch, d += dstpitch)
  {  
    for(; s2 < s2end; s2 += 8, d2++)
    {
      ia = (s2[3]+s2[7])>>1;
      if(ia < 0xff)
      {
        c = (s2[4]<<24)|(s2[5]<<16)|(s2[0]<<8)|s2[1]; // (v<<24)|(y2<<16)|(u<<8)|y1;
        ia = (ia<<24)|(s2[7]<<16)|(ia<<8)|s2[3];
        __asm
        {
          mov      esi, s2
          mov      edi, d2
          pxor    mm0, mm0
          movq    mm1, _8181
          movd    mm2, c
          punpcklbw  mm2, mm0
          movd    mm3, [edi]
          punpcklbw  mm3, mm0
          movd    mm4, ia
          punpcklbw  mm4, mm0
          psrlw    mm4, 1
          psubsw    mm3, mm1
          pmullw    mm3, mm4
          psraw    mm3, 7
          paddsw    mm3, mm2
          packuswb  mm3, mm3
          movd    [edi], mm3
        };
      }
    }
  }
}
#endif

void AlphaBlt_YUY2_C(int w, int h, BYTE* d, int dstpitch, BYTE* s, int srcpitch)
{
  unsigned int ia, c;
  DWORD* d2 = (DWORD*)d;

  BYTE* s2 = s;
  BYTE* s2end = s2 + w*4;
  static const __int64 _8181 = 0x0080001000800010i64;

  for(ptrdiff_t j = 0; j < h; j++, s += srcpitch, d += dstpitch)
  {      
    for(; s2 < s2end; s2 += 8, d2++)
    {
      ia = (s2[3]+s2[7])>>1;
      if(ia < 0xff)
      {
        c = (s2[4]<<24)|(s2[5]<<16)|(s2[0]<<8)|s2[1]; // (v<<24)|(y2<<16)|(u<<8)|y1;

        // YUY2 colorspace fix. rewrited from sse2 asm
        DWORD y1 = (DWORD)(((((*d2&0xff)-0x10)*(s2[3]>>1))>>7)+s2[1])&0xff;      // y1
        DWORD uu = (DWORD)((((((*d2>>8)&0xff)-0x80)*(ia>>1))>>7)+s2[0])&0xff;    // u
        DWORD y2 = (DWORD)((((((*d2>>16)&0xff)-0x10)*(s2[7]>>1))>>7)+s2[5])&0xff;  // y2
        DWORD vv = (DWORD)((((((*d2>>24)&0xff)-0x80)*(ia>>1))>>7)+s2[4])&0xff;    // v
        *d2 = (y1)|(uu<<8)|(y2<<16)|(vv<<24);
      }
    }
  }
}

STDMETHODIMP CMemSubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
  ASSERT(pTarget);

  if(!pSrc || !pDst || !pTarget)
    return E_POINTER;

  const SubPicDesc& src = m_spd;
  SubPicDesc dst = *pTarget; // copy, because we might modify it

  if(src.type != dst.type)
    return E_INVALIDARG;

  Com::SmartRect rs(*pSrc), rd(*pDst);

  if(dst.h < 0)
  {
    dst.h = -dst.h;
    rd.bottom = dst.h - rd.bottom;
    rd.top = dst.h - rd.top;
  }

  if(rs.Width() != rd.Width() || rs.Height() != abs(rd.Height()))
    return E_INVALIDARG;

  int w = rs.Width(), h = rs.Height();

  BYTE* s = (BYTE*)src.bits + src.pitch*rs.top + rs.left*4;
  BYTE* d = (BYTE*)dst.bits + dst.pitch*rd.top + ((rd.left*dst.bpp)>>3);

  if(rd.top > rd.bottom)
  {
    if(dst.type == MSP_RGB32 || dst.type == MSP_RGB24
    || dst.type == MSP_RGB16 || dst.type == MSP_RGB15
    || dst.type == MSP_YUY2 || dst.type == MSP_AYUV)
    {
      d = (BYTE*)dst.bits + dst.pitch*(rd.top-1) + (rd.left*dst.bpp>>3);
    }
    else if(dst.type == MSP_YV12 || dst.type == MSP_IYUV)
    {
      d = (BYTE*)dst.bits + dst.pitch*(rd.top-1) + (rd.left*8>>3);
    }
    else 
    {
      return E_NOTIMPL;
    }

    dst.pitch = -dst.pitch;
  }

  switch(dst.type)
  {
  case MSP_RGBA:
    for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
    {
      BYTE* s2 = s;
      BYTE* s2end = s2 + w*4;
      DWORD* d2 = (DWORD*)d;
      for(; s2 < s2end; s2 += 4, d2++)
      {
        if(s2[3] < 0xff)
        {
          DWORD bd =0x00000100 -( (DWORD) s2[3]);
          DWORD B = ((*((DWORD*)s2)&0x000000ff)<<8)/bd;
          DWORD V = ((*((DWORD*)s2)&0x0000ff00)/bd)<<8;
          DWORD R = (((*((DWORD*)s2)&0x00ff0000)>>8)/bd)<<16;
          *d2 = B | V | R
            | (0xff000000-(*((DWORD*)s2)&0xff000000))&0xff000000;
        }
      }
    }
    break;
  case MSP_RGB32:
  case MSP_AYUV:
    for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
    {
      BYTE* s2 = s;
      BYTE* s2end = s2 + w*4;

      DWORD* d2 = (DWORD*)d;
      for(; s2 < s2end; s2 += 4, d2++)
      {
#ifdef _WIN64
        DWORD ia = 256-s2[3];
        if(s2[3] < 0xff)
        {  
          *d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (((*((DWORD*)s2)&0x00ff00ff)*ia)>>8)&0x00ff00ff)
            | ((((*d2&0x0000ff00)*s2[3])>>8) + (((*((DWORD*)s2)&0x0000ff00)*ia)>>8)&0x0000ff00);
        }
#else
        if(s2[3] < 0xff)
        {  
          *d2 = ((((*d2&0x00ff00ff)*s2[3])>>8) + (*((DWORD*)s2)&0x00ff00ff)&0x00ff00ff)
            | ((((*d2&0x0000ff00)*s2[3])>>8) + (*((DWORD*)s2)&0x0000ff00)&0x0000ff00);
        }
#endif
      }
    }
    break;
  case MSP_RGB24:
    for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
    {
      BYTE* s2 = s;
      BYTE* s2end = s2 + w*4;
      BYTE* d2 = d;
      for(; s2 < s2end; s2 += 4, d2 += 3)
      {
        if(s2[3] < 0xff)
        {
          d2[0] = ((d2[0]*s2[3])>>8) + s2[0];
          d2[1] = ((d2[1]*s2[3])>>8) + s2[1];
          d2[2] = ((d2[2]*s2[3])>>8) + s2[2];
        }
      }
    }
    break;
  case MSP_RGB16:
    for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
    {
      BYTE* s2 = s;
      BYTE* s2end = s2 + w*4;
      WORD* d2 = (WORD*)d;
      for(; s2 < s2end; s2 += 4, d2++)
      {
        if(s2[3] < 0x1f)
        {
          
          *d2 = (WORD)((((((*d2&0xf81f)*s2[3])>>5) + (*(DWORD*)s2&0xf81f))&0xf81f)
                | (((((*d2&0x07e0)*s2[3])>>5) + (*(DWORD*)s2&0x07e0))&0x07e0));
        }
      }
    }
    break;
  case MSP_RGB15:
    for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
    {
      BYTE* s2 = s;
      BYTE* s2end = s2 + w*4;
      WORD* d2 = (WORD*)d;
      for(; s2 < s2end; s2 += 4, d2++)
      {
        if(s2[3] < 0x1f)
        {
          *d2 = (WORD)((((((*d2&0x7c1f)*s2[3])>>5) + (*(DWORD*)s2&0x7c1f))&0x7c1f)
                | (((((*d2&0x03e0)*s2[3])>>5) + (*(DWORD*)s2&0x03e0))&0x03e0));
      }
    }
    }
    break;
  case MSP_YUY2:
  {
      void (*alphablt_func)(int w, int h, BYTE* d, int dstpitch, BYTE* s, int srcpitch);
#ifdef _WIN64
      alphablt_func = AlphaBlt_YUY2_SSE2;
#else
      alphablt_func = AlphaBlt_YUY2_MMX;
#endif
      //alphablt_func = AlphaBlt_YUY2_C;

      alphablt_func(w, h, d, dst.pitch, s, src.pitch);
  }
    break;
  case MSP_YV12:
  case MSP_IYUV:
    for(ptrdiff_t j = 0; j < h; j++, s += src.pitch, d += dst.pitch)
    {
      BYTE* s2 = s;
      BYTE* s2end = s2 + w*4;
      BYTE* d2 = d;
      for(; s2 < s2end; s2 += 4, d2++)
      {
        if(s2[3] < 0xff)
        {
          d2[0] = (((d2[0]-0x10)*s2[3])>>8) + s2[1];
        }
      }
    }
    break;
  default:
      return E_NOTIMPL;
  }

  dst.pitch = abs(dst.pitch);

  if(dst.type == MSP_YV12 || dst.type == MSP_IYUV)
  {
    int h2 = h/2;

    if(!dst.pitchUV)
    {
      dst.pitchUV = dst.pitch/2;
    }

    BYTE* ss[2];
    ss[0] = (BYTE*)src.bits + src.pitch*rs.top + rs.left*4;
    ss[1] = ss[0] + 4;

    if(!dst.bitsU || !dst.bitsV)
    {
      dst.bitsU = (BYTE*)dst.bits + dst.pitch*dst.h;
      dst.bitsV = dst.bitsU + dst.pitchUV*dst.h/2;

      if(dst.type == MSP_YV12)
      {
        BYTE* p = dst.bitsU; 
        dst.bitsU = dst.bitsV; 
        dst.bitsV = p;
      }
    }

    BYTE* dd[2];
    dd[0] = dst.bitsU + dst.pitchUV*rd.top/2 + rd.left/2;
    dd[1] = dst.bitsV + dst.pitchUV*rd.top/2 + rd.left/2;

    if(rd.top > rd.bottom)
    {
      dd[0] = dst.bitsU + dst.pitchUV*(rd.top/2-1) + rd.left/2;
      dd[1] = dst.bitsV + dst.pitchUV*(rd.top/2-1) + rd.left/2;
      dst.pitchUV = -dst.pitchUV;
    }

    for(ptrdiff_t i = 0; i < 2; i++)
    {
      s = ss[i]; d = dd[i];
      BYTE* is = ss[1-i];
      for(ptrdiff_t j = 0; j < h2; j++, s += src.pitch*2, d += dst.pitchUV, is += src.pitch*2)
      {
        BYTE* s2 = s;
        BYTE* s2end = s2 + w*4;
        BYTE* d2 = d;
        BYTE* is2 = is;
        for(; s2 < s2end; s2 += 8, d2++, is2 += 8)
        {
          unsigned int ia = (s2[3]+s2[3+src.pitch]+is2[3]+is2[3+src.pitch])>>2;
          if(ia < 0xff)
          {
            *d2 = (((*d2-0x80)*ia)>>8) + ((s2[0]+s2[src.pitch])>>1);
          }
        }
      }
    }
  }


#ifndef _WIN64
  __asm emms;
#endif

    return S_OK;
}

//
// CMemSubPicAllocator
//

CMemSubPicAllocator::CMemSubPicAllocator(int type, SIZE maxsize) 
  : ISubPicAllocatorImpl(maxsize, false, false)
  , m_type(type)
  , m_maxsize(maxsize)
{
}

// ISubPicAllocatorImpl

bool CMemSubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
  if(!ppSubPic) 
    return(false);

  SubPicDesc spd;
  spd.w = m_maxsize.cx;
  spd.h = m_maxsize.cy;
  spd.bpp = 32;
  spd.pitch = (spd.w*spd.bpp)>>3;
  spd.type = m_type;
  spd.bits = DNew BYTE[spd.pitch*spd.h];
  if(!spd.bits)
    return(false);

  *ppSubPic = DNew CMemSubPic(spd);
  if(!(*ppSubPic))
    return(false);

  (*ppSubPic)->AddRef();

  return(true);
}
