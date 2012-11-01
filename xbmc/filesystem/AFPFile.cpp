/*
 *      Copyright (C) 2011-2012 Team XBMC
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

// FileAFP.cpp: implementation of the CAFPFile class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _LINUX
#include "system.h"

#if defined(HAS_FILESYSTEM_AFP)
#include "AFPFile.h"
#include "PasswordManager.h"
#include "AFPDirectory.h"
#include "Util.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

using namespace XFILE;

#define AFP_MAX_READ_SIZE 131072

CStdString URLEncode(const CStdString value)
{
  CStdString encoded(value);
  CURL::Encode(encoded);
  return encoded;
}

void AfpConnectionLog(void *priv, enum loglevels loglevel, int logtype, const char *message)
{
  if (!message) return;
  CStdString msg = "LIBAFPCLIENT: " + CStdString(message);

  switch(logtype)
  {
    case LOG_WARNING:
      CLog::Log(LOGWARNING, "%s", msg.c_str());
      break;
    case LOG_ERR:
      CLog::Log(LOGERROR, "%s", msg.c_str());
      break;
    default:
      CLog::Log(LOGDEBUG, "%s", msg.c_str());
      break;
  }
}

CAfpConnection::CAfpConnection()
 : m_OpenConnections(0)
 , m_IdleTimeout(0)
 , m_pAfpServer(NULL)
 , m_pAfpVol(NULL)
 , m_pAfpUrl((struct afp_url*)malloc(sizeof(struct afp_url)))
 , m_pAfpClient((struct libafpclient*)malloc(sizeof(struct libafpclient)))
 , m_pLibAfp(new DllLibAfp())
 , m_bDllInited(false)
{
}

CAfpConnection::~CAfpConnection()
{
  Disconnect();
  free(m_pAfpClient);
  free(m_pAfpUrl);
  if (m_pLibAfp->IsLoaded())
    m_pLibAfp->Unload();
  delete m_pLibAfp;
}

bool CAfpConnection::initLib()
{
  if (!m_bDllInited)
  {
    if (m_pLibAfp->Load())
    {
      m_pAfpClient->unmount_volume = NULL;
      m_pAfpClient->log_for_client = AfpConnectionLog;
      m_pAfpClient->forced_ending_hook = NULL;
      m_pAfpClient->scan_extra_fds = NULL;
      m_pAfpClient->loop_started = NULL;

      m_pLibAfp->libafpclient_register(m_pAfpClient);
      m_pLibAfp->init_uams();
      m_pLibAfp->afp_main_quick_startup(NULL);
      CLog::Log(LOGDEBUG, "AFP: Supported UAMs: %s", m_pLibAfp->get_uam_names_list());
      m_bDllInited = true;
    }
    else
    {
      CLog::Log(LOGERROR, "AFP: Error loading afpclient lib.");
    }
  }

  return m_bDllInited;
}

//only unmount here - afpclient lib is not
//stoppable (no afp_main_quick_shutdown as counter part
//for afp_main_quick_startup)
void CAfpConnection::Deinit()
{
  if(m_pAfpVol && m_pLibAfp->IsLoaded())
  {
    disconnectVolume();
    Disconnect();
    m_pAfpUrl->servername[0] = '\0';
  }        
}

void CAfpConnection::Disconnect()
{
  CSingleLock lock(*this);
  m_pAfpServer = NULL;
}

void CAfpConnection::disconnectVolume()
{
  if (m_pAfpVol)
  {
    // afp_unmount_volume(m_pAfpVol);
    m_pLibAfp->afp_unmount_all_volumes(m_pAfpServer);
    m_pAfpVol = NULL;
  }
}

// taken from cmdline tool
bool CAfpConnection::connectVolume(const char *volumename, struct afp_volume *&pVolume)
{
  bool ret = false;
  if (strlen(volumename) != 0)
  {
    // Ah, we're not connected to a volume
    unsigned int len = 0;
    char mesg[1024];

    if ((pVolume = m_pLibAfp->find_volume_by_name(m_pAfpServer, volumename)) == NULL)
    {
      CLog::Log(LOGDEBUG, "AFP: Could not find a volume called %s\n", volumename);
    }
    else
    {
      pVolume->mapping = AFP_MAPPING_LOGINIDS;
      pVolume->extra_flags |= VOLUME_EXTRA_FLAGS_NO_LOCKING;

      if (m_pLibAfp->afp_connect_volume(pVolume, m_pAfpServer, mesg, &len, 1024 ))
      {
        CLog::Log(LOGDEBUG, "AFP: Could not access volume %s (error: %s)\n", pVolume->volume_name, mesg);
        pVolume = NULL;
      }
      else
      {
        CLog::Log(LOGDEBUG, "AFP: Connected to volume %s\n", pVolume->volume_name_printable);
        ret = true;
      }
    }
  }

  return ret;
}

CStdString CAfpConnection::getAuthenticatedPath(const CURL &url)
{
  CURL authURL(url);
  CStdString ret;
  CPasswordManager::GetInstance().AuthenticateURL(authURL);
  ret = authURL.Get();
  return ret;
}

CAfpConnection::afpConnnectError CAfpConnection::Connect(const CURL& url)
{
  CSingleLock lock(*this);
  struct afp_connection_request *conn_req = NULL;
  struct afp_url tmpurl;
  CURL nonConstUrl(getAuthenticatedPath(url)); // we need a editable copy of the url
  bool serverChanged=false;

  if (!initLib())
    return AfpFailed;

  m_pLibAfp->afp_default_url(&tmpurl);

  // if hostname has changed - assume server changed
  if (!nonConstUrl.GetHostName().Equals(m_pAfpUrl->servername, false)|| (m_pAfpServer && m_pAfpServer->connect_state == 0))
  {
    serverChanged = true;
    Disconnect();
  }

  // if volume changed - also assume server changed (afpclient can't reuse old servobject it seems)
  if (!nonConstUrl.GetShareName().Equals(m_pAfpUrl->volumename, false))
  {
   // no reusing of old server object possible with libafpclient it seems...
    serverChanged = true;
    Disconnect();
  }

  // first, try to parse the URL
  if (m_pLibAfp->afp_parse_url(&tmpurl, nonConstUrl.Get().c_str(), 0) != 0)
  {
    // Okay, this isn't a real URL
    CLog::Log(LOGDEBUG, "AFP: Could not parse url: %s!\n", nonConstUrl.Get().c_str());
    return AfpFailed;
  }
  else // parsed sucessfull
  {
    // this is our current url object whe are connected to (at least we try)
    *m_pAfpUrl = tmpurl;
  }

  // if no username and password is set - use no user authent uam
  if (strlen(m_pAfpUrl->password) == 0 && strlen(m_pAfpUrl->username) == 0)
  {
    // try anonymous
    strncpy(m_pAfpUrl->uamname, "No User Authent", sizeof(m_pAfpUrl->uamname));
    CLog::Log(LOGDEBUG, "AFP: Using anonymous authentication.");
  }
  else if ((nonConstUrl.GetPassWord().IsEmpty() || nonConstUrl.GetUserName().IsEmpty()) && serverChanged)
  {
    // this is our current url object whe are connected to (at least we try)
    return AfpAuth;
  }

  // we got a password in the url
  if (!nonConstUrl.GetPassWord().IsEmpty())
  {
    // copy password because afp_parse_url just puts garbage into the password field :(
    strncpy(m_pAfpUrl->password, nonConstUrl.GetPassWord().c_str(), 127);
  }

  // whe are not connected or we want to connect to another server
  if (!m_pAfpServer || serverChanged)
  {
    // code from cmdline tool
    conn_req = (struct afp_connection_request*)malloc(sizeof(struct afp_connection_request));
    memset(conn_req, 0, sizeof(struct afp_connection_request));

    conn_req->url = *m_pAfpUrl;
    conn_req->url.requested_version = 31;

    if (strlen(m_pAfpUrl->uamname)>0)
    {
      if ((conn_req->uam_mask = m_pLibAfp->find_uam_by_name(m_pAfpUrl->uamname)) == 0)
      {
        CLog::Log(LOGDEBUG, "AFP:I don't know about UAM %s\n", m_pAfpUrl->uamname);
        m_pAfpUrl->volumename[0] = '\0';
        m_pAfpUrl->servername[0] = '\0';
        free(conn_req);
        return AfpFailed;
      }
    }
    else
    {
      conn_req->uam_mask = m_pLibAfp->default_uams_mask();
    }

    // try to connect
#ifdef USE_CVS_AFPFS
    if ((m_pAfpServer = m_pLibAfp->afp_wrap_server_full_connect(NULL, conn_req, NULL)) == NULL)
#else
    if ((m_pAfpServer = m_pLibAfp->afp_wrap_server_full_connect(NULL, conn_req)) == NULL)
#endif
    {
      m_pAfpUrl->volumename[0] = '\0';
      m_pAfpUrl->servername[0] = '\0';
      free(conn_req);
      CLog::Log(LOGERROR, "AFP: Error connecting to %s", url.Get().c_str());
      return AfpFailed;
    }
    // success!
    CLog::Log(LOGDEBUG, "AFP: Connected to server %s using UAM \"%s\"\n",
      m_pAfpServer->server_name, m_pLibAfp->uam_bitmap_to_string(m_pAfpServer->using_uam));
    // we don't need it after here ...
    free(conn_req);
  }

  // if server changed reconnect volume
  if (serverChanged)
  {
    connectVolume(m_pAfpUrl->volumename, m_pAfpVol); // connect new volume
  }
  return AfpOk;
}

int CAfpConnection::stat(const CURL &url, struct stat *statbuff)
{
  CSingleLock lock(*this);
  CStdString strPath = gAfpConnection.GetPath(url);
  struct afp_volume *pTmpVol = NULL;
  struct afp_url tmpurl;
  int iResult = -1;
  CURL nonConstUrl(getAuthenticatedPath(url)); // we need a editable copy of the url

  if (!initLib() || !m_pAfpServer)
    return -1;

  m_pLibAfp->afp_default_url(&tmpurl);

  // first, try to parse the URL
  if (m_pLibAfp->afp_parse_url(&tmpurl, nonConstUrl.Get().c_str(), 0) != 0)
  {
    // Okay, this isn't a real URL
    CLog::Log(LOGDEBUG, "AFP: Could not parse url: %s!\n", nonConstUrl.Get().c_str());
    return -1;
  }

  // if no username and password is set - use no user authent uam
  if (strlen(tmpurl.password) == 0 && strlen(tmpurl.username) == 0)
  {
    // try anonymous
    strncpy(tmpurl.uamname, "No User Authent", sizeof(tmpurl.uamname));
    CLog::Log(LOGDEBUG, "AFP: Using anonymous authentication.");
  }
  else if ((nonConstUrl.GetPassWord().IsEmpty() || nonConstUrl.GetUserName().IsEmpty()))
  {
    // this is our current url object whe are connected to (at least we try)
    return -1;
  }

  // we got a password in the url
  if (!nonConstUrl.GetPassWord().IsEmpty())
  {
    // copy password because afp_parse_url just puts garbage into the password field :(
    strncpy(tmpurl.password, nonConstUrl.GetPassWord().c_str(), 127);
  }

  // connect new volume
  if(connectVolume(tmpurl.volumename, pTmpVol) && pTmpVol)
  {
    iResult = m_pLibAfp->afp_wrap_getattr(pTmpVol, strPath.c_str(), statbuff);
    //unmount single volume crashs
    //we will get rid of the mounted volume
    //once the context is changed in connect function
    //ppppooooorrrr!!
    //m_pLibAfp->afp_unmount_volume(pTmpVol);
  }
  return iResult;
}


/* This is called from CApplication::ProcessSlow() and is used to tell if afp have been idle for too long */
void CAfpConnection::CheckIfIdle()
{
  /* We check if there are open connections. This is done without a lock to not halt the mainthread. It should be thread safe as
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if wich will lead to another check, wich is locked.  */
  if (m_OpenConnections == 0 && m_pAfpVol != NULL)
  { /* I've set the the maxiumum IDLE time to be 1 min and 30 sec. */
    CSingleLock lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */)
    {
      if (m_IdleTimeout > 0)
      {
        m_IdleTimeout--;
      }
      else
      {
        CLog::Log(LOGNOTICE, "AFP is idle. Closing the remaining connections.");
        gAfpConnection.Deinit();
      }
    }
  }
}

