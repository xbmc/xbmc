/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "threads/SystemClock.h"
#include "SFTPFile.h"
#ifdef HAS_FILESYSTEM_SFTP
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "URL.h"
#include <fcntl.h>
#include <sstream>

#ifdef TARGET_WINDOWS
#pragma comment(lib, "ssh.lib")
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) ((m & _S_IFDIR) != 0)
#endif
#ifndef S_ISREG
#define S_ISREG(m) ((m & _S_IFREG) != 0)
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif

using namespace XFILE;
using namespace std;


static std::string CorrectPath(const std::string& path)
{
  if (path == "~")
    return "./";
  else if (path.substr(0, 2) == "~/")
    return "./" + path.substr(2);
  else
    return "/" + path;
}

static const char * SFTPErrorText(int sftp_error)
{
  switch(sftp_error)
  {
    case SSH_FX_OK:
      return "No error";
    case SSH_FX_EOF:
      return "End-of-file encountered";
    case SSH_FX_NO_SUCH_FILE:
      return "File doesn't exist";
    case SSH_FX_PERMISSION_DENIED:
      return "Permission denied";
    case SSH_FX_BAD_MESSAGE:
      return "Garbage received from server";
    case SSH_FX_NO_CONNECTION:
      return "No connection has been set up";
    case SSH_FX_CONNECTION_LOST:
      return "There was a connection, but we lost it";
    case SSH_FX_OP_UNSUPPORTED:
      return "Operation not supported by the server";
    case SSH_FX_INVALID_HANDLE:
      return "Invalid file handle";
    case SSH_FX_NO_SUCH_PATH:
      return "No such file or directory path exists";
    case SSH_FX_FILE_ALREADY_EXISTS:
      return "An attempt to create an already existing file or directory has been made";
    case SSH_FX_WRITE_PROTECT:
      return "We are trying to write on a write-protected filesystem";
    case SSH_FX_NO_MEDIA:
      return "No media in remote drive";
    case -1:
      return "Not a valid error code, probably called on an invalid session";
    default:
      CLog::Log(LOGERROR, "SFTPErrorText: Unknown error code: %d", sftp_error);
  }
  return "Unknown error code";
}



CSFTPSession::CSFTPSession(const std::string &host, unsigned int port, const std::string &username, const std::string &password)
{
  CLog::Log(LOGINFO, "SFTPSession: Creating new session on host '%s:%d' with user '%s'", host.c_str(), port, username.c_str());
  CSingleLock lock(m_critSect);
  if (!Connect(host, port, username, password))
    Disconnect();

  m_LastActive = XbmcThreads::SystemClockMillis();
}

CSFTPSession::~CSFTPSession()
{
  CSingleLock lock(m_critSect);
  Disconnect();
}

sftp_file CSFTPSession::CreateFileHande(const std::string &file)
{
  if (m_connected)
  {
    CSingleLock lock(m_critSect);
    m_LastActive = XbmcThreads::SystemClockMillis();
    sftp_file handle = sftp_open(m_sftp_session, CorrectPath(file).c_str(), O_RDONLY, 0);
    if (handle)
    {
      sftp_file_set_blocking(handle);
      return handle;
    }
    else
      CLog::Log(LOGERROR, "SFTPSession: Was connected but couldn't create filehandle for '%s'", file.c_str());
  }
  else
    CLog::Log(LOGERROR, "SFTPSession: Not connected and can't create file handle for '%s'", file.c_str());

  return NULL;
}

void CSFTPSession::CloseFileHandle(sftp_file handle)
{
  CSingleLock lock(m_critSect);
  sftp_close(handle);
}

