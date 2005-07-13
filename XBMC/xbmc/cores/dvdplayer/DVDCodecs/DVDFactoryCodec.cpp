
#include "../../../stdafx.h"
#include "DVDFactoryCodec.h"

#include "DVDVideoCodec.h"
#include "DVDAudioCodec.h"

#include "DVDVideoCodecFFmpeg.h"
#include "DVDVideoCodecLibMpeg2.h"
#include "DVDAudioCodecFFmpeg.h"
#include "DVDAudioCodecLiba52.h"

#define EMULATE_INTTYPES
#include "..\ffmpeg\avcodec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
    //case CODEC_ID_AAC:
  case CODEC_ID_MP2:
  case CODEC_ID_PCM_S16BE:
  case CODEC_ID_PCM_S16LE:
  case CODEC_ID_MP3:
    {
      pAudioCodec = new CDVDAudioCodecFFmpeg();
      break;
    }
  case CODEC_ID_DTS:
    {
      // dts stream
      // asyncaudiostream is unable to open dts streams, use ac97 for this
      CLog::Log(LOGWARNING, "CODEC_ID_DTS is currently not supported");
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
