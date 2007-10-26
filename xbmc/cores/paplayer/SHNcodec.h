#pragma once
#include "CachingCodec.h"
#include "Dlllibshnplay.h"

class SHNCodec : public CachingCodec
{
public:
  SHNCodec();
  virtual ~SHNCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:

  ShnPlayFileStream m_stream;
  ShnPlay *m_handle;
  DllLibShnPlay m_dll;
};
