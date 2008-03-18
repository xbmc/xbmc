#pragma once

#include "IFile.h"

class IFileSystem;

namespace XFILE
{
  class CFileMemUnit : public IFile
  {
  public:
    CFileMemUnit();
    virtual ~CFileMemUnit();
    virtual __int64 GetPosition();
    virtual __int64 GetLength();
    virtual bool Open(const CURL& url, bool bBinary = true);
    virtual bool OpenForWrite(const CURL& url, bool bBinary = true, bool bOverWrite = false);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
    virtual int Write(const void* lpBuf, __int64 uiBufSize);
    virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();
    virtual bool Delete(const CURL& url);
    virtual bool Rename(const CURL& url, const CURL& urlnew);

  protected:
    IFileSystem *GetFileSystem(const CURL &url);
    CStdString   GetPath(const CURL& url);
    IFileSystem *m_fileSystem;
  };
};
