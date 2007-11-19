
#pragma once

#include "DVDAudioCodec.h"
#include "../DllAvCodec.h"
#include "../../DVDDemuxers/DllAvFormat.h"

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec
{
public:
  CDVDAudioCodecFFmpeg();
  virtual ~CDVDAudioCodecFFmpeg();
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits, void* ExtraData, unsigned int ExtraSize);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual const char* GetName() { return "FFmpeg"; }
  virtual int GetBufferSize() { return m_iBuffered; }

protected:
  AVCodecContext* m_pCodecContext;

  unsigned char m_buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
  int m_iBufferSize;
  bool m_bOpenedCodec;
  int m_iBuffered;
  
  DllAvCodec m_dllAvCodec;
  DllAvUtil m_dllAvUtil;
};
