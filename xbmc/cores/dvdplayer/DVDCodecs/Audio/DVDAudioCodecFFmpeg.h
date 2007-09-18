
#pragma once

#include "DVDAudioCodec.h"
#include "../../../ffmpeg/DllAvCodec.h"
#include "../../../ffmpeg/DllAvFormat.h"

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

protected:
  AVCodecContext* m_pCodecContext;

  BYTE *m_buffer;
  int m_iBufferSize;
  bool m_bOpenedCodec;
  
  DllAvCodec m_dllAvCodec;
  DllAvUtil m_dllAvUtil;

};

