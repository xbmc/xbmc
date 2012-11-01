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

// FileSmb.cpp: implementation of the CSmbFile class.
//
//////////////////////////////////////////////////////////////////////

#include "system.h"
#include "SmbFile.h"
#include "PasswordManager.h"
#include "SMBDirectory.h"
#include "Util.h"
#include <libsmbclient.h>
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "commons/Exception.h"

using namespace XFILE;

void xb_smbc_log(const char* msg)
{
  CLog::Log(LOGINFO, "%s%s", "smb: ", msg);
}

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen,
                  char *un, int unlen, char *pw, int pwlen)
{
  return ;
}

smbc_get_cached_srv_fn orig_cache;

SMBCSRV* xb_smbc_cache(SMBCCTX* c, const char* server, const char* share, const char* workgroup, const char* username)
{
  return orig_cache(c, server, share, workgroup, username);
}

CSMB::CSMB()
{
#ifdef TARGET_POSIX
  m_IdleTimeout = 0;
#endif
  m_context = NULL;
}

CSMB::~CSMB()
{
  Deinit();
}

void CSMB::Deinit()
{
  CSingleLock lock(*this);

  /* samba goes loco if deinited while it has some files opened */
  if (m_context)
  {
    try
    {
      smbc_set_context(NULL);
      smbc_free_context(m_context, 1);
    }
    XBMCCOMMONS_HANDLE_UNCHECKED
    catch(...)
    {
      CLog::Log(LOGERROR,"exception on CSMB::Deinit. errno: %d", errno);
    }
    m_context = NULL;
  }
}

void CSMB::Init()
{
  CSingleLock lock(*this);
  if (!m_context)
  {
#ifdef TARGET_POSIX
    // Create ~/.smb/smb.conf. This file is used by libsmbclient.
    // http://us1.samba.org/samba/docs/man/manpages-3/libsmbclient.7.html
    // http://us1.samba.org/samba/docs/man/manpages-3/smb.conf.5.html
    char smb_conf[MAX_PATH];
    snprintf(smb_conf, sizeof(smb_conf), "%s/.smb", getenv("HOME"));
    if (mkdir(smb_conf, 0755) == 0)
    {
      snprintf(smb_conf, sizeof(smb_conf), "%s/.smb/smb.conf", getenv("HOME"));
      FILE* f = fopen(smb_conf, "w");
      if (f != NULL)
      {
        fprintf(f, "[global]\n");

        // make sure we're not acting like a server
        fprintf(f, "\tpreferred master = no\n");
        fprintf(f, "\tlocal master = no\n");
        fprintf(f, "\tdomain master = no\n");

        // use the weaker LANMAN password hash in order to be compatible with older servers
        fprintf(f, "\tclient lanman auth = yes\n");
        fprintf(f, "\tlanman auth = yes\n");

        fprintf(f, "\tsocket options = TCP_NODELAY IPTOS_LOWDELAY SO_RCVBUF=65536 SO_SNDBUF=65536\n");      
        fprintf(f, "\tlock directory = %s/.smb/\n", getenv("HOME"));

        // set wins server if there's one. name resolve order defaults to 'lmhosts host wins bcast'.
        // if no WINS server has been specified the wins method will be ignored.
        if ( g_guiSettings.GetString("smb.winsserver").length() > 0 && !g_guiSettings.GetString("smb.winsserver").Equals("0.0.0.0") )
        {
          fprintf(f, "\twins server = %s\n", g_guiSettings.GetString("smb.winsserver").c_str());
          fprintf(f, "\tname resolve order = bcast wins host\n");
        }
        else
          fprintf(f, "\tname resolve order = bcast host\n");

        // use user-configured charset. if no charset is specified,
        // samba tries to use charset 850 but falls back to ASCII in case it is not available
        if (g_advancedSettings.m_sambadoscodepage.length() > 0)
          fprintf(f, "\tdos charset = %s\n", g_advancedSettings.m_sambadoscodepage.c_str());

        // if no workgroup string is specified, samba will use the default value 'WORKGROUP'
        if ( g_guiSettings.GetString("smb.workgroup").length() > 0 )
          fprintf(f, "\tworkgroup = %s\n", g_guiSettings.GetString("smb.workgroup").c_str());
        fclose(f);
      }
    }
#endif

    // reads smb.conf so this MUST be after we create smb.conf
    // multiple smbc_init calls are ignored by libsmbclient.
    smbc_init(xb_smbc_auth, 0);

#ifdef TARGET_WINDOWS
    // set the log function
    set_log_callback(xb_smbc_log);
#endif

    // setup our context
    m_context = smbc_new_context();
#ifdef DEPRECATED_SMBC_INTERFACE
    smbc_setDebug(m_context, g_advancedSettings.m_logLevel == LOG_LEVEL_DEBUG_SAMBA ? 10 : 0);
    smbc_setFunctionAuthData(m_context, xb_smbc_auth);
    orig_cache = smbc_getFunctionGetCachedServer(m_context);
    smbc_setFunctionGetCachedServer(m_context, xb_smbc_cache);
    smbc_setOptionOneSharePerServer(m_context, false);
    smbc_setOptionBrowseMaxLmbCount(m_context, 0);
    smbc_setTimeout(m_context, g_advancedSettings.m_sambaclienttimeout * 1000);
    smbc_setUser(m_context, strdup("guest"));
#else
    m_context->debug = g_advancedSettings.m_logLevel == LOG_LEVEL_DEBUG_SAMBA ? 10 : 0;
    m_context->callbacks.auth_fn = xb_smbc_auth;
    orig_cache = m_context->callbacks.get_cached_srv_fn;
    m_context->callbacks.get_cached_srv_fn = xb_smbc_cache;
    m_context->options.one_share_per_server = false;
    m_context->options.browse_max_lmb_count = 0;
    m_context->timeout = g_advancedSettings.m_sambaclienttimeout * 1000;
    m_context->user = strdup("guest");
#endif

    // initialize samba and do some hacking into the settings
    if (smbc_init_context(m_context))
    {
      /* setup old interface to use this context */
      smbc_set_context(m_context);

#ifdef TARGET_WINDOWS
      // if a wins-server is set, we have to change name resolve order to
      if ( g_guiSettings.GetString("smb.winsserver").length() > 0 && !g_guiSettings.GetString("smb.winsserver").Equals("0.0.0.0") )
      {
        lp_do_parameter( -1, "wins server", g_guiSettings.GetString("smb.winsserver").c_str());
        lp_do_parameter( -1, "name resolve order", "bcast wins host");
      }
      else
        lp_do_parameter( -1, "name resolve order", "bcast host");

      if (g_advancedSettings.m_sambadoscodepage.length() > 0)
        lp_do_parameter( -1, "dos charset", g_advancedSettings.m_sambadoscodepage.c_str());
      else
        lp_do_parameter( -1, "dos charset", "CP850");
#endif
    }
    else
    {
      smbc_free_context(m_context, 1);
      m_context = NULL;
    }
  }
#ifdef TARGET_POSIX
  m_IdleTimeout = 180;
#endif
}

