
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
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits);
  virtual int Decode(BYTE* pData, int iSize);

protected:

  int m_bufferSize;;
  BYTE m_buffer[LPCM_BUFFER_SIZE];

  CodecID m_codecID;
};
