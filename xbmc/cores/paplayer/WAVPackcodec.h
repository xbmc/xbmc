#pragma once
#include "ICodec.h"
#include "FileSystem/File.h"
#include "DllWAVPack.h"
#include "CachingCodec.h"

class WAVPackCodec : public CachingCodec
{
public:
  WAVPackCodec();
  virtual ~WAVPackCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  static int ReadCallback(void *id, void *data, int bcount);
  static unsigned int GetPosCallback(void *id);
  static int SetPosAbsCallback(void *id, unsigned int pos);
  static int SetPosRelCallback(void *id, int delta, int mode);
  static unsigned int GetLenghtCallback(void *id);
  static int CanSeekCallback(void *id);
  static int PushBackByteCallback(void *id, int c);

  void FormatSamples (BYTE *dst, int bps, long *src, unsigned long samcnt);

  char m_errormsg[512];
  WavpackContext* m_Handle;
  stream_reader m_Callbacks;

  BYTE*     m_Buffer;
  int       m_BufferSize; 
  int       m_BufferPos;
  BYTE*     m_ReadBuffer;

  DllWavPack m_dll;
};

