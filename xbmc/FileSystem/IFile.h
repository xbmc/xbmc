// IFile.h: interface for the IFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_)
#define AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "system.h"
#include "../URL.h"

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

namespace XFILE
{

class ICacheInterface;

class IFile
{
public:
  IFile();
  virtual ~IFile();

  virtual bool Open(const CURL& url, bool bBinary = true) = 0;
  virtual bool OpenForWrite(const CURL& url, bool bBinary = true, bool bOverWrite = false) { return false; };
  virtual bool Exists(const CURL& url) = 0;
  virtual int Stat(const CURL& url, struct __stat64* buffer) = 0;

  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize) = 0;
  virtual int Write(const void* lpBuf, __int64 uiBufSize) { return -1;};
  virtual bool ReadString(char *szLine, int iLineLength)
  {
    if(Seek(0, SEEK_CUR) < 0) return false;

    __int64 iFilePos = GetPosition();
    int iBytesRead = Read( (unsigned char*)szLine, iLineLength - 1);
    if (iBytesRead <= 0)
      return false;

    szLine[iBytesRead] = 0;

    for (int i = 0; i < iBytesRead; i++)
    {
      if ('\n' == szLine[i])
      {
        if ('\r' == szLine[i + 1])
        {
          szLine[i + 1] = 0;
          Seek(iFilePos + i + 2, SEEK_SET);
        }
        else
        {
          // end of line
          szLine[i + 1] = 0;
          Seek(iFilePos + i + 1, SEEK_SET);
        }
        break;
      }
      else if ('\r' == szLine[i])
      {
        if ('\n' == szLine[i + 1])
        {
          szLine[i + 1] = 0;
          Seek(iFilePos + i + 2, SEEK_SET);
        }
        else
        {
          // end of line
          szLine[i + 1] = 0;
          Seek(iFilePos + i + 1, SEEK_SET);
        }
        break;
      }
    }
    return true;
  }
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET) = 0;
  virtual void Close() = 0;
  virtual __int64 GetPosition() = 0;
  virtual __int64 GetLength() = 0;
  virtual void Flush() { }
  virtual int  GetChunkSize() {return 0;}
  virtual bool SkipNext(){return false;}

  virtual bool Delete(const CURL& url) { return false; }
  virtual bool Rename(const CURL& url, const CURL& urlnew) { return false; }

  virtual ICacheInterface* GetCache() {return NULL;} 
  virtual int IoControl(int request, void* param) { return -1; }
};

class CRedirectException
{
public:
  IFile *m_pNewFileImp;

  CRedirectException() : m_pNewFileImp(NULL) { }
  CRedirectException(IFile *pNewFileImp) : m_pNewFileImp(pNewFileImp) { }
};

}

#endif // !defined(AFX_IFILE_H__7EE73AC7_36BC_4822_93FF_44F3B0C766F6__INCLUDED_)
