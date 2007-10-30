#pragma once
#include "IFile.h"

namespace XFILE
{
  class CFileTuxBox : public IFile
  {
    public:
      CFileTuxBox();
      virtual ~CFileTuxBox();
      virtual __int64 GetPosition();
      virtual __int64 GetLength();
      virtual bool Open(const CURL& url, bool bBinary = true);
      virtual void Close();
      virtual bool Exists(const CURL& url) { return true;};
      virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
      virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
      virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
    protected:
  };
};