bool CSFTPSession::GetDirectory(const std::string &base, const std::string &folder, CFileItemList &items)
{
  int sftp_error = SSH_FX_OK;
  if (m_connected)
  {
    sftp_dir dir = NULL;

    {
      CSingleLock lock(m_critSect);
      m_LastActive = XbmcThreads::SystemClockMillis();
      dir = sftp_opendir(m_sftp_session, CorrectPath(folder).c_str());

      //Doing as little work as possible within the critical section
      if (!dir)
        sftp_error = sftp_get_error(m_sftp_session);
    }

    if (!dir)
    {
      CLog::Log(LOGERROR, "%s: %s for '%s'", __FUNCTION__, SFTPErrorText(sftp_error), folder.c_str());
    }
    else
    {
      bool read = true;
      while (read)
      {
        sftp_attributes attributes = NULL;

        {
          CSingleLock lock(m_critSect);
          read = sftp_dir_eof(dir) == 0;
          attributes = sftp_readdir(m_sftp_session, dir);
        }

        if (attributes && (attributes->name == NULL || strcmp(attributes->name, "..") == 0 || strcmp(attributes->name, ".") == 0))
        {
          CSingleLock lock(m_critSect);
          sftp_attributes_free(attributes);
          continue;
        }
        
        if (attributes)
        {
          std::string itemName = attributes->name;
          std::string localPath = folder;
          localPath.append(itemName);

          if (attributes->type == SSH_FILEXFER_TYPE_SYMLINK)
          {
            CSingleLock lock(m_critSect);
            sftp_attributes_free(attributes);
            attributes = sftp_stat(m_sftp_session, CorrectPath(localPath).c_str());
            if (attributes == NULL)
              continue;
          }

          CFileItemPtr pItem(new CFileItem);
          pItem->SetLabel(itemName);

          if (itemName[0] == '.')
            pItem->SetProperty("file:hidden", true);

          if (attributes->flags & SSH_FILEXFER_ATTR_ACMODTIME)
            pItem->m_dateTime = attributes->mtime;

          if (attributes->type & SSH_FILEXFER_TYPE_DIRECTORY)
          {
            localPath.append("/");
            pItem->m_bIsFolder = true;
            pItem->m_dwSize = 0;
          }
          else
          {
            pItem->m_dwSize = attributes->size;
          }

          pItem->SetPath(base + localPath);
          items.Add(pItem);

          {
            CSingleLock lock(m_critSect);
            sftp_attributes_free(attributes);
          }
        }
        else
          read = false;
      }

      {
        CSingleLock lock(m_critSect);
        sftp_closedir(dir);
      }

      return true;
    }
  }
  else
    CLog::Log(LOGERROR, "SFTPSession: Not connected, can't list directory '%s'", folder.c_str());

  return false;
}

bool CSFTPSession::DirectoryExists(const char *path)
{
  uint32_t permissions = 0;
  bool exists = GetItemPermissions(path, permissions);
  return exists && S_ISDIR(permissions);
}

bool CSFTPSession::FileExists(const char *path)
{
  uint32_t permissions = 0;
  bool exists = GetItemPermissions(path, permissions);
  return exists && S_ISREG(permissions);
}

int CSFTPSession::Stat(const char *path, struct __stat64* buffer)
{
  CSingleLock lock(m_critSect);
  if(m_connected)
  {
    m_LastActive = XbmcThreads::SystemClockMillis();
    sftp_attributes attributes = sftp_stat(m_sftp_session, CorrectPath(path).c_str());

    if (attributes)
    {
      memset(buffer, 0, sizeof(struct __stat64));
      buffer->st_size = attributes->size;
      buffer->st_mtime = attributes->mtime;
      buffer->st_atime = attributes->atime;

      if S_ISDIR(attributes->permissions)
        buffer->st_mode = _S_IFDIR;
      else if S_ISREG(attributes->permissions)
        buffer->st_mode = _S_IFREG;

      sftp_attributes_free(attributes);
      return 0;
    }
    else
    {
      CLog::Log(LOGERROR, "SFTPSession::Stat - Failed to get attributes for '%s'", path);
      return -1;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "SFTPSession::Stat - Failed because not connected for '%s'", path);
    return -1;
  }
}

int CSFTPSession::Seek(sftp_file handle, uint64_t position)
{
  CSingleLock lock(m_critSect);
  m_LastActive = XbmcThreads::SystemClockMillis();
  return sftp_seek64(handle, position);
}

int CSFTPSession::Read(sftp_file handle, void *buffer, size_t length)
{
  CSingleLock lock(m_critSect);
  m_LastActive = XbmcThreads::SystemClockMillis();
  return sftp_read(handle, buffer, length);
}

