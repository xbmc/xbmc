#include "DSGuidHelper.h"

#include "moreuuids.h"
#include "ffmpeg_mediaguids.h"
CDSGuidHelper g_GuidHelper;

CMediaType CDSGuidHelper::initAudioType(CodecID codecId)
{
  CMediaType thetype;
  thetype.InitMediaType();
  thetype.majortype = MEDIATYPE_Audio;
  thetype.formattype = FORMAT_WaveFormatEx;//default value
  thetype.subtype = GUID_NULL;//If not set the subtype will be set with //FOURCCMap(avstream->codec->codec_tag);

  switch (codecId)
  {
    case CODEC_ID_AAC:
    

      break;
    
    case CODEC_ID_AC3:
      break;
  }
  if (codecId == CODEC_ID_AC3)
  {
    thetype.formattype = FORMAT_WaveFormatEx;
    thetype.subtype = MEDIASUBTYPE_WAVE_DOLBY_AC3;
  }
  else if (codecId == CODEC_ID_MP2)
  {
  
  }
  else if (codecId == CODEC_ID_MP3)
  {
    thetype.formattype = FORMAT_WaveFormatEx;
  }
  else if (codecId == CODEC_ID_AAC)
  {
    thetype.formattype = FORMAT_WaveFormatEx;
    thetype.subtype =  MEDIASUBTYPE_AAC;
  }
  else if (codecId == CODEC_ID_DTS)
  {
  }
  else if (codecId == CODEC_ID_VORBIS)
  {
  }
  else if (codecId == CODEC_ID_DVAUDIO)
  {
  }
  else if (codecId == CODEC_ID_WMAV1)
  {
  }
  else if (codecId == CODEC_ID_WMAV2)
  {
  }
  else if (codecId == CODEC_ID_MACE3)
  {
  }
  else if (codecId == CODEC_ID_MACE6)
  {
  }
  else if (codecId == CODEC_ID_VMDAUDIO)
  {
  }
  else if (codecId == CODEC_ID_SONIC)
  {
  }
  else if (codecId == CODEC_ID_SONIC_LS)
  {
  }
  else if (codecId == CODEC_ID_FLAC)
  {
    thetype.formattype = FORMAT_WaveFormatEx;
    thetype.subtype =  MEDIASUBTYPE_FLAC2;
  }
  else if (codecId == CODEC_ID_MP3ADU)
  {
  }
  else if (codecId == CODEC_ID_MP3ON4)
  {
  }
  else if (codecId == CODEC_ID_SHORTEN)
  {
  }
  else if (codecId == CODEC_ID_ALAC)
  {
  }
  else if (codecId == CODEC_ID_WESTWOOD_SND1)
  {
  }
  else if (codecId == CODEC_ID_QDM2)
  {
  }
  else if (codecId == CODEC_ID_COOK)
  {
  }
  else if (codecId == CODEC_ID_TRUESPEECH)
  {
  }
  else if (codecId == CODEC_ID_TTA)
  {
  }
  else if (codecId == CODEC_ID_SMACKAUDIO)
  {
  }
  else if (codecId == CODEC_ID_QCELP)
  {
  }
  else if (codecId == CODEC_ID_WAVPACK)
  {
  }
  else if (codecId == CODEC_ID_DSICINAUDIO)
  {
  }
  else if (codecId == CODEC_ID_IMC)
  {
  }
  else if (codecId == CODEC_ID_MUSEPACK7)
  {
  }
  else if (codecId == CODEC_ID_MLP)
  {
  }
  else if (codecId == CODEC_ID_GSM_MS)
  {
  }
  else if (codecId == CODEC_ID_ATRAC3)
  {
  }
  else if (codecId == CODEC_ID_VOXWARE)
  {
  }
  else if (codecId == CODEC_ID_APE)
  {
  }
  else if (codecId == CODEC_ID_NELLYMOSER)
  {
  }
  else if (codecId == CODEC_ID_MUSEPACK8)
  {
  }
  else if (codecId == CODEC_ID_SPEEX)
  {
  }
  else if (codecId == CODEC_ID_WMAVOICE)
  {
  }
  else if (codecId == CODEC_ID_WMAPRO)
  {
  }
  else if (codecId == CODEC_ID_WMALOSSLESS)
  {
  }
  else if (codecId == CODEC_ID_ATRAC3P)
  {
  }
  else if (codecId == CODEC_ID_EAC3)
  {
  }
  else if (codecId == CODEC_ID_SIPR)
  {
  }
  else if (codecId == CODEC_ID_MP1)
  {
  }
  else if (codecId == CODEC_ID_TWINVQ)
  {
  }
  else if (codecId == CODEC_ID_TRUEHD)
  {
  }
  else if (codecId == CODEC_ID_MP4ALS)
  {
  }
  else if (codecId == CODEC_ID_ATRAC1)
  {
  }
  else if (codecId == CODEC_ID_BINKAUDIO_RDFT)
  {
  }
  else if (codecId == CODEC_ID_BINKAUDIO_DCT)
  {
  }
  return thetype;
}

