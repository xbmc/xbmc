
#pragma once

#include "ICodec.h"
#include "../../cores/DllLoader/DllLoader.h"
#include "FileReader.h"


typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8  uint8_t;
typedef __int32          int32_t;
typedef __int16          int16_t;

#ifdef __cplusplus
extern "C"
{
#endif

#include "..\dvdplayer\dvdcodecs\audio\libdts\dts.h"
#ifdef LIBDTS_DOUBLE
typedef float convert_t;
#else
typedef sample_t convert_t;
#endif

#ifdef __cplusplus
}
#endif

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

  CFileReader m_file;
  bool m_bDllLoaded;
  dts_state_t* m_pState;

  BYTE m_inputBuffer[4096];
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

  int    m_iFrameSize;
  int    m_iFlags;
  float* m_fSamples;

  int m_iSourceSampleRate;
  int m_iSourceChannels;
  int m_iSourceBitrate;

  int m_iOutputChannels;

  struct dtsdll
  {
    dts_state_t * (__cdecl * dts_init) (uint32_t mm_accel);
    int (__cdecl * dts_syncinfo) (dts_state_t *state, uint8_t * buf, int * flags,
                      int * sample_rate, int * bit_rate, int *frame_length);
    int (__cdecl * dts_frame) (dts_state_t * state, uint8_t * buf, int * flags,
                  level_t * level, sample_t bias);
    void (__cdecl * dts_dynrng) (dts_state_t * state,
                    level_t (* call) (level_t, void *), void * data);
    int (__cdecl * dts_blocks_num) (dts_state_t * state);
    int (__cdecl * dts_block) (dts_state_t * state);
    sample_t * (__cdecl * dts_samples) (dts_state_t * state);
    void (__cdecl * dts_free) (dts_state_t * state);
  };
  dtsdll m_dll;

};
