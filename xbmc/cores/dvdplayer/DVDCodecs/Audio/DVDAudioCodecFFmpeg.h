
#pragma once

#include "DVDAudioCodec.h"

struct AVCodecContext;

// should be the same as in avcodec.h
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 131072

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec
{
public:
  CDVDAudioCodecFFmpeg();
  virtual ~CDVDAudioCodecFFmpeg();
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();

protected:
  AVCodecContext* m_pCodecContext;

  unsigned char m_buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
  int m_iBufferSize;
  bool m_bOpenedCodec;
  bool m_bDllLoaded;
};
