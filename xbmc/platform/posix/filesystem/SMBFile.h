/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// SMBFile.h: interface for the CSMBFile class.

//

//////////////////////////////////////////////////////////////////////


#include "URL.h"
#include "filesystem/IFile.h"
#include "threads/CriticalSection.h"

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
  /* Makes sense to be called after acquiring the lock */
  bool IsSmbValid() const { return m_context != nullptr; }
  void CheckIfIdle();
  void SetActivityTime();
  void AddActiveConnection();
  void AddIdleConnection();
  std::string URLEncode(const std::string &value);
  std::string URLEncode(const CURL &url);

  DWORD ConvertUnixToNT(int error);
  static CURL GetResolvedUrl(const CURL& url);

private:
  SMBCCTX *m_context;
  int m_OpenConnections;
  unsigned int m_IdleTimeout;
  static bool IsFirstInit;
};

extern CSMB smb;

namespace XFILE
{
class CSMBFile : public IFile
{
public:
  CSMBFile();
  int OpenFile(const CURL &url, std::string& strAuth);
  ~CSMBFile() override;
  void Close() override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  bool Open(const CURL& url) override;
  bool Exists(const CURL& url) override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  int Stat(struct __stat64* buffer) override;
  int Truncate(int64_t size) override;
  int64_t GetLength() override;
  int64_t GetPosition() override;
  ssize_t Write(const void* lpBuf, size_t uiBufSize) override;

  bool OpenForWrite(const CURL& url, bool bOverWrite = false) override;
  bool Delete(const CURL& url) override;
  bool Rename(const CURL& url, const CURL& urlnew) override;
  int GetChunkSize() override;
  int IoControl(EIoControl request, void* param) override;

protected:
  CURL m_url;
  bool IsValidFile(const std::string& strFileName);
  std::string GetAuthenticatedPath(const CURL &url);
  int64_t m_fileSize;
  int m_fd;
  bool m_allowRetry;
};
}
