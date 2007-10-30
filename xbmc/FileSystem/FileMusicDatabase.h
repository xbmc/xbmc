#pragma once
#include "IFile.h"

namespace XFILE
{
class CFileMusicDatabase : public IFile
{
public:
  CFileMusicDatabase(void);
  virtual ~CFileMusicDatabase(void);
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);

  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();

protected:
  CFile m_file;
};
};