void CSMB::Purge()
{
#ifdef TARGET_WINDOWS
  CSingleLock lock(*this);
  smbc_purge();
#endif
}

/*
 * For each new connection samba creates a new session
 * But this is not what we want, we just want to have one session at the time
 * This means that we have to call smbc_purge() if samba created a new session
 * Samba will create a new session when:
 * - connecting to another server
 * - connecting to another share on the same server (share, not a different folder!)
 *
 * We try to avoid lot's of purge commands because it slow samba down.
 */
void CSMB::PurgeEx(const CURL& url)
{
  CSingleLock lock(*this);
  CStdString strShare = url.GetFileName().substr(0, url.GetFileName().Find('/'));

#ifdef TARGET_WINDOWS
  if (m_strLastShare.length() > 0 && (m_strLastShare != strShare || m_strLastHost != url.GetHostName()))
    smbc_purge();
#endif

  m_strLastShare = strShare;
  m_strLastHost = url.GetHostName();
}

CStdString CSMB::URLEncode(const CURL &url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  CStdString flat = "smb://";

  if(url.GetDomain().length() > 0)
  {
    flat += URLEncode(url.GetDomain());
    flat += ";";
  }

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    flat += URLEncode(url.GetUserName());
    flat += ":";
    flat += URLEncode(url.GetPassWord());
    flat += "@";
  }
  flat += URLEncode(url.GetHostName());

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<CStdString> parts;
  std::vector<CStdString>::iterator it;
  CUtil::Tokenize(url.GetFileName(), parts, "/");
  for( it = parts.begin(); it != parts.end(); it++ )
  {
    flat += "/";
    flat += URLEncode((*it));
  }

  /* okey options should go here, thou current samba doesn't support any */

  return flat;
}

CStdString CSMB::URLEncode(const CStdString &value)
{
  CStdString encoded(value);
  CURL::Encode(encoded);
  return encoded;
}

