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

#ifdef HAS_DS_PLAYER
 
#include "cspinfo.h"
#include <vector>
#include "PODtypes.h"
#include "streams.h"
#include "moreuuids.h"
#undef _l
#define _l(x) x

const TcspInfo cspInfos[]=
{
 {
  FF_CSP_420P,_l("YV12"),
  1,12, //Bpp
  3, //numplanes
  {0,1,1,0}, //shiftX
  {0,1,1,0}, //shiftY
  {0,128,128,0},  //black,
  FOURCC_YV12, FOURCC_YV12, &MEDIASUBTYPE_YV12
 },
 {
  FF_CSP_422P,_l("422P"),
  1,18, //Bpp
  3, //numplanes
  {0,1,1,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,128,128,0},  //black
  FOURCC_422P, FOURCC_422P, &MEDIASUBTYPE_422P
 },
 {
  FF_CSP_444P,_l("444P"),
  1,24, //Bpp
  3, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,128,128,0},  //black
  FOURCC_444P, FOURCC_444P, &MEDIASUBTYPE_444P
 },
 {
  FF_CSP_411P,_l("411P"),
  1,17, //Bpp
  3, //numplanes
  {0,2,2,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,128,128,0},  //black
  FOURCC_411P, FOURCC_411P, &MEDIASUBTYPE_411P
 },
 {
  FF_CSP_410P,_l("410P"),
  1,10, //Bpp
  3, //numplanes
  {0,2,2,0}, //shiftX
  {0,2,2,0}, //shiftY
  {0,128,128,0}, //black
  FOURCC_410P, FOURCC_410P, &MEDIASUBTYPE_410P
 },

 {
  FF_CSP_YUY2,_l("YUY2"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0x8000,0,0,0},  //black
  FOURCC_YUY2, FOURCC_YUY2, &MEDIASUBTYPE_YUY2,
  0,1 //packedLumaOffset,packedChromaOffset
 },
 {
  FF_CSP_UYVY,_l("UYVY"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0x0080,0,0,0},  //black
  FOURCC_UYVY, FOURCC_UYVY, &MEDIASUBTYPE_UYVY,
  1,0 //packedLumaOffset,packedChromaOffset
 },
 {
  FF_CSP_YVYU,_l("YVYU"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0},  //shiftY
  {0x8000,0,0,0},  //black
  FOURCC_YVYU, FOURCC_YVYU, &MEDIASUBTYPE_YVYU,
  0,1 //packedLumaOffset,packedChromaOffset
 },
 {
  FF_CSP_VYUY,_l("VYUY"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0},  //shiftY
  {0x0080,0,0,0},  //black
  FOURCC_VYUY, FOURCC_VYUY, &MEDIASUBTYPE_VYUY,
  1,0 //packedLumaOffset,packedChromaOffset
 },

 {
  FF_CSP_ABGR,_l("ABGR"),
  4,32, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, 0, &MEDIASUBTYPE_RGB32
 },
 {
  FF_CSP_RGBA,_l("RGBA"),
  4,32, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, 0, &MEDIASUBTYPE_RGB32
 },
 {
  FF_CSP_BGR32,_l("BGR32"),
  4,32, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, 0, &MEDIASUBTYPE_RGB32
 },
 {
  FF_CSP_BGR24,_l("BGR24"),
  3,24, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, 0, &MEDIASUBTYPE_RGB24
 },
 {
  FF_CSP_BGR15,_l("BGR15"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, 0, &MEDIASUBTYPE_RGB555
 },
 {
  FF_CSP_BGR16,_l("BGR16"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, 0, &MEDIASUBTYPE_RGB565
 },
 {
  FF_CSP_RGB32,_l("RGB32"),
  4,32, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, FOURCC_RGB3, &MEDIASUBTYPE_RGB32
 },
 {
  FF_CSP_RGB24,_l("RGB24"),
  3,24, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, FOURCC_RGB2, &MEDIASUBTYPE_RGB24
 },
 {
  FF_CSP_RGB15,_l("RGB15"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, FOURCC_RGB5, &MEDIASUBTYPE_RGB555
 },
 {
  FF_CSP_RGB16,_l("RGB16"),
  2,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, FOURCC_RGB6, &MEDIASUBTYPE_RGB565
 },

 {
  FF_CSP_PAL8,_l("pal8"),
  1,8, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  BI_RGB, 0, &MEDIASUBTYPE_RGB8
 },
 {
  FF_CSP_CLJR,_l("cljr"),
  1,16, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  FOURCC_CLJR, FOURCC_CLJR, &MEDIASUBTYPE_CLJR
 },
 {
  FF_CSP_Y800,_l("gray"),
  1,8, //Bpp
  1, //numplanes
  {0,0,0,0}, //shiftX
  {0,0,0,0}, //shiftY
  {0,0,0,0}, //black
  FOURCC_Y800, FOURCC_Y800, &MEDIASUBTYPE_Y800
 },
 {
  FF_CSP_NV12,_l("NV12"),
  1,12, //Bpp
  2, //numplanes
  {0,0,0,0}, //shiftX
  {0,1,1,0}, //shiftY
  {0,128,128,0}, //black
  FOURCC_NV12, FOURCC_NV12, &MEDIASUBTYPE_NV12
 },
 0
};

TcspInfo cspInfoIYUV=
{
 FF_CSP_420P,_l("YV12"),
 1,12, //Bpp
 3, //numplanes
 {0,1,1,0}, //shiftX
 {0,1,1,0}, //shiftY
 {0,128,128,0},  //black,
 FOURCC_IYUV, FOURCC_IYUV, &MEDIASUBTYPE_IYUV
};

TcspInfo cspInfoNV21=
{
 FF_CSP_NV12,_l("NV21"),
 1,12, //Bpp
 2, //numplanes
 {0,0,0,0}, //shiftX
 {0,1,1,0}, //shiftY
 {0,128,128,0}, //black
 FOURCC_NV21, FOURCC_NV21, &MEDIASUBTYPE_NV21
};

const TcspFcc cspFccs[]=
{
 _l("YV12")     ,FOURCC_YV12,FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ,false,true,
 _l("I420/IYUV"),FOURCC_I420,FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ|FF_CSP_FLAGS_YUV_ORDER,false,true,
 _l("YUY2")     ,FOURCC_YUY2,FF_CSP_YUY2,false,true,
 _l("YVYU")     ,FOURCC_YVYU,FF_CSP_YVYU,false,true,
 _l("UYVY")     ,FOURCC_UYVY,FF_CSP_UYVY,false,true,
 _l("VYUY")     ,FOURCC_VYUY,FF_CSP_VYUY,false,true,
 _l("RGB32")    ,FOURCC_RGB3,FF_CSP_RGB32,true,true,
 _l("RGB24")    ,FOURCC_RGB2,FF_CSP_RGB24,true,true,
 _l("RGB555")   ,FOURCC_RGB5,FF_CSP_RGB15,true,true,
 _l("RGB565")   ,FOURCC_RGB6,FF_CSP_RGB16,true,true,
 _l("CLJR")     ,FOURCC_CLJR,FF_CSP_CLJR,false,false,
 _l("Y800")     ,FOURCC_Y800,FF_CSP_Y800,false,true,
 _l("NV12")     ,FOURCC_NV12,FF_CSP_NV12,false,false,
 _l("NV21")     ,FOURCC_NV21,FF_CSP_NV12|FF_CSP_FLAGS_YUV_ORDER,false,false,
 NULL,0
};

char* csp_getName(int csp,char *buf,size_t len)
{
 return csp_getName(csp_getInfo(csp),csp,buf,len);
}
char* csp_getName(const TcspInfo *cspInfo,int csp,char *buf,size_t len)
{
 const char *colorspaceName=cspInfo?cspInfo->name:_l("unknown");
 _sntprintf_s(buf,
             len,
             _TRUNCATE,
             _l("%s%s%s%s%s%s"),
             colorspaceName,
             csp & FF_CSP_FLAGS_VFLIP ? _l(",flipped") : _l(""),
             csp & FF_CSP_FLAGS_INTERLACED ? _l(",interlaced") : _l(""),
             csp & FF_CSP_FLAGS_YUV_ADJ ? _l(",adj") : _l(""),
             csp & FF_CSP_FLAGS_YUV_ORDER ? _l(",VU") : _l(""),
             csp & FF_CSP_FLAGS_YUV_JPEG ? _l(",JPEG") : _l(""));
 return buf;
}

const TcspInfo* csp_getInfoFcc(FOURCC fcccsp)
{
 if (fcccsp==FOURCC_IYUV || fcccsp==FOURCC_I420)
  return &cspInfoIYUV;
 else
  {
   for (int i=0;i<FF_CSPS_NUM;i++)
    if (cspInfos[i].fcccsp==fcccsp)
     return cspInfos+i;
   return NULL;
  }
}

int csp_bestMatch(int inCSP,int wantedCSPS,int *rank)
{
 int outCSP=inCSP&wantedCSPS&FF_CSPS_MASK;
 if (outCSP)
  {
   if (rank) *rank=100;
   return outCSP|(inCSP&~FF_CSPS_MASK);
  }

 const int *bestcsps=NULL;
 switch (inCSP&FF_CSPS_MASK)
  {
   case FF_CSP_420P:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_422P:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_YUY2 ,
       FF_CSP_420P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_444P:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_422P ,
       FF_CSP_420P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_411P:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_410P:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }

   case FF_CSP_YUY2:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_UYVY:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_YUY2 ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_YVYU:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_VYUY ,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_VYUY:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }

   case FF_CSP_ABGR:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_RGBA:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_ABGR ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_BGR32:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_BGR24:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_RGB24,
       FF_CSP_BGR32,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_RGB32,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_BGR15:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_BGR32,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR24,
       FF_CSP_BGR16,
       FF_CSP_RGB32,
       FF_CSP_RGB24,
       FF_CSP_RGB15,
       FF_CSP_RGB16,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_BGR16:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_BGR32,
       FF_CSP_BGR24,
       FF_CSP_BGR15,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_RGB32,
       FF_CSP_RGB24,
       FF_CSP_RGB15,
       FF_CSP_RGB16,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_RGB32:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR15,
       FF_CSP_BGR16,
       FF_CSP_RGB15,
       FF_CSP_RGB16,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_RGB24:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_BGR24,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR15,
       FF_CSP_BGR16,
       FF_CSP_RGB15,
       FF_CSP_RGB16,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_RGB15:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_BGR15,
       FF_CSP_RGB32,
       FF_CSP_RGB24,
       FF_CSP_RGB16,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_BGR24,
       FF_CSP_BGR16,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_RGB16:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_RGB15,
       FF_CSP_RGB32,
       FF_CSP_RGB24,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_BGR24,
       FF_CSP_BGR15,
       FF_CSP_BGR16,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_PAL8:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_RGB32,
       FF_CSP_BGR32,
       FF_CSP_RGB24,
       FF_CSP_BGR24,
       FF_CSP_RGB15,
       FF_CSP_BGR15,
       FF_CSP_RGB16,
       FF_CSP_BGR16,
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_CLJR:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_Y800:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   case FF_CSP_NV12:
    {
     static const int best[FF_CSPS_NUM]=
      {
       FF_CSP_420P ,
       FF_CSP_422P ,
       FF_CSP_444P ,
       FF_CSP_411P ,
       FF_CSP_410P ,
       FF_CSP_YUY2 ,
       FF_CSP_UYVY ,
       FF_CSP_YVYU ,
       FF_CSP_VYUY ,
       FF_CSP_ABGR ,
       FF_CSP_RGBA ,
       FF_CSP_BGR32,
       FF_CSP_RGB32,
       FF_CSP_BGR24,
       FF_CSP_RGB24,
       FF_CSP_BGR16,
       FF_CSP_RGB16,
       FF_CSP_BGR15,
       FF_CSP_RGB15,
       FF_CSP_NULL
      };
     bestcsps=best;
     break;
    }
   default:return FF_CSP_NULL;
  }
 if (rank) *rank=99;
 while (*bestcsps)
  {
   if (*bestcsps&wantedCSPS)
    return *bestcsps|(inCSP&~FF_CSPS_MASK);
   bestcsps++;
   if (rank) (*rank)--;
  }
 return FF_CSP_NULL;
}



