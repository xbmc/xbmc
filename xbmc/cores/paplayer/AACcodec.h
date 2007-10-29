#pragma once
#include "CachingCodec.h"
#include "FileSystem/File.h"
#include "DllAACCodec.h"

class AACCodec : public CachingCodec
{
public:
  AACCodec();
  virtual ~AACCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
  static unsigned __int32 AACOpenCallback(const char *pName, const char *mode, void *userData);
  static void AACCloseCallback(void *userData);
  static unsigned __int32 AACReadCallback(void *userData, void *pBuffer, unsigned long nBytesToRead);
  static __int32 AACSeekCallback(void *userData, unsigned __int64 pos);
  static __int64 AACFilesizeCallback(void *userData);

  AACHandle m_Handle;
  BYTE*     m_Buffer;
  int       m_BufferSize; 
  int       m_BufferPos;

  // Our dll
  DllAACCodec m_dll;
};
