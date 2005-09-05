
#pragma once

#include "DVDAudioCodec.h"

class DllLoader;

typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8  uint8_t;
typedef __int32          int32_t;
typedef __int16          int16_t;

#ifdef __cplusplus
extern "C"
{
#endif

#include "libdts\dts.h"
#ifdef LIBDTS_DOUBLE
typedef float convert_t;
#else
typedef sample_t convert_t;
#endif

#ifdef __cplusplus
}
#endif

class CDVDAudioCodecLibDts : public CDVDAudioCodec
{
public:
  CDVDAudioCodecLibDts();
  virtual ~CDVDAudioCodecLibDts();
  virtual bool Open(CodecID codecID, int iChannels, int iSampleRate, int iBits);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize);
  virtual int GetData(BYTE** dst);
  virtual void Reset();
  virtual int GetChannels()      { return m_iOutputChannels; }
  virtual int GetSampleRate()    { return m_iSourceSampleRate; }
  virtual int GetBitsPerSample() { return 16; }

protected:
  virtual void SetDefault();
  int GetNrOfChannels(int flags);
  
  // taken from the libdts project
  static void convert2s16_1(convert_t * _f, int16_t * s16);
  static void convert2s16_2(convert_t * _f, int16_t * s16);
  static void convert2s16_3(convert_t * _f, int16_t * s16);
  static void convert2s16_4(convert_t * _f, int16_t * s16);
  static void convert2s16_5(convert_t * _f, int16_t * s16);
  static void convert2s16_multi(convert_t * _f, int16_t * s16, int flags);
  static void s16_swap(int16_t * s16, int channels);
  static void s32_swap(int32_t * s32, int channels);

  bool m_bDllLoaded;
  dts_state_t* m_pState;

  BYTE m_inputBuffer[4096];
  BYTE* m_pInputBuffer;


  BYTE m_decodedData[131072]; // could be a bit to big
  int m_decodedDataSize;

  int m_iFrameSize;
  int m_iFlags;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;

  int m_iOutputChannels;
};