/* The following two function is used to keep track on how many Opened files/directories there are.
needed for unloading the dylib*/
void CAfpConnection::AddActiveConnection()
{
  CSingleLock lock(*this);
  m_OpenConnections++;
}

void CAfpConnection::AddIdleConnection()
{
  CSingleLock lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any wierd behaviours if a user
   leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;
}

CStdString CAfpConnection::GetPath(const CURL &url)
{
  struct afp_url tmpurl;
  CStdString ret = "";

  m_pLibAfp->afp_default_url(&tmpurl);

  // First, try to parse the URL
  if (m_pLibAfp->afp_parse_url(&tmpurl, url.Get().c_str(), 0) != 0 )
  {
    // Okay, this isn't a real URL
    CLog::Log(LOGDEBUG, "AFP: Could not parse url.\n");
  }
  else
  {
    ret = CStdString(tmpurl.path);
  }
  return ret;
}

CAfpConnection gAfpConnection;

CAFPFile::CAFPFile()
 : m_fileSize(0)
 , m_fileOffset(0)
 , m_pFp(NULL)
 , m_pAfpVol(NULL)
{
  gAfpConnection.AddActiveConnection();
}

CAFPFile::~CAFPFile()
{
  gAfpConnection.AddIdleConnection();
  Close();
}

