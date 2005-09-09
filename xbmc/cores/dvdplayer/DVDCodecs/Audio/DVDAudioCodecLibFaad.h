
#pragma once

#include "DVDAudioCodec.h"
#include "libfaad\faad.h"

#define LIBFAAD_INPUT_SIZE (FAAD_MIN_STREAMSIZE * 8)   // 6144 bytes / 6k
#define LIBFAAD_DECODED_SIZE (16 * LIBFAAD_INPUT_SIZE)

class CDVDAudioCodecLibFaad : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLibFaad();
  virtual ~CDVDAudioCodecLibFaad();
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels()      { return m_iSourceChannels; }
  virtual int GetSampleRate()    { return m_iSourceSampleRate; }
  virtual int GetBitsPerSample() { return 16; }

private:

  void CloseDecoder();
  bool OpenDecoder();
  
  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;
  
  bool m_bDllLoaded;
  bool m_bInitializedDecoder;
  
  faacDecHandle m_pHandle;
  faacDecFrameInfo m_frameInfo;
  //
  BYTE m_decodedData[LIBFAAD_DECODED_SIZE];
  int  m_iDecodedDataSize;
  
  BYTE m_inputBuffer[LIBFAAD_INPUT_SIZE];
  int m_iInputBufferSize;
};
