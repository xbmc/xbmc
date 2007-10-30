// FileSmb.h: interface for the CFileSMB class.

//

//////////////////////////////////////////////////////////////////////



#if !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)

#define AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_


#if _MSC_VER > 1000

#pragma once

#endif // _MSC_VER > 1000

#include "IFile.h"

#define NT_STATUS_CONNECTION_REFUSED long(0xC0000000 | 0x0236)
#define NT_STATUS_INVALID_HANDLE long(0xC0000000 | 0x0008)
#define NT_STATUS_ACCESS_DENIED long(0xC0000000 | 0x0022)
#define NT_STATUS_OBJECT_NAME_NOT_FOUND long(0xC0000000 | 0x0034)
#define NT_STATUS_INVALID_COMPUTER_NAME long(0xC0000000 | 0x0122)

struct _SMBCCTX;
typedef _SMBCCTX SMBCCTX;

class CSMB : public CCriticalSection
{
public:
  CSMB();
  ~CSMB();
  void Init();
  void Deinit();
  void Purge();
  void PurgeEx(const CURL& url);
  
  CStdString URLEncode(const CStdString &value);
  CStdString URLEncode(const CURL &url);

  DWORD ConvertUnixToNT(int error);
private:
  SMBCCTX *m_context;
  CStdString m_strLastHost;
  CStdString m_strLastShare;
};

extern CSMB smb;

namespace XFILE
{
class CFileSMB : public IFile
{
public:
  CFileSMB();
  int OpenFile(const CURL &url, CStdString& strAuth);
  virtual ~CFileSMB();
  virtual void Close();
  virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
  virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
  virtual bool Open(const CURL& url, bool bBinary = true);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual __int64 GetLength();
  virtual __int64 GetPosition();
  virtual int Write(const void* lpBuf, __int64 uiBufSize);

  virtual bool OpenForWrite(const CURL& url, bool bBinary = true, bool bOverWrite = false);
  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);

protected:
  CURL m_url;
  bool IsValidFile(const CStdString& strFileName);  
  __int64 m_fileSize;
  bool m_bBinary;
  int m_fd;
};
};

#endif // !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)
