#pragma once
#include "ICodec.h"
#include "FileReader.h"
#include "Dlllibshnplay.h"

class SHNCodec : public ICodec
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

  CFileReader m_file;
  ShnPlayFileStream m_stream;
  ShnPlay *m_handle;
  DllLibShnPlay m_dll;
};
