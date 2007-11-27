
#pragma once

#include "DVDAudioCodec.h"
#include "DllLiba52.h"

#ifdef _LINUX
#define XBMC_ALIGN_INT __attribute__ ((aligned (4)))
#else
#define XBMC_ALIGN_INT
#endif

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

  int m_iFrameSize;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceFlags;
  int m_iSourceBitrate;

  int m_iOutputChannels;
  unsigned int m_iOutputMapping;
  DllLiba52 m_dll;

  int m_decodedDataSize;
  BYTE* m_pInputBuffer;

  int16_t m_decodedData[131072/2] XBMC_ALIGN_INT; // could be a bit to big
  BYTE m_inputBuffer[4096] XBMC_ALIGN_INT;
};