bool TcspInfos::TsortFc::operator ()(const TcspInfo* &csp1,const TcspInfo* &csp2)
{
 int rank1;csp_bestMatch(csp,csp1->id,&rank1);
 int rank2;csp_bestMatch(csp,csp2->id,&rank2);
 return rank1>rank2;
}
//void TcspInfos::sort(int csp)
//{
// std::sort(begin(),end(),TsortFc(csp&FF_CSPS_MASK));
//}

//int getBMPcolorspace(const BITMAPINFOHEADER *hdr,const TcspInfos &forcedCsps)
//{
// int csp;
// switch(hdr->biCompression)
//  {
//   case BI_RGB:
//    switch (hdr->biBitCount)
//     {
//      case  8:csp=FF_CSP_PAL8|FF_CSP_FLAGS_VFLIP;break;
//      case 15:csp=FF_CSP_RGB15|FF_CSP_FLAGS_VFLIP;break;
//      case 16:csp=FF_CSP_RGB16|FF_CSP_FLAGS_VFLIP;break;
//      case 24:csp=FF_CSP_RGB24|FF_CSP_FLAGS_VFLIP;break;
//      case 32:csp=FF_CSP_RGB32|FF_CSP_FLAGS_VFLIP;break;
//      default:return FF_CSP_NULL;
//     }
//    break;
//   case FOURCC_I420:
//   case FOURCC_IYUV:
//    csp=FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ|FF_CSP_FLAGS_YUV_ORDER;
//    break;
//   case FOURCC_YV12:
//    csp=FF_CSP_420P|FF_CSP_FLAGS_YUV_ADJ;
//    break;
//   case FOURCC_YUYV:
//   case FOURCC_YUY2:
//   case FOURCC_V422:
//    csp=FF_CSP_YUY2;
//    break;
//   case FOURCC_YVYU:
//    csp=FF_CSP_YVYU;
//    break;
//   case FOURCC_UYVY:
//    csp=FF_CSP_UYVY;
//    break;
//   case FOURCC_VYUY:
//    csp=FF_CSP_VYUY;
//    break;
//   case FOURCC_Y800:
//    csp=FF_CSP_Y800;
//    break;
//   case FOURCC_444P:
//   case FOURCC_YV24:
//    csp=FF_CSP_444P;
//    break;
//   case FOURCC_422P:
//    csp=FF_CSP_422P;
//    break;
//   case FOURCC_YV16:
//    csp=FF_CSP_422P|FF_CSP_FLAGS_YUV_ADJ;
//    break;
//   case FOURCC_411P:
//   case FOURCC_Y41B:
//    csp=FF_CSP_411P;
//    break;
//   case FOURCC_410P:
//    csp=FF_CSP_410P;
//    break;
//   default:
//    return FF_CSP_NULL;
//  }
// bool ok;
// if (!forcedCsps.empty())
//  ok=std::find(forcedCsps.begin(),forcedCsps.end(),csp_getInfo(csp))!=forcedCsps.end();
// else
//  ok=true;
// return ok?csp:FF_CSP_NULL;
//}

#endif