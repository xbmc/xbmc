
#pragma once

#include "DVDAudioCodec.h"
#include "DllLibMad.h"

#define MAD_INPUT_SIZE (8 * 1024)
#define MAD_DECODED_SIZE (16 * MAD_INPUT_SIZE)

class CDVDAudioCodecLibMad : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLibMad();
  virtual ~CDVDAudioCodecLibMad();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels()      { return m_iSourceChannels; }
  virtual int GetSampleRate()    { return m_iSourceSampleRate; }
  virtual int GetBitsPerSample() { return 16; }
  virtual const char* GetName()  { return "libmad"; }
  virtual int GetBufferSize()    { return m_iInputBufferSize; }

private:

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;
  
  bool m_bInitialized;
  
  struct mad_synth m_synth; 
  struct mad_stream m_stream;
  struct mad_frame m_frame;
  
  BYTE m_decodedData[MAD_DECODED_SIZE];
  int  m_iDecodedDataSize;
  
  BYTE m_inputBuffer[MAD_INPUT_SIZE];
  int m_iInputBufferSize;

  DllLibMad m_dll;
};