int64_t CAFPFile::GetPosition()
{
  if (m_pFp == NULL) return 0;
  return m_fileOffset;
}

int64_t CAFPFile::GetLength()
{
  if (m_pFp == NULL) return 0;
  return m_fileSize;
}

bool CAFPFile::Open(const CURL& url)
{
  Close();
  // we can't open files like afp://file.f or afp://server/file.f
  // if a file matches the if below return false, it can't exist on a afp share.
  if (!IsValidFile(url.GetFileName()))
  {
    CLog::Log(LOGNOTICE, "FileAfp: Bad URL : '%s'", url.GetFileName().c_str());
    return false;
  }

  CSingleLock lock(gAfpConnection);
  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return false;
  m_pAfpVol = gAfpConnection.GetVolume();

  CStdString strPath = gAfpConnection.GetPath(url);

  if (gAfpConnection.GetImpl()->afp_wrap_open(m_pAfpVol, strPath.c_str(), O_RDONLY, &m_pFp))
  {
    if (gAfpConnection.GetImpl()->afp_wrap_open(m_pAfpVol, URLEncode(strPath.c_str()).c_str(), O_RDONLY, &m_pFp))
    {
      // write error to logfile
      CLog::Log(LOGINFO, "CAFPFile::Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strPath.c_str(), errno, strerror(errno));
      return false;
    }
  }
  
  CLog::Log(LOGDEBUG,"CAFPFile::Open - opened %s, fd=%d",url.GetFileName().c_str(), m_pFp ? m_pFp->fileid:-1);
  m_url = url;
  
#ifdef _LINUX
  struct __stat64 tmpBuffer;
#else
  struct stat tmpBuffer;
#endif  
  if(Stat(&tmpBuffer))
  {
    m_url.Reset();
    Close();
    return false;
  }

  m_fileSize = tmpBuffer.st_size;
  m_fileOffset = 0;
  // We've successfully opened the file!
  return true;
}


bool CAFPFile::Exists(const CURL& url)
{
  return Stat(url, NULL) == 0;
}

int CAFPFile::Stat(struct __stat64* buffer)
{
  if (m_pFp == NULL)
    return -1;
  return Stat(m_url, buffer);
}

// TODO - maybe check returncode!
int CAFPFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CSingleLock lock(gAfpConnection);
  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return -1;

  CStdString strPath = gAfpConnection.GetPath(url);

  struct stat tmpBuffer = {0};
  int iResult = gAfpConnection.GetImpl()->afp_wrap_getattr(gAfpConnection.GetVolume(), strPath.c_str(), &tmpBuffer);

  if (buffer)
  {
    memset(buffer, 0, sizeof(struct __stat64));
    buffer->st_dev   = tmpBuffer.st_dev;
    buffer->st_ino   = tmpBuffer.st_ino;
    buffer->st_mode  = tmpBuffer.st_mode;
    buffer->st_nlink = tmpBuffer.st_nlink;
    buffer->st_uid   = tmpBuffer.st_uid;
    buffer->st_gid   = tmpBuffer.st_gid;
    buffer->st_rdev  = tmpBuffer.st_rdev;
    buffer->st_size  = tmpBuffer.st_size;
    buffer->st_atime = tmpBuffer.st_atime;
    buffer->st_mtime = tmpBuffer.st_mtime;
    buffer->st_ctime = tmpBuffer.st_ctime;
  }

  return iResult;
}