CMediaType CDSGuidHelper::initVideoType(CodecID codecId)
{
  CMediaType thetype;
  thetype.InitMediaType();
  thetype.majortype = MEDIATYPE_Video;
  thetype.formattype = FORMAT_VideoInfo;//default value
  if (codecId ==  CODEC_ID_4XM)
  {
  }
  else if (codecId == CODEC_ID_8BPS)
  {
    thetype.subtype = FOURCCMap(FOURCC_8BPS);
  }
  //else if (codecId == CODEC_ID_FRWU)
  else if (codecId == CODEC_ID_AASC)
  {
    thetype.subtype = FOURCCMap(FOURCC_AASC);
  }
  else if (codecId == CODEC_ID_AMV)
  {
  }
  else if (codecId == CODEC_ID_ANM)
  {
  }
  else if (codecId == CODEC_ID_ASV1)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_ASV1);
  }
  else if (codecId == CODEC_ID_ASV2)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_ASV2);
  }
  //else if (codecId == CODEC_ID_AURA)
  //else if (codecId == CODEC_ID_AURA2)
  else if (codecId == CODEC_ID_AVS)
  {
  }
  else if (codecId == CODEC_ID_BETHSOFTVID)
  {
  }
  else if (codecId == CODEC_ID_BFI)
  {
  }
  else if (codecId == CODEC_ID_BINKVIDEO)
  {
  }
  else if (codecId == CODEC_ID_BMP)
  {
  }
  else if (codecId == CODEC_ID_C93)
  {
  }
  //else if (codecId == CODEC_ID_CAMSTUDIO)
  //else if (codecId == CODEC_ID_CAMTASIA)
  else if (codecId == CODEC_ID_CAVS)
  {
    thetype.subtype = FOURCCMap(FOURCC_CAVS);
  }
  else if (codecId == CODEC_ID_CDGRAPHICS)
  {
  }
  else if (codecId == CODEC_ID_CINEPAK)
  {
  }
  else if (codecId == CODEC_ID_CLJR)
  {
    thetype.subtype = FOURCCMap(FOURCC_CLJR);
  }
  else if (codecId == CODEC_ID_CYUV)
  {
    thetype.subtype = FOURCCMap(FOURCC_CYUV);
  }
  else if (codecId == CODEC_ID_DNXHD)
  {
  }
  else if (codecId == CODEC_ID_DPX)
  {
  }
  else if (codecId == CODEC_ID_DSICINVIDEO)
  {
  }
  else if (codecId == CODEC_ID_DVVIDEO)
  {
  }
  else if (codecId == CODEC_ID_DXA)
  {
  }
  else if (codecId == CODEC_ID_CMV)//Electronic Arts CMV video
  {
  }
  else if (codecId == CODEC_ID_MAD)//Electronic Arts Madcow Video
  {
  }
  else if (codecId == CODEC_ID_TGQ)//Electronic Arts TGQ Video
  {
  }
  else if (codecId == CODEC_ID_TGV)//Electronic Arts TGV Video
  {
  }
  else if (codecId == CODEC_ID_TQI)//Electronic Arts TQI Video
  {
  }
  else if (codecId == CODEC_ID_ESCAPE124)
  {
  }
  else if (codecId == CODEC_ID_FFV1)
  {
    thetype.subtype = FOURCCMap(FOURCC_FFV1);
  }
  else if (codecId == CODEC_ID_FFVHUFF)
  {
  }
  else if (codecId == CODEC_ID_FLASHSV)
  {
  }
  else if (codecId == CODEC_ID_FLIC)
  {
  }
  else if (codecId == CODEC_ID_FLV1)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_FLV1);
  }
  else if (codecId == CODEC_ID_FRAPS)
  {
  }
  else if (codecId == CODEC_ID_GIF)
  {
  }
  else if (codecId == CODEC_ID_H261)
  {
    thetype.subtype = FOURCCMap(FOURCC_H261);
  }
  else if (codecId == CODEC_ID_H263 || codecId == CODEC_ID_H263I)
  {
    thetype.subtype = FOURCCMap(FOURCC_H263);
  }
  else if (codecId == CODEC_ID_H264)
  {
    thetype.formattype = FORMAT_MPEG2Video;
    thetype.subtype = FOURCCMap(FOURCC_AVC1);//FOURCC_H264,FOURCC_X264,FOURCC_AVC1,FOURCC_H264_HAALI
  }
  else if (codecId == CODEC_ID_HUFFYUV)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_HFYU);
  }
  else if (codecId == CODEC_ID_IDCIN)//id Quake II CIN video
  {
  }
  else if (codecId == CODEC_ID_IFF_BYTERUN1)
  {
  }
  else if (codecId == CODEC_ID_IFF_ILBM)
  {
  }
  else if (codecId == CODEC_ID_INDEO2)
  {
  }
  else if (codecId == CODEC_ID_INDEO3)
  {
  }
  else if (codecId == CODEC_ID_INDEO5)
  {
  }
  else if (codecId == CODEC_ID_INTERPLAY_VIDEO)//Interplay MVE video
  {
  }
  else if (codecId == CODEC_ID_JPEGLS)
  {
  }
  else if (codecId == CODEC_ID_KGV1)
  {
  }
  else if (codecId == CODEC_ID_KMVC)
  {
  }
  else if (codecId == CODEC_ID_JPEG2000)
  {
  }
  else if (codecId == CODEC_ID_DIRAC)
  {
  }
  else if (codecId == CODEC_ID_LOCO)
  {
    thetype.subtype = FOURCCMap(FOURCC_LOCO);
  }
  else if (codecId == CODEC_ID_MDEC)
  {

  }
  else if (codecId == CODEC_ID_MIMIC)
  {
  }
  else if (codecId == CODEC_ID_MJPEG)
  {
  }
  else if (codecId == CODEC_ID_MJPEGB)
  {
  }
  else if (codecId == CODEC_ID_MMVIDEO)
  {
  }
  else if (codecId == CODEC_ID_MOTIONPIXELS)
  {
  }
  else if (codecId == CODEC_ID_MPEG1VIDEO)
  {
  }
  else if (codecId == CODEC_ID_MPEG2VIDEO)
  {
  }
  else if (codecId == CODEC_ID_MPEG4)
  {
    thetype.formattype = FORMAT_VideoInfo;
    thetype.subtype = FOURCCMap(FOURCC_MP4V);//FOURCC_XVID,FOURCC_FFDS,FOURCC_FVFW,FOURCC_DX50,FOURCC_DIVX,FOURCC_MP4V
  }
  else if (codecId == CODEC_ID_MPEG1VIDEO || codecId == CODEC_ID_MPEG2VIDEO)
  {
  }
  else if (codecId == CODEC_ID_MSMPEG4V3)
  {
  }
  else if (codecId == CODEC_ID_MSMPEG4V1)
  {
  }
  else if (codecId == CODEC_ID_MSMPEG4V2)
  {
  }
  else if (codecId == CODEC_ID_MSRLE)
  {
  }
  else if (codecId == CODEC_ID_MSVIDEO1)
  {
  }
  else if (codecId == CODEC_ID_MSZH)
  {
    thetype.subtype = FOURCCMap(FOURCC_MSZH);
  }
  else if (codecId == CODEC_ID_NUV)
  {
  }
  else if (codecId == CODEC_ID_PAM)
  {
  }
  else if (codecId == CODEC_ID_PBM)
  {
  }
  else if (codecId == CODEC_ID_PCX)
  {
  }
  else if (codecId == CODEC_ID_PGM)
  {
  }
  else if (codecId == CODEC_ID_PGMYUV)
  {
  }
  else if (codecId == CODEC_ID_PNG)
  {
  }
  else if (codecId == CODEC_ID_PPM)
  {
  }
  else if (codecId == CODEC_ID_PTX)
  {
  }
  else if (codecId == CODEC_ID_QDRAW)
  {
  }
  else if (codecId == CODEC_ID_QPEG)
  {
    thetype.subtype = FOURCCMap(FOURCC_QPEG);
  }
  else if (codecId == CODEC_ID_QTRLE)
  {
  }
  else if (codecId == CODEC_ID_R210)
  {

  }
  else if (codecId == CODEC_ID_RAWVIDEO)
  {
  }
  else if (codecId == CODEC_ID_RL2)
  {
  }
  else if (codecId == CODEC_ID_ROQ)
  {
  }
  else if (codecId == CODEC_ID_RPZA)
  {
    thetype.subtype = FOURCCMap(FOURCC_RPZA);
  }
  else if (codecId == CODEC_ID_RV10)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_RV10);
  }
  else if (codecId == CODEC_ID_RV20)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_RV20);
  }
  else if (codecId == CODEC_ID_RV30)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_RV30);
  }
  else if (codecId == CODEC_ID_RV40)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_RV40);
  }
  else if (codecId == CODEC_ID_SGI)
  {
  }
  else if (codecId == CODEC_ID_SMACKVIDEO)
  {
  }
  else if (codecId == CODEC_ID_SMC)
  {
  }
  else if (codecId == CODEC_ID_SNOW)
  {

  }
  else if (codecId == CODEC_ID_SP5X)
  {
    thetype.subtype = FOURCCMap(FOURCC_SP5X);
  }
  else if (codecId == CODEC_ID_SUNRAST)
  {
  }
  else if (codecId == CODEC_ID_SVQ1)
  {
    thetype.subtype = FOURCCMap(FOURCC_SVQ1);
  }
  else if (codecId == CODEC_ID_SVQ3)
  {
    thetype.subtype = FOURCCMap(FOURCC_SVQ3);
  }
  else if (codecId == CODEC_ID_TARGA)
  {
  }
  else if (codecId == CODEC_ID_THEORA)
  {
  }
  else if (codecId == CODEC_ID_THP)
  {
  }
  else if (codecId == CODEC_ID_TIERTEXSEQVIDEO)
  {
  }
  else if (codecId == CODEC_ID_TIFF)
  {

  }
  else if (codecId == CODEC_ID_TMV)
  {
  }
  else if (codecId == CODEC_ID_TRUEMOTION1)
  {
  }
  else if (codecId == CODEC_ID_TRUEMOTION2)
  {
  }
  else if (codecId == CODEC_ID_TXD)
  {
  }
  //else if (codecId == CODEC_ID_ULTIMOTION)
  //else if (codecId == CODEC_ID_V210)
  else if (codecId == CODEC_ID_V210X)
  {
  }
  else if (codecId == CODEC_ID_VB)
  {
  }
  else if (codecId == CODEC_ID_VC1)
  {
  }
  else if (codecId == CODEC_ID_VCR1)
  {
    thetype.subtype = FOURCCMap(FOURCC_VCR1);
  }
  else if (codecId == CODEC_ID_VMDVIDEO)
  {
  }
  //else if (codecId == CODEC_ID_VMNC)
  else if (codecId == CODEC_ID_VP3)
  {
  }
  else if (codecId == CODEC_ID_VP5)
  {
  }
  else if (codecId == CODEC_ID_VP6)
  {
  }
  else if (codecId == CODEC_ID_VP6A)
  {
    thetype.subtype = FOURCCMap(FOURCC_VP6A);
  }
  else if (codecId == CODEC_ID_VP6F)
  {
    thetype.subtype = FOURCCMap(FOURCC_VP6F);
  }
  else if (codecId == CODEC_ID_WS_VQA)
  {
  }
  else if (codecId == CODEC_ID_WMV1)
  {
    thetype.subtype = FOURCCMap(FOURCC_WMV1);
  }
  else if (codecId == CODEC_ID_WMV2)
  {
    thetype.subtype = FOURCCMap(FOURCC_WMV2);
  }
  else if (codecId == CODEC_ID_WMV3)
  {
    thetype.formattype = FORMAT_VideoInfo;
    thetype.subtype = FOURCCMap(FOURCC_WMV3);
  }
