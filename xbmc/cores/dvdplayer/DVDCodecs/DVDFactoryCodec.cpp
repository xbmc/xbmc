
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
  if (codecID == CODEC_ID_AC3)
  {
    pAudioCodec = new CDVDAudioCodecLiba52();
  }
  else
  {
    pAudioCodec = new CDVDAudioCodecFFmpeg();
  }
  
  return pAudioCodec;
}
