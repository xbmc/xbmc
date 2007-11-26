
#pragma once

#include "DVDAudioCodec.h"
#include "DllLiba52.h"

class CDVDAudioCodecLiba52 : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLiba52();
  virtual ~CDVDAudioCodecLiba52();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual const char* GetName() { return "liba52"; }

protected:
  virtual void SetDefault();

  void SetupChannels();

  a52_state_t* m_pState;

  BYTE m_inputBuffer[4096];
  BYTE* m_pInputBuffer;

  BYTE m_decodedData[131072]; // could be a bit to big
  int m_decodedDataSize;

  int m_iFrameSize;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceFlags;
  int m_iSourceBitrate;

  int m_iOutputChannels;
  unsigned int m_iOutputMapping;
  DllLiba52 m_dll;
};
