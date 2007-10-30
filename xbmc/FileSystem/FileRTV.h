// FileRTV.h: interface for the CFileRTV class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILERTV_H___INCLUDED_)
#define AFX_FILERTV_H___INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

extern "C"
{
#include "../lib/libRTV/interface.h"
}

namespace XFILE
{

class CFileRTV : public IFile
{
public:
  CFileRTV();
  virtual ~CFileRTV();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  bool Open(const char* strHostName, const char* strFileName, int iport, bool bBinary);
  virtual bool Exists(const CURL& url) { return true;};
  virtual int Stat(const CURL& url, struct __stat64* buffer) { errno = ENOENT; return -1; };
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
protected:
  UINT64 m_fileSize;
  UINT64 m_filePos;
  char m_hostName[255];
  char m_fileName[255];
  int m_iport;
private:
  RTVD m_rtvd;
  bool m_bOpened;

};
};

#endif // !defined(AFX_FILERTV_H___INCLUDED_)
