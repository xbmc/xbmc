/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// FileSmb.h: interface for the CSmbFile class.

//

//////////////////////////////////////////////////////////////////////



#if !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)

#define AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_


#if _MSC_VER > 1000

#pragma once

#endif // _MSC_VER > 1000

#include "IFile.h"
#include "URL.h"
#include "threads/CriticalSection.h"

#define NT_STATUS_CONNECTION_REFUSED long(0xC0000000 | 0x0236)
#define NT_STATUS_INVALID_HANDLE long(0xC0000000 | 0x0008)
#define NT_STATUS_ACCESS_DENIED long(0xC0000000 | 0x0022)
#define NT_STATUS_OBJECT_NAME_NOT_FOUND long(0xC0000000 | 0x0034)
#ifdef _LINUX
#define NT_STATUS_INVALID_COMPUTER_NAME long(0xC0000000 | 0x0122)
#endif

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
#ifdef _LINUX
  void CheckIfIdle();
  void SetActivityTime();
  void AddActiveConnection();
  void AddIdleConnection();
#endif
  CStdString URLEncode(const CStdString &value);
  CStdString URLEncode(const CURL &url);

  DWORD ConvertUnixToNT(int error);
private:
  SMBCCTX *m_context;
  CStdString m_strLastHost;
  CStdString m_strLastShare;
#ifdef _LINUX
  int m_OpenConnections;
  unsigned int m_IdleTimeout;
#endif
};

extern CSMB smb;

namespace XFILE
{
class CSmbFile : public IFile
{
public:
  CSmbFile();
  int OpenFile(const CURL &url, CStdString& strAuth);
  virtual ~CSmbFile();
  virtual void Close();
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual bool Open(const CURL& url);
  virtual bool Exists(const CURL& url);
  virtual int Stat(const CURL& url, struct __stat64* buffer);
  virtual int Stat(struct __stat64* buffer);
  virtual int Truncate(int64_t size);
  virtual int64_t GetLength();
  virtual int64_t GetPosition();
  virtual int Write(const void* lpBuf, int64_t uiBufSize);

  virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
  virtual bool Delete(const CURL& url);
  virtual bool Rename(const CURL& url, const CURL& urlnew);
  virtual int  GetChunkSize() {return 1;}

protected:
  CURL m_url;
  bool IsValidFile(const CStdString& strFileName);
  CStdString GetAuthenticatedPath(const CURL &url);
  int64_t m_fileSize;
  int m_fd;
};
}

#endif // !defined(AFX_FILESMB_H__2C4AB5BC_0742_458D_95EA_E9C77BA5663D__INCLUDED_)