#ifdef TARGET_WINDOWS
DWORD CSMB::ConvertUnixToNT(int error)
{
  DWORD nt_error;
  if (error == ENODEV || error == ENETUNREACH || error == WSAETIMEDOUT) nt_error = NT_STATUS_INVALID_COMPUTER_NAME;
  else if(error == WSAECONNREFUSED || error == WSAECONNABORTED) nt_error = NT_STATUS_CONNECTION_REFUSED;
  else nt_error = map_nt_error_from_unix(error);

  return nt_error;
}
#endif

#ifdef TARGET_POSIX
/* This is called from CApplication::ProcessSlow() and is used to tell if smbclient have been idle for too long */
void CSMB::CheckIfIdle()
{
/* We check if there are open connections. This is done without a lock to not halt the mainthread. It should be thread safe as
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if wich will lead to another check, wich is locked.  */
  if (m_OpenConnections == 0)
  { /* I've set the the maxiumum IDLE time to be 1 min and 30 sec. */
    CSingleLock lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */ && m_context != NULL)
    {
      if (m_IdleTimeout > 0)
	  {
        m_IdleTimeout--;
      }
	  else
	  {
        CLog::Log(LOGNOTICE, "Samba is idle. Closing the remaining connections");
        smb.Deinit();
      }
    }
  }
}

void CSMB::SetActivityTime()
{
  /* Since we get called every 500ms from ProcessSlow we limit the tick count to 180 */
  /* That means we have 2 ticks per second which equals 180/2 == 90 seconds */
  m_IdleTimeout = 180;
}

/* The following two function is used to keep track on how many Opened files/directories there are.
   This makes the idle timer not count if a movie is paused for example */
void CSMB::AddActiveConnection()
{
  CSingleLock lock(*this);
  m_OpenConnections++;
}
void CSMB::AddIdleConnection()
{
  CSingleLock lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any wierd behaviours if a user
     leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;
}
#endif

CSMB smb;

CSmbFile::CSmbFile()
{
  smb.Init();
  m_fd = -1;
#ifdef TARGET_POSIX
  smb.AddActiveConnection();
#endif
}

CSmbFile::~CSmbFile()
{
  Close();
#ifdef TARGET_POSIX
  smb.AddIdleConnection();
#endif
}

int64_t CSmbFile::GetPosition()
{
  if (m_fd == -1) return 0;
  smb.Init();
  CSingleLock lock(smb);
  int64_t pos = smbc_lseek(m_fd, 0, SEEK_CUR);
  if ( pos < 0 )
    return 0;
  return pos;
}

int64_t CSmbFile::GetLength()
{
  if (m_fd == -1) return 0;
  return m_fileSize;
}

bool CSmbFile::Open(const CURL& url)
{
  Close();

  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName()))
  {
      CLog::Log(LOGNOTICE,"FileSmb->Open: Bad URL : '%s'",url.GetFileName().c_str());
      return false;
  }
  m_url = url;

  // opening a file to another computer share will create a new session
  // when opening smb://server xbms will try to find folder.jpg in all shares
  // listed, which will create lot's of open sessions.

  CStdString strFileName;
  m_fd = OpenFile(url, strFileName);

  CLog::Log(LOGDEBUG,"CSmbFile::Open - opened %s, fd=%d",url.GetFileName().c_str(), m_fd);
  if (m_fd == -1)
  {
    // write error to logfile
#ifdef TARGET_WINDOWS
    int nt_error = smb.ConvertUnixToNT(errno);
    CLog::Log(LOGINFO, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'", strFileName.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
#else
    CLog::Log(LOGINFO, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strFileName.c_str(), errno, strerror(errno));
#endif
    return false;
  }

  CSingleLock lock(smb);
#ifdef TARGET_WINDOWS
  struct __stat64 tmpBuffer = {0};
#else
  struct stat tmpBuffer;
#endif
  if (smbc_stat(strFileName, &tmpBuffer) < 0)
  {
    smbc_close(m_fd);
    m_fd = -1;
    return false;
  }

  m_fileSize = tmpBuffer.st_size;

  int64_t ret = smbc_lseek(m_fd, 0, SEEK_SET);
  if ( ret < 0 )
  {
    smbc_close(m_fd);
    m_fd = -1;
    return false;
  }
  // We've successfully opened the file!
  return true;
}


