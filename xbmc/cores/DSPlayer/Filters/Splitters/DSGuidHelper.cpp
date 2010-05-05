#include "DSGuidHelper.h"

CDSGuidHelper g_GuidHelper;

const FOURCC* CDSGuidHelper::getCodecFOURCCs(CodecID codecId)
{
 switch (codecId)
  {
   case CODEC_ID_MPEG4:
   //case CODEC_ID_XVID4:
    {
     static const FOURCC fccs[]={FOURCC_XVID,FOURCC_FFDS,FOURCC_FVFW,FOURCC_DX50,FOURCC_DIVX,FOURCC_MP4V,0};
     return fccs;
    }
   case CODEC_ID_MSMPEG4V3:
    {
     static const FOURCC fccs[]={FOURCC_MP43,FOURCC_DIV3,0};
     return fccs;
    }
   case CODEC_ID_MSMPEG4V2:
    {
     static const FOURCC fccs[]={FOURCC_MP42,FOURCC_DIV2,0};
     return fccs;
    }
   case CODEC_ID_MSMPEG4V1:
    {
     static const FOURCC fccs[]={FOURCC_MP41,FOURCC_DIV1,0};
     return fccs;
    }
   case CODEC_ID_MPEG1VIDEO:
    {
     static const FOURCC fccs[]={FOURCC_MPEG,FOURCC_MPG1,0};
     return fccs;
    }
   case CODEC_ID_MPEG2VIDEO:
    {
     static const FOURCC fccs[]={FOURCC_MPEG,FOURCC_MPG2,0};
     return fccs;
    }
   case CODEC_ID_H263:
   case CODEC_ID_H263P:
   case CODEC_ID_H263I:
    {
     static const FOURCC fccs[]={FOURCC_H263,FOURCC_S263,0};
     return fccs;
    }
   case CODEC_ID_H261:
    {
     static const FOURCC fccs[]={FOURCC_H261,0};
     return fccs;
    }
   case CODEC_ID_WMV1:
    {
     static const FOURCC fccs[]={FOURCC_WMV1,0};
     return fccs;
    }
   case CODEC_ID_WMV2:
    {
     static const FOURCC fccs[]={FOURCC_WMV2,0};
     return fccs;
    }
   case CODEC_ID_MJPEG:
    {
     static const FOURCC fccs[]={FOURCC_MJPG,0};
     return fccs;
    }
   case CODEC_ID_LJPEG:
    {
     static const FOURCC fccs[]={FOURCC_LJPG,0};
     return fccs;
    }
   case CODEC_ID_HUFFYUV:
    {
     static const FOURCC fccs[]={FOURCC_HFYU,FOURCC_FFVH,0};
     return fccs;
    }
   case CODEC_ID_FFV1:
    {
     static const FOURCC fccs[]={FOURCC_FFV1,0};
     return fccs;
    }
   case CODEC_ID_DVVIDEO:
    {
     /* lowercase FourCC 'dvsd' for compatibility with MS DV decoder */
     static const FOURCC fccs[]={mmioFOURCC('d','v','s','d'),FOURCC_DVSD,mmioFOURCC('d','v','2','5'),FOURCC_DV25,mmioFOURCC('d','v','5','0'),FOURCC_DV50,0};
     return fccs;
    }
   case CODEC_ID_THEORA:
   //case CODEC_ID_THEORA_LIB:
    {
     static const FOURCC fccs[]={FOURCC_THEO,0};
     return fccs;
    }
   case CODEC_ID_H264:
    {
     static const FOURCC fccs[]={FOURCC_H264,FOURCC_X264,FOURCC_AVC1,FOURCC_H264_HAALI,0};
     return fccs;
    }
   case CODEC_ID_FLV1:
    {
     static const FOURCC fccs[]={FOURCC_FLV1,0};
     return fccs;
    }
   case CODEC_ID_CYUV:
    {
     static const FOURCC fccs[]={FOURCC_CYUV,0};
     return fccs;
    }
   case CODEC_ID_CLJR:
    {
     static const FOURCC fccs[]={FOURCC_CLJR,0};
     return fccs;
    }
   default:
    {
     static const FOURCC fccs[]={0,0};
     return fccs;
    }
  }
}