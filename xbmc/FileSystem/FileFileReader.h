#pragma once

#include "IFile.h"

namespace XFILE
{
class CFileFileReader : public IFile
{
public:
  CFileFileReader();
  virtual ~CFileFileReader();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual int Write(const void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  
  virtual bool OpenForWrite(const CURL& url, bool bBinary = true, bool bOverWrite = false);
  protected:
  CFile m_reader;
};

};

