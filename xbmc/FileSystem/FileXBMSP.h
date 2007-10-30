// FileXBMSP.h: interface for the CFileXBMSP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEXMBMSP_H___INCLUDED_)
#define AFX_FILEXMBMSP_H___INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"

extern "C"
{
#include "../lib/libXBMS/ccincludes.h"
#include "../lib/libXBMS/ccxclient.h"
}

namespace XFILE
{

class CFileXBMSP : public IFile
{
public:
  CFileXBMSP();
  virtual ~CFileXBMSP();
  virtual __int64 GetPosition();
  virtual __int64 GetLength();
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
protected:
  UINT64 m_fileSize;
  UINT64 m_filePos;
  SOCKET m_socket;
private:
  CcXstreamServerConnection m_connection;
  unsigned long m_handle;
  bool m_bOpened;

};
};

#endif // !defined(AFX_FILEXMBMSP_H___INCLUDED_)
