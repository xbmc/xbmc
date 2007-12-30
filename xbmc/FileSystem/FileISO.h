// FileISO.h: interface for the CFileISO class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_)
#define AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "iso9660.h"
#include "../xbox/IoSupport.h"
#include "RingBuffer.h"

namespace XFILE
{

class CFileISO : public IFile
{
public:
  CFileISO();
  virtual ~CFileISO();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
protected:
  bool m_bOpened;
  HANDLE m_hFile;
  CRingBuffer m_cache;
};
}

#endif // !defined(AFX_FILEISO_H__C2FB9C6D_3319_4182_AB45_65E57EFAC8D1__INCLUDED_)
