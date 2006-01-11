
#include "../../../stdafx.h"

#include "../DVDDemuxers/DVDDemux.h"

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
#include "Audio\DVDAudioCodecPassthrough.h"

#include "DVDCodecs.h"

CDVDVideoCodec* CDVDFactoryCodec::OpenCodec(CDVDVideoCodec* pCodec, CDemuxStreamVideo *pDemuxStream )
{  
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Opening", pCodec->GetName());
    if( pCodec->Open( pDemuxStream->codec, pDemuxStream->iWidth, pDemuxStream->iHeight, pDemuxStream->ExtraData, pDemuxStream->ExtraSize ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Video: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Video: Failed with exception");
  }
  return NULL;
}

CDVDAudioCodec* CDVDFactoryCodec::OpenCodec(CDVDAudioCodec* pCodec, CDemuxStreamAudio *pDemuxStream )
{    
  try
  {
    CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Opening", pCodec->GetName());
    if( pCodec->Open( pDemuxStream->codec, pDemuxStream->iChannels, pDemuxStream->iSampleRate, 16, pDemuxStream->ExtraData, pDemuxStream->ExtraSize ) )
    {
      CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Opened", pCodec->GetName());
      return pCodec;
    }

    CLog::Log(LOGDEBUG, "FactoryCodec - Audio: %s - Failed", pCodec->GetName());
    pCodec->Dispose();
    delete pCodec;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "FactoryCodec - Audio: Failed with exception");
  }
  return NULL;
}

CDVDVideoCodec* CDVDFactoryCodec::CreateVideoCodec( CDemuxStreamVideo *pDemuxStream )
{
  CDVDVideoCodec* pCodec = NULL;

  if (pDemuxStream->codec == CODEC_ID_MPEG2VIDEO || pDemuxStream->codec == CODEC_ID_MPEG1VIDEO)
  {
    if( pCodec = OpenCodec(new CDVDVideoCodecLibMpeg2(), pDemuxStream) ) return pCodec;
  }
  
  if( pCodec = OpenCodec(new CDVDVideoCodecFFmpeg(), pDemuxStream) ) return pCodec;

  return NULL;
}

CDVDAudioCodec* CDVDFactoryCodec::CreateAudioCodec( CDemuxStreamAudio *pDemuxStream )
{
  CDVDAudioCodec* pCodec = NULL;

  pCodec = OpenCodec( new CDVDAudioCodecPassthrough(), pDemuxStream );
  if( pCodec ) return pCodec;
  
  switch (pDemuxStream->codec)
  {
  case CODEC_ID_AC3:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLiba52(), pDemuxStream );
      if( pCodec ) return pCodec;
      break;
    }
  case CODEC_ID_DTS:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibDts(), pDemuxStream );
      if( pCodec ) return pCodec;
      break;
    }
  case CODEC_ID_MP2:
  case CODEC_ID_MP3:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibMad(), pDemuxStream );
      if( pCodec ) return pCodec;
      break;
    }
  case CODEC_ID_AAC:
  //case CODEC_ID_MPEG4AAC:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLibFaad(), pDemuxStream );
      if( pCodec ) return pCodec;
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
      pCodec = OpenCodec( new CDVDAudioCodecPcm(), pDemuxStream );
      if( pCodec ) return pCodec;
      break;
    }
  //case CODEC_ID_LPCM_S16BE:
  //case CODEC_ID_LPCM_S20BE:
  case CODEC_ID_LPCM_S24BE:
    {
      pCodec = OpenCodec( new CDVDAudioCodecLPcm(), pDemuxStream );
      if( pCodec ) return pCodec;
      break;
    }
  default:
    {
      pCodec = NULL;
      break;
    }
  }

  pCodec = OpenCodec( new CDVDAudioCodecFFmpeg(), pDemuxStream );
  if( pCodec ) return pCodec;

  return NULL;
}