unsigned int CAFPFile::Read(void *lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(gAfpConnection);
  if (m_pFp == NULL || !m_pAfpVol)
    return 0;

  if (uiBufSize > AFP_MAX_READ_SIZE)
    uiBufSize = AFP_MAX_READ_SIZE;

#ifdef USE_CVS_AFPFS
  char *name = m_pFp->basename;
#else
  char *name = m_pFp->name;
  if (strlen(name) == 0)
    name = m_pFp->basename;

#endif
  int eof = 0;
  int bytesRead = gAfpConnection.GetImpl()->afp_wrap_read(m_pAfpVol,
    name, (char *)lpBuf,(size_t)uiBufSize, m_fileOffset, m_pFp, &eof);
  if (bytesRead > 0)
    m_fileOffset += bytesRead;

  if (bytesRead < 0)
  {
    CLog::Log(LOGERROR, "%s - Error( %d, %d, %s )", __FUNCTION__, bytesRead, errno, strerror(errno));
    return 0;
  }

  return (unsigned int)bytesRead;
}

int64_t CAFPFile::Seek(int64_t iFilePosition, int iWhence)
{
  off_t newOffset = m_fileOffset;
  if (m_pFp == NULL) return -1;

  switch(iWhence)
  {
    case SEEK_SET:
      newOffset = iFilePosition;
      break;
    case SEEK_END:
      newOffset = m_fileSize+iFilePosition;
      break;
    case SEEK_CUR:
      newOffset += iFilePosition;
      break;
  }

  if ( newOffset < 0 || newOffset > m_fileSize)
  {
    CLog::Log(LOGERROR, "%s - Error( %"PRId64")", __FUNCTION__, newOffset);
    return -1;
  }

  m_fileOffset = newOffset;
  return (int64_t)m_fileOffset;
}

