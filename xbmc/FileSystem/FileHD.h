// FileHD.h: interface for the CFileHD class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
#define AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

namespace XFILE
{
class CFileHD : public IFile
{
public:
  CFileHD();
  virtual ~CFileHD();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual int Write(const void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual void Flush();

  virtual bool OpenForWrite(const CURL& url, bool bBinary = true, bool bOverWrite = false);

  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);
protected:
  CStdString GetLocal(const CURL &url); /* crate a properly format path from an url */
  CAutoPtrHandle m_hFile;
  __int64 m_i64FileLength;
  __int64 m_i64FilePos;
};

}
#endif // !defined(AFX_FILEHD_H__DD2B0A9E_4971_4A29_B525_78CEFCDAF4A1__INCLUDED_)
