
#pragma once

#include "DVDAudioCodec.h"
#include "../DVDCodecs.h"
#include "DVDAudioCodecPcm.h"

#define LPCM_BUFFER_SIZE (AVCODEC_MAX_AUDIO_FRAME_SIZE / 2)

class CDVDAudioCodecLPcm : public CDVDAudioCodecPcm
{
public:
  CDVDAudioCodecLPcm();
  virtual ~CDVDAudioCodecLPcm() {}
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual int Decode(BYTE* pData, int iSize);
  virtual const char* GetName()  { return "lpcm"; }

protected:

  int m_bufferSize;;
  BYTE m_buffer[LPCM_BUFFER_SIZE];

  CodecID m_codecID;
};