//if block limit
  if (codecId == CODEC_ID_WNV1)
  {
    thetype.subtype = FOURCCMap(FOURCC_WNV1);
  }
  else if (codecId == CODEC_ID_XAN_WC3)
  {
  }
  else if (codecId == CODEC_ID_VIXL)
  {
  }
  //else if (codecId == CODEC_ID_YOP) too recent for the current ffmpeg version
  else if (codecId == CODEC_ID_ZLIB)
  {
    thetype.subtype = FOURCCMap(FOURCC_ZLIB);
  }
  else if (codecId == CODEC_ID_ZMBV)
  {
    thetype.subtype = FOURCCMap(FOURCC_ZMBV);
  }
  
  return thetype;
  
}

int64_t CDSGuidHelper::lavc_gcd(int64_t a, int64_t b)
{
    if(b) return lavc_gcd(b, a%b);
    else  return a;
}
uint64_t ff_abs(int64_t x)
{
 return uint64_t((x<0) ? -x : x);
}

int CDSGuidHelper::math_reduce(int *dst_nom, int *dst_den, int64_t nom, int64_t den, int64_t max)
{
AVRational a0={0,1}, a1={1,0};
    int sign= (nom<0) ^ (den<0);
    int64_t gcd= lavc_gcd(ff_abs(nom), ff_abs(den));

    if(gcd){
    nom = ff_abs(nom)/gcd;
    den = ff_abs(den)/gcd;
    }
    if(nom<=max && den<=max){
        a1.num=(int)nom;a1.den=(int)den;//= (AVRational){nom, den};
        den=0;
    }

    while(den){
        int64_t x       = nom / den;
        int64_t next_den= nom - den*x;
        int64_t a2n= x*a1.num + a0.num;
        int64_t a2d= x*a1.den + a0.den;

        if(a2n > max || a2d > max) break;

        a0= a1;
        a1.num=(int)a2n;a1.den=(int)a2d;//= (AVRational){a2n, a2d};
        nom= den;
        den= next_den;
    }
    assert(lavc_gcd(a1.num, a1.den) == 1U);

    *dst_nom = sign ? -a1.num : a1.num;
    *dst_den = a1.den;

    return den==0;
}