void CAFPFile::Close()
{
  CSingleLock lock(gAfpConnection);
  if (m_pFp != NULL && m_pAfpVol)
  {
    CLog::Log(LOGDEBUG, "CAFPFile::Close closing fd %d", m_pFp->fileid);
#ifdef USE_CVS_AFPFS
    char *name = m_pFp->basename;
#else
    char *name = m_pFp->name;
    if (strlen(name) == 0)
      name = m_pFp->basename;
#endif
    gAfpConnection.GetImpl()->afp_wrap_close(m_pAfpVol, name, m_pFp);
    delete m_pFp;
    m_pFp = NULL;
    m_pAfpVol = NULL;
  }
}

int CAFPFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(gAfpConnection);
  if (m_pFp == NULL || !m_pAfpVol)
   return -1;

  int numberOfBytesWritten = 0;
  uid_t uid;
  gid_t gid;

  // FIXME need a better way to get server's uid/gid
  uid = getuid();
  gid = getgid();
#ifdef USE_CVS_AFPFS
  char *name = m_pFp->basename;
#else
  char *name = m_pFp->name;
  if (strlen(name) == 0)
    name = m_pFp->basename;
#endif
  numberOfBytesWritten = gAfpConnection.GetImpl()->afp_wrap_write(m_pAfpVol,
    name, (const char *)lpBuf, (size_t)uiBufSize, m_fileOffset, m_pFp, uid, gid);

  return numberOfBytesWritten;
}

bool CAFPFile::Delete(const CURL& url)
{
  CSingleLock lock(gAfpConnection);
  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return false;

  CStdString strPath = gAfpConnection.GetPath(url);

  int result = gAfpConnection.GetImpl()->afp_wrap_unlink(gAfpConnection.GetVolume(), strPath.c_str());

  if (result != 0)
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CAFPFile::Rename(const CURL& url, const CURL& urlnew)
{
  CSingleLock lock(gAfpConnection);
  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return false;

  CStdString strFile = gAfpConnection.GetPath(url);
  CStdString strFileNew = gAfpConnection.GetPath(urlnew);

  int result = gAfpConnection.GetImpl()->afp_wrap_rename(gAfpConnection.GetVolume(), strFile.c_str(), strFileNew.c_str());

  if (result != 0)
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CAFPFile::OpenForWrite(const CURL& url, bool bOverWrite)
{

  int ret = 0;
  m_fileSize = 0;
  m_fileOffset = 0;

  Close();
  CSingleLock lock(gAfpConnection);
  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return false;

  // we can't open files like afp://file.f or afp://server/file.f
  // if a file matches the if below return false, it can't exist on a afp share.
  if (!IsValidFile(url.GetFileName()))
    return false;

  m_pAfpVol = gAfpConnection.GetVolume();

  CStdString strPath = gAfpConnection.GetPath(url);

  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "FileAFP::OpenForWrite() called with overwriting enabled! - %s", strPath.c_str());
    ret = gAfpConnection.GetImpl()->afp_wrap_creat(m_pAfpVol, strPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  }

  ret = gAfpConnection.GetImpl()->afp_wrap_open(m_pAfpVol, strPath.c_str(), O_RDWR, &m_pFp);

  if (ret || m_pFp == NULL)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "CAFPFile::Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strPath.c_str(), errno, strerror(errno));
    return false;
  }

  // We've successfully opened the file!
  return true;
}

bool CAFPFile::IsValidFile(const CStdString& strFileName)
{
  if (strFileName.Find('/') == -1   || // doesn't have sharename
      strFileName.Right(2)  == "/." || // not current folder
      strFileName.Right(3)  == "/..")  // not parent folder
  {
    return false;
  }

  return true;
}
#endif // HAS_FILESYSTEM_AFP
#endif // _LINUX
