
#pragma once

#include "DVDAudioCodec.h"

class CDVDAudioCodecPcm : public CDVDAudioCodec
{
public:
  CDVDAudioCodecPcm();
  virtual ~CDVDAudioCodecPcm();
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits, void* ExtraData, unsigned int ExtraSize);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels();
  virtual int GetSampleRate();
  virtual int GetBitsPerSample();
  virtual const char* GetName()  { return "pcm"; }

protected:
  virtual void SetDefault();

  BYTE m_inputBuffer[4096];
  BYTE* m_pInputBuffer;

  short m_decodedData[131072]; // could be a bit to big
  int m_decodedDataSize;

  CodecID m_codecID;
  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;

  int m_iOutputChannels;
  
  short table[256];
};