int64_t CSFTPSession::GetPosition(sftp_file handle)
{
  CSingleLock lock(m_critSect);
  m_LastActive = XbmcThreads::SystemClockMillis();
  return sftp_tell64(handle);
}

bool CSFTPSession::IsIdle()
{
  return (XbmcThreads::SystemClockMillis() - m_LastActive) > 90000;
}

bool CSFTPSession::VerifyKnownHost(ssh_session session)
{
  switch (ssh_is_server_known(session))
  {
    case SSH_SERVER_KNOWN_OK:
      return true;
    case SSH_SERVER_KNOWN_CHANGED:
      CLog::Log(LOGERROR, "SFTPSession: Server that was known has changed");
      return false;
    case SSH_SERVER_FOUND_OTHER:
      CLog::Log(LOGERROR, "SFTPSession: The host key for this server was not found but an other type of key exists. An attacker might change the default server key to confuse your client into thinking the key does not exist");
      return false;
    case SSH_SERVER_FILE_NOT_FOUND:
      CLog::Log(LOGINFO, "SFTPSession: Server file was not found, creating a new one");
    case SSH_SERVER_NOT_KNOWN:
      CLog::Log(LOGINFO, "SFTPSession: Server unkown, we trust it for now");
      if (ssh_write_knownhost(session) < 0)
      {
        CLog::Log(LOGERROR, "CSFTPSession: Failed to save host '%s'", strerror(errno));
        return false;
      }

      return true;
    case SSH_SERVER_ERROR:
      CLog::Log(LOGERROR, "SFTPSession: Failed to verify host '%s'", ssh_get_error(session));
      return false;
  }

  return false;
}

bool CSFTPSession::Connect(const std::string &host, unsigned int port, const std::string &username, const std::string &password)
{
  int timeout     = SFTP_TIMEOUT;
  m_connected     = false;
  m_session       = NULL;
  m_sftp_session  = NULL;

  m_session=ssh_new();
  if (m_session == NULL)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to initialize session for host '%s'", host.c_str());
    return false;
  }

#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,4,0)
  if (ssh_options_set(m_session, SSH_OPTIONS_USER, username.c_str()) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set username '%s' for session", username.c_str());
    return false;
  }

  if (ssh_options_set(m_session, SSH_OPTIONS_HOST, host.c_str()) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set host '%s' for session", host.c_str());
    return false;
  }

  if (ssh_options_set(m_session, SSH_OPTIONS_PORT, &port) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set port '%d' for session", port);
    return false;
  }

  ssh_options_set(m_session, SSH_OPTIONS_LOG_VERBOSITY, 0);
  ssh_options_set(m_session, SSH_OPTIONS_TIMEOUT, &timeout);  
#else
  SSH_OPTIONS* options = ssh_options_new();

  if (ssh_options_set_username(options, username.c_str()) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set username '%s' for session", username.c_str());
    return false;
  }

  if (ssh_options_set_host(options, host.c_str()) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set host '%s' for session", host.c_str());
    return false;
  }

  if (ssh_options_set_port(options, port) < 0)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to set port '%d' for session", port);
    return false;
  }
  
  ssh_options_set_timeout(options, timeout, 0);

  ssh_options_set_log_verbosity(options, 0);

  ssh_set_options(m_session, options);
#endif

  if(ssh_connect(m_session))
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to connect '%s'", ssh_get_error(m_session));
    return false;
  }

  if (!VerifyKnownHost(m_session))
  {
    CLog::Log(LOGERROR, "SFTPSession: Host is not known '%s'", ssh_get_error(m_session));
    return false;
  }


  int noAuth = SSH_AUTH_DENIED;
  if ((noAuth = ssh_userauth_none(m_session, NULL)) == SSH_AUTH_ERROR)
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to authenticate via guest '%s'", ssh_get_error(m_session));
    return false;
  }

#if LIBSSH_VERSION_MINOR >= 6
  int method = ssh_userauth_list(m_session, NULL);
#else
  int method = ssh_auth_list(m_session);
#endif

  // Try to authenticate with public key first
  int publicKeyAuth = SSH_AUTH_DENIED;