/// \brief Checks authentication against SAMBA share. Reads password cache created in CSMBDirectory::OpenDir().
/// \param strAuth The SMB style path
/// \return SMB file descriptor
/*
int CSmbFile::OpenFile(CStdString& strAuth)
{
  int fd = -1;

  CStdString strPath = g_passwordManager.GetSMBAuthFilename(strAuth);

  fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
  // TODO: Run a loop here that prompts for our username/password as appropriate?
  // We have the ability to run a file (eg from a button action) without browsing to
  // the directory first.  In the case of a password protected share that we do
  // not have the authentication information for, the above smbc_open() will have
  // returned negative, and the file will not be opened.  While this is not a particular
  // likely scenario, we might want to implement prompting for the password in this case.
  // The code from SMBDirectory can be used for this.
  if(fd >= 0)
    strAuth = strPath;

  return fd;
}
*/

int CSmbFile::OpenFile(const CURL &url, CStdString& strAuth)
{
  int fd = -1;
  smb.Init();

  strAuth = GetAuthenticatedPath(url);
  CStdString strPath = strAuth;

  {
    CSingleLock lock(smb);
    fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
  }

  if (fd >= 0)
    strAuth = strPath;

  return fd;
}

bool CSmbFile::Exists(const CURL& url)
{
  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName())) return false;

  smb.Init();
  CStdString strFileName = GetAuthenticatedPath(url);

#ifdef TARGET_WINDOWS
  struct __stat64 info;
#else
  struct stat info;
#endif

  CSingleLock lock(smb);
  int iResult = smbc_stat(strFileName, &info);

  if (iResult < 0) return false;
  return true;
}

int CSmbFile::Stat(struct __stat64* buffer)
{
  if (m_fd == -1)
    return -1;

#ifdef TARGET_WINDOWS
  struct __stat64 tmpBuffer = {0};
#else
  struct stat tmpBuffer = {0};
#endif

  CSingleLock lock(smb);
  int iResult = smbc_fstat(m_fd, &tmpBuffer);

  memset(buffer, 0, sizeof(struct __stat64));
  buffer->st_dev = tmpBuffer.st_dev;
  buffer->st_ino = tmpBuffer.st_ino;
  buffer->st_mode = tmpBuffer.st_mode;
  buffer->st_nlink = tmpBuffer.st_nlink;
  buffer->st_uid = tmpBuffer.st_uid;
  buffer->st_gid = tmpBuffer.st_gid;
  buffer->st_rdev = tmpBuffer.st_rdev;
  buffer->st_size = tmpBuffer.st_size;
  buffer->st_atime = tmpBuffer.st_atime;
  buffer->st_mtime = tmpBuffer.st_mtime;
  buffer->st_ctime = tmpBuffer.st_ctime;

  return iResult;
}

int CSmbFile::Stat(const CURL& url, struct __stat64* buffer)
{
  smb.Init();
  CStdString strFileName = GetAuthenticatedPath(url);
  CSingleLock lock(smb);

#ifdef TARGET_WINDOWS
  struct __stat64 tmpBuffer = {0};
#else
  struct stat tmpBuffer = {0};
#endif
  int iResult = smbc_stat(strFileName, &tmpBuffer);

  memset(buffer, 0, sizeof(struct __stat64));
  buffer->st_dev = tmpBuffer.st_dev;
  buffer->st_ino = tmpBuffer.st_ino;
  buffer->st_mode = tmpBuffer.st_mode;
  buffer->st_nlink = tmpBuffer.st_nlink;
  buffer->st_uid = tmpBuffer.st_uid;
  buffer->st_gid = tmpBuffer.st_gid;
  buffer->st_rdev = tmpBuffer.st_rdev;
  buffer->st_size = tmpBuffer.st_size;
  buffer->st_atime = tmpBuffer.st_atime;
  buffer->st_mtime = tmpBuffer.st_mtime;
  buffer->st_ctime = tmpBuffer.st_ctime;

  return iResult;
}

int CSmbFile::Truncate(int64_t size)
{
  if (m_fd == -1) return 0;
/* 
 * This would force us to be dependant on SMBv3.2 which is GPLv3
 * This is only used by the TagLib writers, which are not currently in use
 * So log and warn until we implement TagLib writing & can re-implement this better.
  CSingleLock lock(smb); // Init not called since it has to be "inited" by now

#if defined(TARGET_ANDROID)
  int iResult = 0;
#else
  int iResult = smbc_ftruncate(m_fd, size);
#endif
*/
  CLog::Log(LOGWARNING, "%s - Warning(smbc_ftruncate called and not implemented)", __FUNCTION__);
  return 0;
}

