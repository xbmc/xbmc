/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>
#include "IFile.h"
#include "platform/threads/mutex.h"
#include "os-dependent.h"

struct _SMBCCTX;
typedef _SMBCCTX SMBCCTX;

namespace PLATFORM
{

class CSMB: public CMutex
{
public:
  CSMB();
  ~CSMB();
  void Init();
  void Deinit();
  void Purge();
  //void PurgeEx(const CURL& url);
#ifdef TARGET_LINUX
  void CheckIfIdle();
  void SetActivityTime();
  void AddActiveConnection();
  void AddIdleConnection();
#endif
  CStdString URLEncode(const CStdString &value);
  //CStdString URLEncode(const CURL &url);

  //DWORD ConvertUnixToNT(int error);
private:
  SMBCCTX *m_context;
  CStdString m_strLastHost;
  CStdString m_strLastShare;
#ifdef TARGET_LINUX
  int m_OpenConnections;
  unsigned int m_IdleTimeout;
#endif
};

extern CSMB smb;
  
class CFile: public IFile
{
public:
  CFile();
  ~CFile();

  bool Open(const CStdString& strFileName, unsigned int flags = 0);
  bool IsInvalid();
  unsigned long Read(void* lpBuf, int64_t uiBufSize);
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int64_t GetPosition();
  int64_t GetLength();
  void Close();
  int Stat(struct __stat64 *buffer);
  static bool Exists(const CStdString& strFileName, bool bUseCache = true);

private:
  int OpenFile();
  
  int64_t m_fileSize;
  int m_fd;
  CStdString m_fileName;

};

} // namespace PLATFORM