#if LIBSSH_VERSION_MINOR >= 6
  if (method & SSH_AUTH_METHOD_PUBLICKEY && (publicKeyAuth = ssh_userauth_publickey_auto(m_session, NULL, NULL)) == SSH_AUTH_ERROR)
#else
  if (method & SSH_AUTH_METHOD_PUBLICKEY && (publicKeyAuth = ssh_userauth_autopubkey(m_session, NULL)) == SSH_AUTH_ERROR)
#endif
  {
    CLog::Log(LOGERROR, "SFTPSession: Failed to authenticate via publickey '%s'", ssh_get_error(m_session));
    return false;
  }

  // Try to authenticate with password
  int passwordAuth = SSH_AUTH_DENIED;
  if (method & SSH_AUTH_METHOD_PASSWORD)
  {
    if (publicKeyAuth != SSH_AUTH_SUCCESS &&
        (passwordAuth = ssh_userauth_password(m_session, username.c_str(), password.c_str())) == SSH_AUTH_ERROR)
      {
        CLog::Log(LOGERROR, "SFTPSession: Failed to authenticate via password '%s'", ssh_get_error(m_session));
        return false;
      }
  }
  else if (!password.empty())
  {
    CLog::Log(LOGERROR, "SFTPSession: Password present, but server does not support password authentication");
  }

  if (noAuth == SSH_AUTH_SUCCESS || publicKeyAuth == SSH_AUTH_SUCCESS || passwordAuth == SSH_AUTH_SUCCESS)
  {
    m_sftp_session = sftp_new(m_session);

    if (m_sftp_session == NULL)
    {
      CLog::Log(LOGERROR, "SFTPSession: Failed to initialize channel '%s'", ssh_get_error(m_session));
      return false;
    }

    if (sftp_init(m_sftp_session))
    {
      CLog::Log(LOGERROR, "SFTPSession: Failed to initialize sftp '%s'", ssh_get_error(m_session));
      return false;
    }

    m_connected = true;
  }
  else
  {
    CLog::Log(LOGERROR, "SFTPSession: No authentication method successful");
  }

  return m_connected;
}

void CSFTPSession::Disconnect()
{
  if (m_sftp_session)
    sftp_free(m_sftp_session);

  if (m_session)
    ssh_disconnect(m_session);

  m_sftp_session = NULL;
  m_session = NULL;
}

/*!
 \brief Gets POSIX compatible permissions information about the specified file or directory.
 \param path Remote SSH path to the file or directory.
 \param permissions POSIX compatible permissions information for the file or directory (if it exists). i.e. can use macros S_ISDIR() etc.
 \return Returns \e true, if it was possible to get permissions for the file or directory, \e false otherwise.
 */
bool CSFTPSession::GetItemPermissions(const char *path, uint32_t &permissions)
{
  bool gotPermissions = false;
  CSingleLock lock(m_critSect);
  if(m_connected)
  {
    sftp_attributes attributes = sftp_stat(m_sftp_session, CorrectPath(path).c_str());
    if (attributes)
    {
      if (attributes->flags & SSH_FILEXFER_ATTR_PERMISSIONS)
      {
        permissions = attributes->permissions;
        gotPermissions = true;
      }

      sftp_attributes_free(attributes);
    }
  }
  return gotPermissions;
}

CCriticalSection CSFTPSessionManager::m_critSect;
map<std::string, CSFTPSessionPtr> CSFTPSessionManager::sessions;

CSFTPSessionPtr CSFTPSessionManager::CreateSession(const CURL &url)
{
  string username = url.GetUserName().c_str();
  string password = url.GetPassWord().c_str();
  string hostname = url.GetHostName().c_str();
  unsigned int port = url.HasPort() ? url.GetPort() : 22;

  return CSFTPSessionManager::CreateSession(hostname, port, username, password);
}

CSFTPSessionPtr CSFTPSessionManager::CreateSession(const std::string &host, unsigned int port, const std::string &username, const std::string &password)
{
  // Convert port number to string
  stringstream itoa;
  itoa << port;
  std::string portstr = itoa.str();

  CSingleLock lock(m_critSect);
  std::string key = username + ':' + password + '@' + host + ':' + portstr;
  CSFTPSessionPtr ptr = sessions[key];
  if (ptr == NULL)
  {
    ptr = CSFTPSessionPtr(new CSFTPSession(host, port, username, password));
    sessions[key] = ptr;
  }

  return ptr;
}