unsigned int CSmbFile::Read(void *lpBuf, int64_t uiBufSize)
{
  if (m_fd == -1) return 0;
  CSingleLock lock(smb); // Init not called since it has to be "inited" by now
#ifdef TARGET_POSIX
  smb.SetActivityTime();
#endif
  /* work around stupid bug in samba */
  /* some samba servers has a bug in it where the */
  /* 17th bit will be ignored in a request of data */
  /* this can lead to a very small return of data */
  /* also worse, a request of exactly 64k will return */
  /* as if eof, client has a workaround for windows */
  /* thou it seems other servers are affected too */
  if( uiBufSize >= 64*1024-2 )
    uiBufSize = 64*1024-2;

  int bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);

  if ( bytesRead < 0 && errno == EINVAL )
  {
    CLog::Log(LOGERROR, "%s - Error( %d, %d, %s ) - Retrying", __FUNCTION__, bytesRead, errno, strerror(errno));
    bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
  }

  if ( bytesRead < 0 )
  {
#ifdef TARGET_WINDOWS
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %d, %d, %s )", __FUNCTION__, bytesRead, errno, strerror(errno));
#endif
    return 0;
  }

  return (unsigned int)bytesRead;
}

int64_t CSmbFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fd == -1) return -1;

  CSingleLock lock(smb); // Init not called since it has to be "inited" by now
#ifdef TARGET_POSIX
  smb.SetActivityTime();
#endif
  int64_t pos = smbc_lseek(m_fd, iFilePosition, iWhence);

  if ( pos < 0 )
  {
#ifdef TARGET_WINDOWS
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %"PRId64", %d, %s )", __FUNCTION__, pos, errno, strerror(errno));
#endif
    return -1;
  }

  return (int64_t)pos;
}

void CSmbFile::Close()
{
  if (m_fd != -1)
  {
    CLog::Log(LOGDEBUG,"CSmbFile::Close closing fd %d", m_fd);
    CSingleLock lock(smb);
    smbc_close(m_fd);
  }
  m_fd = -1;
}

int CSmbFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  if (m_fd == -1) return -1;
  DWORD dwNumberOfBytesWritten = 0;

  // lpBuf can be safely casted to void* since xmbc_write will only read from it.
  smb.Init();
  CSingleLock lock(smb);
  dwNumberOfBytesWritten = smbc_write(m_fd, (void*)lpBuf, (DWORD)uiBufSize);

  return (int)dwNumberOfBytesWritten;
}

bool CSmbFile::Delete(const CURL& url)
{
  smb.Init();
  CStdString strFile = GetAuthenticatedPath(url);

  CSingleLock lock(smb);

  int result = smbc_unlink(strFile.c_str());

  if(result != 0)
#ifdef TARGET_WINDOWS
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0);
}

bool CSmbFile::Rename(const CURL& url, const CURL& urlnew)
{
  smb.Init();
  CStdString strFile = GetAuthenticatedPath(url);
  CStdString strFileNew = GetAuthenticatedPath(urlnew);
  CSingleLock lock(smb);

  int result = smbc_rename(strFile.c_str(), strFileNew.c_str());

  if(result != 0)
#ifdef TARGET_WINDOWS
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0);
}

bool CSmbFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  m_fileSize = 0;

  Close();
  smb.Init();
  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName())) return false;

  CStdString strFileName = GetAuthenticatedPath(url);
  CSingleLock lock(smb);

  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "FileSmb::OpenForWrite() called with overwriting enabled! - %s", strFileName.c_str());
    m_fd = smbc_creat(strFileName.c_str(), 0);
  }
  else
  {
    m_fd = smbc_open(strFileName.c_str(), O_RDWR, 0);
  }

  if (m_fd == -1)
  {
    // write error to logfile
#ifdef TARGET_WINDOWS
    int nt_error = map_nt_error_from_unix(errno);
    CLog::Log(LOGERROR, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'", strFileName.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
#else
    CLog::Log(LOGERROR, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strFileName.c_str(), errno, strerror(errno));
#endif
    return false;
  }

  // We've successfully opened the file!
  return true;
}

bool CSmbFile::IsValidFile(const CStdString& strFileName)
{
  if (strFileName.Find('/') == -1 || /* doesn't have sharename */
      strFileName.Right(2) == "/." || /* not current folder */
      strFileName.Right(3) == "/..")  /* not parent folder */
      return false;
  return true;
}

CStdString CSmbFile::GetAuthenticatedPath(const CURL &url)
{
  CURL authURL(url);
  CPasswordManager::GetInstance().AuthenticateURL(authURL);
  return smb.URLEncode(authURL);
}
