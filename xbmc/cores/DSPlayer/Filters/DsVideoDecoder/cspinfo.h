/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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

#ifndef _CSPINFO_H
#define _CSPINFO_H

#include "ffimgfmt.h"
#include "array_allocator.h"


struct TcspInfo
{
 int id;
 const char *name;
 int Bpp,bpp;
 unsigned int numPlanes;
 unsigned int shiftX[4],shiftY[4];
 unsigned int black[4];
 FOURCC fcc,fcccsp;const GUID *subtype;
 int packedLumaOffset,packedChromaOffset;
};
extern const TcspInfo cspInfos[];
struct TcspInfos :std::vector<const TcspInfo*,array_allocator<const TcspInfo*,FF_CSPS_NUM*2> >
{
private:
 struct TsortFc
  {
  private:
   int csp;
  public:
   TsortFc(int Icsp):csp(Icsp) {}
   bool operator ()(const TcspInfo* &csp1,const TcspInfo* &csp2);
  };
public:
 void sort(int csp);
};

static __inline const TcspInfo* csp_getInfo(int csp)
{
 switch (csp&(FF_CSPS_MASK|FF_CSP_FLAGS_YUV_ORDER))
  {
   case FF_CSP_420P|FF_CSP_FLAGS_YUV_ORDER:
    {
     extern TcspInfo cspInfoIYUV;
     return &cspInfoIYUV;
    }
   case FF_CSP_NV12|FF_CSP_FLAGS_YUV_ORDER:
    {
     extern TcspInfo cspInfoNV21;
     return &cspInfoNV21;
    }
   default:
    csp&=FF_CSPS_MASK;
    if (csp==0) return NULL;
    int i=0;
    while (csp>>=1)
     i++;
    if (i<=FF_CSPS_NUM)
     return &cspInfos[i];
    else
     return NULL;
  }
}
const TcspInfo* csp_getInfoFcc(FOURCC fcc);

static __inline int csp_isYUVplanar(int x)
{
 return x&FF_CSPS_MASK&FF_CSPS_MASK_YUV_PLANAR;
}
static __inline int csp_isYUVpacked(int x)
{
 return x&FF_CSPS_MASK&FF_CSPS_MASK_YUV_PACKED;
}
static __inline int csp_isYUV(int x)
{
 return csp_isYUVpacked(x)|csp_isYUVplanar(x);
}
static __inline int csp_isYUV_NV(int x)
{
 return csp_isYUVpacked(x)|csp_isYUVplanar(x)|(x & FF_CSP_NV12);
}
static __inline int csp_isRGB_RGB(int x)
{
 return x&FF_CSPS_MASK&FF_CSPS_MASK_RGB;
}
static __inline int csp_isRGB_BGR(int x)
{
 return x&FF_CSPS_MASK&FF_CSPS_MASK_BGR;
}
static __inline int csp_isRGB(int x)
{
 return csp_isRGB_RGB(x)|csp_isRGB_BGR(x);
}
static __inline int csp_isPAL(int x)
{
 return x&FF_CSPS_MASK&FF_CSP_PAL8;
}
static __inline int csp_supXvid(int x)
{
 return (x&FF_CSPS_MASK)&(FF_CSP_RGB24|FF_CSP_420P|FF_CSP_YUY2|FF_CSP_UYVY|FF_CSP_YVYU|FF_CSP_VYUY|FF_CSP_RGB15|FF_CSP_RGB16|FF_CSP_RGB32|FF_CSP_ABGR|FF_CSP_RGBA|FF_CSP_BGR24);
}

bool csp_inFOURCCmask(int x,FOURCC fcc);

extern char* csp_getName(const TcspInfo *cspInfo,int csp,char *buf,size_t len);
extern char* csp_getName(int csp,char *buf,size_t len);
extern int csp_bestMatch(int inCSP,int wantedCSPS,int *rank=NULL);

static __inline void csp_yuv_adj_to_plane(int &csp,const TcspInfo *cspInfo,unsigned int dy,unsigned char *data[4],stride_t stride[4])
{
    if (csp_isYUVplanar(csp) && (csp & FF_CSP_FLAGS_YUV_ADJ)) {
        csp&=~FF_CSP_FLAGS_YUV_ADJ;
        data[2]=data[0]+stride[0]*(dy>>cspInfo->shiftY[0]);stride[1]=stride[0]>>cspInfo->shiftX[1];
        data[1]=data[2]+stride[1]*(dy>>cspInfo->shiftY[1]);stride[2]=stride[0]>>cspInfo->shiftX[2];
    } else if ((csp & FF_CSP_NV12) && (csp & FF_CSP_FLAGS_YUV_ADJ)) {
        csp&=~FF_CSP_FLAGS_YUV_ADJ;
        data[1] = data[0] + stride[0] *dy;
        stride[1] = stride[0];
    }
 
}
static __inline void csp_yuv_order(int &csp,unsigned char *data[4],stride_t stride[4])
{
 if (csp_isYUVplanar(csp) && (csp&FF_CSP_FLAGS_YUV_ORDER))
  {
   csp&=~FF_CSP_FLAGS_YUV_ORDER;
   std::swap(data[1],data[2]);
   std::swap(stride[1],stride[2]);
  }
}
static __inline void csp_vflip(int &csp,const TcspInfo *cspInfo,unsigned char *data[],stride_t stride[],unsigned int dy)
{
 if (csp&FF_CSP_FLAGS_VFLIP)
  {
   csp&=~FF_CSP_FLAGS_VFLIP;
   for (unsigned int i=0;i<cspInfo->numPlanes;i++)
    {
     data[i]+=stride[i]*((dy>>cspInfo->shiftY[i])-1);
     stride[i]*=-1;
    }
  }
}

int getBMPcolorspace(const BITMAPINFOHEADER *hdr,const TcspInfos &forcedCsps);

struct TcspFcc
{
 const char *name;
 FOURCC fcc;int csp;bool flip;
 bool supEnc;
};
extern const TcspFcc cspFccs[];

#endif


