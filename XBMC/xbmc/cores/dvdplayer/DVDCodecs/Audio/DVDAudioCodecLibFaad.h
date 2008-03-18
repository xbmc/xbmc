
#pragma once

#include "DVDAudioCodec.h"
#include "DllLibFaad.h"

#define LIBFAAD_INPUT_SIZE (FAAD_MIN_STREAMSIZE * 8)   // 6144 bytes / 6k
#define LIBFAAD_DECODED_SIZE (16 * LIBFAAD_INPUT_SIZE)

class CDVDAudioCodecLibFaad : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLibFaad();
  virtual ~CDVDAudioCodecLibFaad();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels()      { return m_iSourceChannels; }
  virtual int GetSampleRate()    { return m_iSourceSampleRate; }
  virtual int GetBitsPerSample() { return 16; }
  virtual const char* GetName()  { return "libfaad"; }
  virtual int GetBufferSize()    { return m_InputBufferSize; }

private:

  void CloseDecoder();
  bool OpenDecoder();
  
  bool SyncStream();
  
  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;
  
  bool m_bInitializedDecoder;
  bool m_bRawAACStream;
  
  faacDecHandle m_pHandle;
  faacDecFrameInfo m_frameInfo;

  short* m_DecodedData;
  int   m_DecodedDataSize;
  
  BYTE m_InputBuffer[LIBFAAD_INPUT_SIZE];
  int  m_InputBufferSize;

  DllLibFaad m_dll;
};
