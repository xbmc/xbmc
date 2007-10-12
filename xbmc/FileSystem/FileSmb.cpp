// FileSmb.cpp: implementation of the CFileSMB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileSmb.h"
#include "../GUIPassword.h"
#include "SMBDirectory.h"
#ifndef _LINUX
#include "../lib/libsmb/xbLibSmb.h"
#else
#include <libsmbclient.h>
#endif
#include "../Util.h"
#include "../utils/Network.h"
#include "../utils/Win32Exception.h"

using namespace XFILE;
using namespace DIRECTORY;

void xb_smbc_log(const char* msg)
{
  CLog::Log(LOGINFO, "%s%s", "smb: ", msg);
}

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen,
                  char *un, int unlen, char *pw, int pwlen)
{
  return ;
}

CSMB::CSMB()
{  
  m_context = NULL;
}

CSMB::~CSMB()
{
  Deinit();
}
void CSMB::Deinit()
{
  CSingleLock(*this);

  /* samba goes loco if deinited while it has some files opened */
  if (m_context)
  {
    try
    {
      smbc_set_context(NULL);
      smbc_free_context(m_context, 1);
    }
    catch(win32_exception e)
    {
      e.writelog(__FUNCTION__);
    }
    m_context = NULL;
  }
}

void CSMB::Init()
{
  CSingleLock(*this);
  if (!m_context)
  {
#ifdef _XBOX
    set_xbox_interface(g_network.m_networkinfo.ip, g_network.m_networkinfo.subnet);

    // set log function
    set_log_callback(xb_smbc_log);

    // set workgroup for samba, after smbc_init it can be freed();
    xb_setSambaWorkgroup((char*)g_guiSettings.GetString("smb.workgroup").c_str());
#endif

    // setup our context
    m_context = smbc_new_context();
    m_context->debug = g_advancedSettings.m_logLevel == LOG_LEVEL_DEBUG_SAMBA ? 10 : 0;
    m_context->callbacks.auth_fn = xb_smbc_auth;
#ifndef _LINUX
    m_context->options.one_share_per_server = true;
#else
    // the one_share_per_server behavior causes problems when using more than one smb shares 
    // concurently. for example - listening to music from one smb share and viewing slide show from the
    // other.
    // we dont have a problem under linux to support many concurrent connections so we turn this off.
    m_context->options.one_share_per_server = false;
#endif

    /* set connection timeout. since samba always tries two ports, divide this by two the correct value */
    m_context->timeout = g_advancedSettings.m_sambaclienttimeout * 1000;    

#ifdef _LINUX
    // Create ~/.smb/smb.conf
    char smb_conf[MAX_PATH];
    sprintf(smb_conf, "%s/.smb", getenv("HOME"));
    mkdir(smb_conf, 0755);
    sprintf(smb_conf, "%s/.smb/smb.conf", getenv("HOME"));
    FILE* f = fopen(smb_conf, "w");
    if (f != NULL)
    {
      fprintf(f, "[global]\n");
      // if a wins-server is set, we have to change name resolve order to
      if ( g_guiSettings.GetString("smb.winsserver").length() > 0 && !g_guiSettings.GetString("smb.winsserver").Equals("0.0.0.0") ) 
      {
        fprintf(f, "  wins server = %s\n", g_guiSettings.GetString("smb.winsserver").c_str());
        fprintf(f, "  name resolve order = bcast wins\n");
      }
      else
        fprintf(f, "name resolve order = bcast\n");

      if (g_advancedSettings.m_sambadoscodepage.length() > 0)
        fprintf(f, "dos charset = %s\n", g_advancedSettings.m_sambadoscodepage.c_str());
      else
        fprintf(f, "dos charset = CP850\n");

      fprintf(f, "workgroup = %s\n", g_guiSettings.GetString("smb.workgroup").c_str());
      fclose(f);
    }
#endif
    
    // initialize samba and do some hacking into the settings
    if (smbc_init_context(m_context))
    {
      /* setup old interface to use this context */
      smbc_set_context(m_context);

#ifndef _LINUX
      // if a wins-server is set, we have to change name resolve order to
      if ( g_guiSettings.GetString("smb.winsserver").length() > 0 && !g_guiSettings.GetString("smb.winsserver").Equals("0.0.0.0") )
      {
        lp_do_parameter( -1, "wins server", g_guiSettings.GetString("smb.winsserver").c_str());
        lp_do_parameter( -1, "name resolve order", "bcast wins");
      }
      else 
        lp_do_parameter( -1, "name resolve order", "bcast");
            
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
}

void CSMB::Purge()
{
#ifndef _LINUX
  CSingleLock(*this);
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
  CSingleLock(*this);
  CStdString strShare = url.GetFileName().substr(0, url.GetFileName().Find('/'));

#ifndef _LINUX
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
  else if( !url.GetHostName().IsEmpty() && !g_guiSettings.GetString("smb.username").IsEmpty() )
  {
    /* okey this is abit uggly to do this here, as we don't really only url encode */
    /* but it's the simplest place to do so */
    flat += URLEncode(g_guiSettings.GetString("smb.username"));
    flat += ":";
    flat += URLEncode(g_guiSettings.GetString("smb.password"));
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
  int buffer_len = value.length()*3+1;
  char* buffer = (char*)malloc(buffer_len);

  smbc_urlencode(buffer, (char*)value.c_str(), buffer_len);

  CStdString encoded = buffer;
  free(buffer);
  return encoded;
}

#ifndef _LINUX
DWORD CSMB::ConvertUnixToNT(int error)
{
  DWORD nt_error;
  if (error == ENODEV || error == ENETUNREACH || error == WSAETIMEDOUT) nt_error = NT_STATUS_INVALID_COMPUTER_NAME;
  else if(error == WSAECONNREFUSED || error == WSAECONNABORTED) nt_error = NT_STATUS_CONNECTION_REFUSED;
  else nt_error = map_nt_error_from_unix(error);

  return nt_error;
}
#endif

CSMB smb;

CFileSMB::CFileSMB()
{
  smb.Init();
  m_fd = -1;
}

CFileSMB::~CFileSMB()
{
  Close();
}

__int64 CFileSMB::GetPosition()
{
  if (m_fd == -1) return 0;
  CSingleLock lock(smb);
  __int64 pos = smbc_lseek(m_fd, 0, SEEK_CUR);
  if ( pos < 0 )
    return 0;
  return pos;
}

__int64 CFileSMB::GetLength()
{
  if (m_fd == -1) return 0;
  return m_fileSize;
}

bool CFileSMB::Open(const CURL& url, bool bBinary)
{
  m_bBinary = bBinary;

  Close();

  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName()))
  {
      CLog::Log(LOGNOTICE,"FileSmb->Open: Bad URL : '%s'",url.GetFileName().c_str());
      return false;
  }
  m_url = url;

  CSingleLock lock(smb);

  // opening a file to another computer share will create a new session
  // when opening smb://server xbms will try to find folder.jpg in all shares
  // listed, which will create lot's of open sessions.

  CStdString strFileName;
  m_fd = OpenFile(url, strFileName);

  CLog::Log(LOGDEBUG,"CFileSMB::Open - opened %s, fd=%d",url.GetFileName().c_str(), m_fd);
  if (m_fd == -1)
  {
    // write error to logfile
#ifndef _LINUX
    int nt_error = smb.ConvertUnixToNT(errno);
    CLog::Log(LOGINFO, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'", strFileName.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
#else
    CLog::Log(LOGINFO, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strFileName.c_str(), errno, strerror(errno));
#endif
    return false;
  }
  UINT64 ret = smbc_lseek(m_fd, 0, SEEK_END);

  if ( ret < 0 )
  {
    smbc_close(m_fd);
    m_fd = -1;    
    return false;
  }

  m_fileSize = ret;
  ret = smbc_lseek(m_fd, 0, SEEK_SET);
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
int CFileSMB::OpenFile(CStdString& strAuth)
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

int CFileSMB::OpenFile(const CURL &url, CStdString& strAuth)
{
  int fd = -1;

  /* original auth name */
  strAuth = smb.URLEncode(url);

  CStdString strPath = g_passwordManager.GetSMBAuthFilename(strAuth);

  { CSingleLock lock(smb);
    fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
  }

  // file open failed, try to open the directory to force authentication
#ifndef _LINUX
  if (fd < 0 && smb.ConvertUnixToNT(errno) == NT_STATUS_ACCESS_DENIED)
#else
  if (fd < 0 && errno == EACCES)
#endif
  {
    CURL urlshare(url);
    
    /* just replace the filename with the sharename */
    urlshare.SetFileName(url.GetShareName());

    CSMBDirectory smbDir;
    // TODO: Currently we always allow prompting on files.  This may need to
    // change in the future as background scanners are more prolific.
    smbDir.SetAllowPrompting(true);
    fd = smbDir.Open(urlshare);

    // directory open worked, try opening the file again
    if (fd >= 0)
    {
      CSingleLock lock(smb);
      // close current directory filehandle
      // dont need to purge since its the same server and share
      smbc_closedir(fd);

      // set up new filehandle (as CFileSMB::Open does)
      strPath = g_passwordManager.GetSMBAuthFilename(strPath);
      fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
    }
  }

  if (fd >= 0)
    strAuth = strPath;

  return fd;
}

bool CFileSMB::Exists(const CURL& url)
{
  // if a file matches the if below return false, it can't exist on a samba share.
  if (url.GetFileName().Find('/') < 0 ||
      url.GetFileName().at(0) == '.' ||
      url.GetFileName().Find("/.") >= 0) return false;

  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

#ifndef _LINUX
  struct __stat64 info;
#else
  struct stat info;
#endif

  CSingleLock lock(smb);
  int iResult = smbc_stat(strFileName, &info);

  if (iResult < 0) return false;
  return true;
}

int CFileSMB::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  CSingleLock lock(smb);
  struct stat tmpBuffer;
  int iResult = smbc_stat(strFileName, &tmpBuffer);

  buffer->st_dev = tmpBuffer.st_dev;
  buffer->st_ino = tmpBuffer.st_ino;  
  buffer->st_mode = tmpBuffer.st_mode;
  buffer->st_nlink = tmpBuffer.st_nlink;
  buffer->st_uid = tmpBuffer.st_uid;
  buffer->st_gid = tmpBuffer.st_gid;
  buffer->st_rdev = tmpBuffer.st_rdev;
  buffer->st_size = tmpBuffer.st_size;
  buffer->_st_atime = tmpBuffer.st_atime;
  buffer->_st_mtime = tmpBuffer.st_mtime;
  buffer->_st_ctime = tmpBuffer.st_ctime;

  return iResult;
}

unsigned int CFileSMB::Read(void *lpBuf, __int64 uiBufSize)
{
  if (m_fd == -1) return 0;
  CSingleLock lock(smb);

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

  if ( bytesRead < 0 )
  {
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif
    return 0;
  }

  return (unsigned int)bytesRead;
}

__int64 CFileSMB::Seek(__int64 iFilePosition, int iWhence)
{
  if (m_fd == -1) return -1;
  if(iWhence == SEEK_POSSIBLE)
    return 1;

  CSingleLock lock(smb);

  INT64 pos = smbc_lseek(m_fd, iFilePosition, iWhence);

  if ( pos < 0 )
  {
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif
    return -1;
  }

  return (__int64)pos;
}

void CFileSMB::Close()
{
  if (m_fd != -1)
  {
    CLog::Log(LOGDEBUG,"CFileSMB::Close closing fd %d", m_fd);
    CSingleLock lock(smb);
    smbc_close(m_fd);
  }
  m_fd = -1;
}

int CFileSMB::Write(const void* lpBuf, __int64 uiBufSize)
{
  if (m_fd == -1) return -1;
  DWORD dwNumberOfBytesWritten = 0;

  // lpBuf can be safely casted to void* since xmbc_write will only read from it.
  CSingleLock lock(smb);
  dwNumberOfBytesWritten = smbc_write(m_fd, (void*)lpBuf, (DWORD)uiBufSize);

  return (int)dwNumberOfBytesWritten;
}

bool CFileSMB::Delete(const CURL& url)
{
  CStdString strFile = g_passwordManager.GetSMBAuthFilename(smb.URLEncode(url));

  CSingleLock lock(smb);
  smb.Init();
  int result = smbc_unlink(strFile.c_str());

  if(result != 0)
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0);
}

bool CFileSMB::Rename(const CURL& url, const CURL& urlnew)
{
  CStdString strFile = g_passwordManager.GetSMBAuthFilename(smb.URLEncode(url));
  CStdString strFileNew = g_passwordManager.GetSMBAuthFilename(smb.URLEncode(urlnew));

  CSingleLock lock(smb);
  smb.Init();
  int result = smbc_rename(strFile.c_str(), strFileNew.c_str());

  if(result != 0)
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0);
}

bool CFileSMB::OpenForWrite(const CURL& url, bool bBinary, bool bOverWrite)
{
  m_bBinary = bBinary;
  m_fileSize = 0;

  Close();

  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName())) return false;

  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

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
#ifndef _LINUX
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

bool CFileSMB::IsValidFile(const CStdString& strFileName)
{
  if (strFileName.Find('/') == CStdString::npos || /* doesn't have sharename */
      strFileName.Right(2) == "/." || /* not current folder */
      strFileName.Right(3) == "/..")  /* not parent folder */
      return false;
  return true;
}
