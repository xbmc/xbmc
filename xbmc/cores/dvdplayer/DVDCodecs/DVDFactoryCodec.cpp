
#include "../../../stdafx.h"
#include "DVDFactoryCodec.h"
#include "Video\DVDVideoCodec.h"
#include "Audio\DVDAudioCodec.h"

#include "Video\DVDVideoCodecFFmpeg.h"
#include "Video\DVDVideoCodecLibMpeg2.h"
#include "Audio\DVDAudioCodecFFmpeg.h"
#include "Audio\DVDAudioCodecLiba52.h"
#include "Audio\DVDAudioCodecLibDts.h"
#include "Audio\DVDAudioCodecLibMad.h"
#include "Audio\DVDAudioCodecLibFaad.h"
#include "Audio\DVDAudioCodecPcm.h"
#include "Audio\DVDAudioCodecLPcm.h"

#include "DVDCodecs.h"


CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CodecID codecID)
{
  CDVDVideoCodec* pVideoCodec = NULL;
  if (codecID == CODEC_ID_MPEG2VIDEO || codecID == CODEC_ID_MPEG1VIDEO)
  {
    pVideoCodec = new CDVDVideoCodecLibMpeg2();
  }
  else
  {
    pVideoCodec = new CDVDVideoCodecFFmpeg();
  }
  return pVideoCodec;
}

CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodec(CodecID codecID)
{
  CDVDAudioCodec* pAudioCodec = NULL;

  switch (codecID)
  {
  case CODEC_ID_AC3:
    {
      pAudioCodec = new CDVDAudioCodecLiba52();
      break;
    }
  case CODEC_ID_DTS:
    {
      pAudioCodec = new CDVDAudioCodecLibDts();
      break;
    }
  case CODEC_ID_MP2:
  case CODEC_ID_MP3:
    {
      pAudioCodec = new CDVDAudioCodecLibMad();
      break;
    }
  case CODEC_ID_AAC:
  //case CODEC_ID_MPEG4AAC:
    {
      pAudioCodec = new CDVDAudioCodecLibFaad();
      break;
    }
  case CODEC_ID_PCM_S32LE:
  case CODEC_ID_PCM_S32BE:
  case CODEC_ID_PCM_U32LE:
  case CODEC_ID_PCM_U32BE:
  case CODEC_ID_PCM_S24LE:
  case CODEC_ID_PCM_S24BE:
  case CODEC_ID_PCM_U24LE:
  case CODEC_ID_PCM_U24BE:
  case CODEC_ID_PCM_S24DAUD:
  case CODEC_ID_PCM_S16LE:
  case CODEC_ID_PCM_S16BE:
  case CODEC_ID_PCM_U16LE:
  case CODEC_ID_PCM_U16BE:
  case CODEC_ID_PCM_S8:
  case CODEC_ID_PCM_U8:
  case CODEC_ID_PCM_ALAW:
  case CODEC_ID_PCM_MULAW:
    {
      pAudioCodec = new CDVDAudioCodecPcm();
      break;
    }
  //case CODEC_ID_LPCM_S16BE:
  //case CODEC_ID_LPCM_S20BE:
  case CODEC_ID_LPCM_S24BE:
    {
      pAudioCodec = new CDVDAudioCodecLPcm();
      break;
    }
  default:
    {
      CLog::Log(LOGWARNING, "Unsupported audio codec");
      break;
    }
  }

  return pAudioCodec;
}
