
#pragma once

#include "DVDAudioCodec.h"

class DllLoader;
struct a52_state_s;
typedef struct a52_state_s a52_state_t;

class CDVDAudioCodecLiba52 : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLiba52();
  virtual ~CDVDAudioCodecLiba52();
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual bool NeedPasstrough() {return false;};

  bool SyncAC3Header(BYTE* pData, int iDataSize, int* iOffset, int* iFrameSize );

protected:
  virtual void SetDefault();

  bool m_bDllLoaded;
  a52_state_t* m_pState;

  BYTE m_inputBuffer[4096];
  BYTE* m_pInputBuffer;

  //BYTE m_frameBuffer[3840]; // max frame buffer
  //int m_frameBufferSize;

  BYTE m_decodedData[131072]; // could be a bit to big
  int m_decodedDataSize;
  //int m_iBufferSize;
  int m_iFrameSize;
  int m_iFlags;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;

  int m_iOutputChannels;
};
