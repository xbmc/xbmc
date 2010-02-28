#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "system.h"
#ifdef HAS_FILESYSTEM_SFTP
#include "IFile.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/CriticalSection.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

class CSFTPSession
{
public:
  CSFTPSession(const CStdString &host, const CStdString &username, const CStdString &password);
  virtual ~CSFTPSession();

  SFTP_FILE *CreateFileHande(const CStdString &file);
  void CloseFileHandle(SFTP_FILE *handle);
  bool GetDirectory(const CStdString &base, const CStdString &folder, CFileItemList &items);
  bool Exists(const char *path);
  int Stat(const char *path, struct __stat64* buffer);
  void Seek(SFTP_FILE *handle, u64 position);
  int Read(SFTP_FILE *handle, void *buffer, int64_t length);
  int64_t GetPosition(SFTP_FILE *handle);
  bool IsIdle();
private:
  bool VerifyKnownHost(ssh_session *session);
  bool Connect(const CStdString &host, const CStdString &username, const CStdString &password);
  void Disconnect();
  CCriticalSection m_critSect;

  bool m_connected;
  ssh_session  *m_session;
  SFTP_SESSION *m_sftp_session;
  int m_LastActive;
};

typedef boost::shared_ptr<CSFTPSession> CSFTPSessionPtr;

class CSFTPSessionManager
{
public:
  static CSFTPSessionPtr CreateSession(const CStdString &host, const CStdString &username, const CStdString &password);
  static void ClearOutIdleSessions();
  static void DisconnectAllSessions();
private:
  static CCriticalSection m_critSect;
  static std::map<CStdString, CSFTPSessionPtr> sessions;
};

namespace XFILE
{
  class CFileSFTP : public IFile
  {
  public:
    CFileSFTP();
    virtual ~CFileSFTP();
    virtual void Close();
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
    virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual int Stat(struct __stat64* buffer);
    virtual int64_t GetLength();
    virtual int64_t GetPosition();

  private:
    CStdString m_file;
    CSFTPSessionPtr m_session;
    SFTP_FILE *m_sftp_handle;
  };
}
#endif
