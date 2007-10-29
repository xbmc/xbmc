#pragma once
#include "CachingCodec.h"
#include "FileSystem/File.h"

class CDDACodec : public CachingCodec
{
public:
  CDDACodec();
  virtual ~CDDACodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  // Input buffer to read our cdda data into
  BYTE*   m_Buffer;
  int     m_BufferSize; 
  int     m_BufferPos;
};
