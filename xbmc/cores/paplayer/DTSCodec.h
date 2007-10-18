
#pragma once

#include "FileSystem/File.h"
#include "ICodec.h"
#include "DllDCACodec.h"

#ifdef HAS_DTS_CODEC

class DTSCodec : public ICodec
{
public:
  DTSCodec();
  virtual ~DTSCodec();

  virtual bool    Init(const CStdString &strFile, unsigned int filecache);
  virtual void    DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int     ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool    CanInit();

protected:
  virtual bool CalculateTotalTime();
  virtual int  ReadInput();
  virtual void PrepairBuffers();
  virtual bool InitFile(const CStdString &strFile, unsigned int filecache);
  virtual void CloseFile();
  virtual void SetDefault();
  
  int  Decode(BYTE* pData, int iSize);
  int  GetNrOfChannels(int flags);
  
  static void convert2s16_2(convert_t * _f, int16_t * s16);
  static void convert2s16_3(convert_t * _f, int16_t * s16);
  static void convert2s16_4(convert_t * _f, int16_t * s16);
  static void convert2s16_5(convert_t * _f, int16_t * s16);
  static void convert2s16_multi(convert_t * _f, int16_t * s16, int flags);

  dts_state_t* m_pState;

  BYTE m_inputBuffer[12288];
  BYTE* m_pInputBuffer;

  BYTE* m_readBuffer;
  unsigned int m_readingBufferSize;
  unsigned int m_readBufferPos;

  BYTE* m_decodedData;
  unsigned int m_decodingBufferSize;
  unsigned int m_decodedDataSize;

  bool m_eof;
  int  m_iDataStart;
  bool m_IsInitialized;
  bool m_DecoderError;

  int    m_iFrameSize;
  int    m_iFlags;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;

  int m_iOutputChannels;

  DllDCACodec m_dll;
};
#endif
