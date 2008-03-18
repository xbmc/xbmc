
#pragma once

#include "DVDAudioCodec.h"
#include "../../../ffmpeg/DllAvCodec.h"
#include "../../../ffmpeg/DllAvFormat.h"

class CDVDAudioCodecFFmpeg : public CDVDAudioCodec
{
public:
  CDVDAudioCodecFFmpeg();
  virtual ~CDVDAudioCodecFFmpeg();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
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

  BYTE *m_buffer;
  int m_iBufferSize;
  bool m_bOpenedCodec;
  int m_iBuffered;
  
  DllAvCodec m_dllAvCodec;
  DllAvUtil m_dllAvUtil;

};