void CSFTPSessionManager::ClearOutIdleSessions()
{
  CSingleLock lock(m_critSect);
  for(map<std::string, CSFTPSessionPtr>::iterator iter = sessions.begin(); iter != sessions.end();)
  {
    if (iter->second->IsIdle())
      sessions.erase(iter++);
    else
      ++iter;
  }
}

void CSFTPSessionManager::DisconnectAllSessions()
{
  CSingleLock lock(m_critSect);
  sessions.clear();
}

CSFTPFile::CSFTPFile()
{
  m_sftp_handle = NULL;
}

CSFTPFile::~CSFTPFile()
{
  Close();
}

bool CSFTPFile::Open(const CURL& url)
{
  m_session = CSFTPSessionManager::CreateSession(url);
  if (m_session)
  {
    m_file = url.GetFileName().c_str();
    m_sftp_handle = m_session->CreateFileHande(m_file);

    return (m_sftp_handle != NULL);
  }
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Failed to allocate session");
    return false;
  }
}

void CSFTPFile::Close()
{
  if (m_session && m_sftp_handle)
  {
    m_session->CloseFileHandle(m_sftp_handle);
    m_sftp_handle = NULL;
    m_session = CSFTPSessionPtr();
  }
}

int64_t CSFTPFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_session && m_sftp_handle)
  {
    uint64_t position = 0;
    if (iWhence == SEEK_SET)
      position = iFilePosition;
    else if (iWhence == SEEK_CUR)
      position = GetPosition() + iFilePosition;
    else if (iWhence == SEEK_END)
      position = GetLength() + iFilePosition;

    if (m_session->Seek(m_sftp_handle, position) == 0)
      return GetPosition();
    else
      return -1;
  }
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Can't seek without a filehandle");
    return -1;
  }
}

ssize_t CSFTPFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (m_session && m_sftp_handle)
  {
    if (uiBufSize > SSIZE_MAX)
      uiBufSize = SSIZE_MAX;

    int rc = m_session->Read(m_sftp_handle, lpBuf, (size_t)uiBufSize);

    if (rc >= 0)
      return rc;
    else
      CLog::Log(LOGERROR, "SFTPFile: Failed to read %i", rc);
  }
  else
    CLog::Log(LOGERROR, "SFTPFile: Can't read without a filehandle");

  return 0;
}

bool CSFTPFile::Exists(const CURL& url)
{
  CSFTPSessionPtr session = CSFTPSessionManager::CreateSession(url);
  if (session)
    return session->FileExists(url.GetFileName().c_str());
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Failed to create session to check exists for '%s'", url.GetFileName().c_str());
    return false;
  }
}

int CSFTPFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CSFTPSessionPtr session = CSFTPSessionManager::CreateSession(url);
  if (session)
    return session->Stat(url.GetFileName().c_str(), buffer);
  else
  {
    CLog::Log(LOGERROR, "SFTPFile: Failed to create session to stat for '%s'", url.GetFileName().c_str());
    return -1;
  }
}

int CSFTPFile::Stat(struct __stat64* buffer)
{
  if (m_session)
    return m_session->Stat(m_file.c_str(), buffer);

  CLog::Log(LOGERROR, "SFTPFile: Can't stat without a session for '%s'", m_file.c_str());
  return -1;
}

int64_t CSFTPFile::GetLength()
{
  struct __stat64 buffer;
  if (Stat(&buffer) != 0)
    return 0;
  else
  {
    int64_t length = buffer.st_size;
    return length;
  }
}

int64_t CSFTPFile::GetPosition()
{
  if (m_session && m_sftp_handle)
    return m_session->GetPosition(m_sftp_handle);

  CLog::Log(LOGERROR, "SFTPFile: Can't get position without a filehandle for '%s'", m_file.c_str());
  return 0;
}

int CSFTPFile::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
    return 1;

  return -1;
}

#endif
