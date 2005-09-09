
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

#define EMULATE_INTTYPES
#include "..\ffmpeg\avcodec.h"


CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec(CodecID codecID)
{
  CDVDVideoCodec* pVideoCodec = NULL;
  if (codecID == CODEC_ID_MPEG2VIDEO)
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
  case CODEC_ID_PCM_S16BE:
  case CODEC_ID_PCM_S16LE:
    {
      pAudioCodec = new CDVDAudioCodecFFmpeg();
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
