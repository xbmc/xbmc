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
    thetype.subtype =  MEDIASUBTYPE_AAC;// FOURCCMap(0x00ff);
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
  if (codecId == CODEC_ID_MPEG4)
  {
    thetype.formattype = FORMAT_VideoInfo;
    thetype.subtype = FOURCCMap(FOURCC_XVID);
    
    //static const FOURCC fccs[]={FOURCC_XVID,FOURCC_FFDS,FOURCC_FVFW,FOURCC_DX50,FOURCC_DIVX,FOURCC_MP4V,0};
  }
  else if (codecId == CODEC_ID_MSMPEG4V3)
  {
    //static const FOURCC fccs[]={FOURCC_MP43,FOURCC_DIV3,0};
    thetype.subtype = FOURCCMap(FOURCC_MP43);

  }
  else if (codecId == CODEC_ID_MSMPEG4V2)
  {
    thetype.subtype = FOURCCMap(FOURCC_MP42);
    //static const FOURCC fccs[]={FOURCC_MP42,FOURCC_DIV2,0};

  }
  else if (codecId == CODEC_ID_MSMPEG4V1)
  {
    thetype.subtype = FOURCCMap(FOURCC_MP41);
    //static const FOURCC fccs[]={FOURCC_MP41,FOURCC_DIV1,0};

  }
  else if (codecId == CODEC_ID_MPEG1VIDEO)
  {
    thetype.subtype = FOURCCMap(FOURCC_MPEG);
    //static const FOURCC fccs[]={FOURCC_MPEG,FOURCC_MPG1,0};

  }
  else if (codecId == CODEC_ID_MPEG2VIDEO)
  {
    thetype.subtype = FOURCCMap(FOURCC_MPEG);
    //static const FOURCC fccs[]={FOURCC_MPEG,FOURCC_MPG2,0};

  }
  else if (codecId == CODEC_ID_H263 || codecId == CODEC_ID_H263P || codecId == CODEC_ID_H263I)
  {
    thetype.subtype = FOURCCMap(FOURCC_H263);
    //static const FOURCC fccs[]={FOURCC_H263,FOURCC_S263,0};

  }
  else if (codecId == CODEC_ID_H261)
  {
    thetype.subtype = FOURCCMap(FOURCC_H261);
    //static const FOURCC fccs[]={FOURCC_H261,0};

  }
  else if (codecId == CODEC_ID_WMV1)
  {
    thetype.subtype = FOURCCMap(FOURCC_WMV1);
    //static const FOURCC fccs[]={FOURCC_WMV1,0};

  }
  else if (codecId == CODEC_ID_WMV2)
  {
    thetype.subtype = FOURCCMap(FOURCC_WMV2);
    //static const FOURCC fccs[]={FOURCC_WMV2,0};

  }
  else if (codecId == CODEC_ID_WMV3)
  {
    thetype.formattype = FORMAT_VideoInfo;
    thetype.subtype = FOURCCMap(FOURCC_WMV3);
  }
  else if (codecId == CODEC_ID_MJPEG)
  {
    thetype.subtype = FOURCCMap(FOURCC_MJPG);
    //static const FOURCC fccs[]={FOURCC_MJPG,0};

  }
  else if (codecId == CODEC_ID_LJPEG)
  {
    thetype.subtype = FOURCCMap(FOURCC_LJPG);
    //static const FOURCC fccs[]={FOURCC_LJPG,0};

  }
  else if (codecId == CODEC_ID_HUFFYUV)
  {
    thetype.formattype = FORMAT_VideoInfo;//should fix it to be able to use FORMAT_VideoInfo2
    thetype.subtype = FOURCCMap(FOURCC_HFYU);
    //static const FOURCC fccs[]={FOURCC_HFYU,FOURCC_FFVH,0};

  }
  else if (codecId == CODEC_ID_FFV1)
  {
    thetype.subtype = FOURCCMap(FOURCC_FFV1);
    //static const FOURCC fccs[]={FOURCC_FFV1,0};

  }
  else if (codecId == CODEC_ID_DVVIDEO)
  {
    thetype.subtype = FOURCCMap(FOURCC_DV50);
    /* lowercase FourCC 'dvsd' for compatibility with MS DV decoder */
    //static const FOURCC fccs[]={mmioFOURCC('d','v','s','d'),FOURCC_DVSD,mmioFOURCC('d','v','2','5'),FOURCC_DV25,mmioFOURCC('d','v','5','0'),FOURCC_DV50,0};

  }
  else if (codecId == CODEC_ID_THEORA)
  {
    thetype.subtype = FOURCCMap(FOURCC_THEO);

  }
  else if (codecId == CODEC_ID_H263)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_H263);
  }
  else if (codecId == CODEC_ID_H264)
   {
    thetype.formattype = FORMAT_MPEG2Video;
    thetype.subtype = FOURCCMap(FOURCC_AVC1);
    //static const FOURCC fccs[]={FOURCC_H264,FOURCC_X264,FOURCC_AVC1,FOURCC_H264_HAALI,0};
    return thetype;
   }
  else if (codecId == CODEC_ID_FLV1)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_FLV1);
  }
  else if (codecId == CODEC_ID_CYUV)
  {
    thetype.subtype = FOURCCMap(FOURCC_CYUV);
    //static const FOURCC fccs[]={FOURCC_CYUV,0};

  }
  else if (codecId == CODEC_ID_CLJR)
  {
    thetype.subtype = FOURCCMap(FOURCC_CLJR);
    //static const FOURCC fccs[]={FOURCC_CLJR,0};

  }
  else if (codecId == CODEC_ID_ASV2)
  {
    thetype.formattype = FORMAT_VideoInfo2;
    thetype.subtype = FOURCCMap(FOURCC_ASV2);
  }
  else if (codecId == CODEC_ID_RV10)
  {
    thetype.formattype = FORMAT_VideoInfo;
    thetype.subtype = FOURCCMap(FOURCC_RV10);
  }
  else
  {
  }
  return thetype;
  
}