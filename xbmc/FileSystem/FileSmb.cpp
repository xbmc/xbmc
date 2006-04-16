// FileSmb.cpp: implementation of the CFileSMB class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "FileSmb.h"
#include "../GUIPassword.h"
#include "SMBDirectory.h"
#include "../util.h"

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
  if (m_context)
  {
    smbc_free_context(m_context, 1);
    m_context = NULL;
  }
}

void CSMB::Init()
{
  if (!m_context)
  {
    CStdString strIPAddress = g_guiSettings.GetString("Network.IPAddress");
    CStdString strSubnet = g_guiSettings.GetString("Network.Subnet");

    set_xbox_interface((char*)strIPAddress.c_str(), (char*)strSubnet.c_str());
    // set log function
    set_log_callback(xb_smbc_log);

    // set workgroup for samba, after smbc_init it can be freed();
    xb_setSambaWorkgroup((char*)g_stSettings.m_strSambaWorkgroup.c_str());

    // setup our context
    m_context = smbc_new_context();
    m_context->debug = g_stSettings.m_iSambaDebugLevel;
    m_context->callbacks.auth_fn = xb_smbc_auth;

    /* set connection timeout. since samba always tries two ports, divide this by two the correct value */
    m_context->timeout = g_stSettings.m_iSambaTimeout / 2 * 1000;    

    // initialize samba and do some hacking into the settings
    if (smbc_init_context(m_context))
    {
      /* setup old interface to use this context */
      smbc_set_context(m_context);

      // if a wins-server is set, we have to change name resolve order to
      if (g_stSettings.m_strSambaWinsServer.length() > 1)
      {
        lp_do_parameter( -1, "wins server", g_stSettings.m_strSambaWinsServer.c_str());
        lp_do_parameter( -1, "name resolve order", "bcast wins");
      }
      else lp_do_parameter( -1, "name resolve order", "bcast");
            
      if (g_stSettings.m_strSambaDosCodepage.length() > 1 && !g_stSettings.m_strSambaDosCodepage.Equals("DEFAULT"))
      {
        lp_do_parameter( -1, "dos charset", g_stSettings.m_strSambaDosCodepage.c_str());
      }      
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
  CSingleLock(*this);
  smbc_purge();
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

  if (m_strLastShare.length() > 0 && (m_strLastShare != strShare || m_strLastHost != url.GetHostName()))
    smbc_purge();

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

  if(url.GetUserName().length() > 0 || url.GetPassWord().length() > 0)
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
  int buffer_len = value.length()*3+1;
  char* buffer = (char*)malloc(buffer_len);

  smbc_urlencode(buffer, (char*)value.c_str(), buffer_len);

  CStdString encoded = buffer;
  free(buffer);
  return encoded;
}


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
  CStdString strFileName = smb.URLEncode(url);

  CSingleLock lock(smb);

  // opening a file to another computer share will create a new session
  // when opening smb://server xbms will try to find folder.jpg in all shares
  // listed, which will create lot's of open sessions.

  m_fd = OpenFile(strFileName);

  if (m_fd == -1)
  {
    smb.PurgeEx(url);    
    // write error to logfile
    int nt_error = map_nt_error_from_unix(errno);
    CLog::Log(LOGINFO, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'",
              strFileName.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
    return false;
  }
  UINT64 ret = smbc_lseek(m_fd, 0, SEEK_END);

  if ( ret < 0 )
  {
    smbc_close(m_fd);
    smb.PurgeEx(url);
    m_fd = -1;    
    return false;
  }

  m_fileSize = ret;
  ret = smbc_lseek(m_fd, 0, SEEK_SET);
  if ( ret < 0 )
  {
    smbc_close(m_fd);
    smb.PurgeEx(url);
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

int CFileSMB::OpenFile(CStdString& strAuth)
{
  int fd = -1;
  
  CStdString strPath = g_passwordManager.GetSMBAuthFilename(strAuth);

  { CSingleLock lock(smb);
    fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
  }

  // file open failed, try to open the directory to force authentication
  if (fd < 0)
  {
    // 012345
    // smb://
    int iPos = strAuth.ReverseFind('/');
    if (iPos > 4)
    {
      strPath = strAuth.Left(iPos + 1);

      CSMBDirectory smbDir;
      // TODO: Currently we always allow prompting on files.  This may need to
      // change in the future as background scanners are more prolific.
      smbDir.SetAllowPrompting(true);
      fd = smbDir.Open(strPath);

      // directory open worked, try opening the file again
      if (fd >= 0)
      {
        CSingleLock lock(smb);
        // close current directory filehandle
        // dont need to purge since its the same server and share
        smbc_closedir(fd);

        // set up new filehandle (as CFileSMB::Open does)
        strPath = g_passwordManager.GetSMBAuthFilename(strAuth);
        fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
      }
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

  struct __stat64 info;

  CSingleLock lock(smb);
  int iResult = smbc_stat(strFileName, &info);
  smb.PurgeEx(url);

  if (iResult < 0) return false;
  return true;
}

int CFileSMB::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  CSingleLock lock(smb);
  int iResult = smbc_stat(strFileName, buffer);
  smb.PurgeEx(url);

  return iResult;
}

unsigned int CFileSMB::Read(void *lpBuf, __int64 uiBufSize)
{
  if (m_fd == -1) return 0;
  CSingleLock lock(smb);

  int bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);

  if ( bytesRead < 0 )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - smbc_read returned error %i", errno);
    return 0;
  }

  return (unsigned int)bytesRead;
}

bool CFileSMB::ReadString(char *szLine, int iLineLength)
{
  if (m_fd == -1) return false;
  __int64 iFilePos = GetPosition();

  CSingleLock lock(smb);

  int iBytesRead = smbc_read(m_fd, (unsigned char*)szLine, iLineLength - 1);

  if (iBytesRead <= 0)
  {
    return false;
  }

  szLine[iBytesRead] = 0;

  for (int i = 0; i < iBytesRead; i++)
  {
    if ('\n' == szLine[i])
    {
      if ('\r' == szLine[i + 1])
      {
        szLine[i + 2] = 0;
        Seek(iFilePos + i + 2, SEEK_SET);
      }
      else
      {
        // end of line
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 1, SEEK_SET);
      }
      break;
    }
    else if ('\r' == szLine[i])
    {
      if ('\n' == szLine[i + 1])
      {
        szLine[i + 2] = 0;
        Seek(iFilePos + i + 2, SEEK_SET);
      }
      else
      {
        // end of line
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 1, SEEK_SET);
      }
      break;
    }
  }
  if (iBytesRead > 0)
  {
    return true;
  }
  return false;

}

__int64 CFileSMB::Seek(__int64 iFilePosition, int iWhence)
{
  if (m_fd == -1) return -1;
  CSingleLock lock(smb);

  INT64 pos = smbc_lseek(m_fd, iFilePosition, iWhence);

  if ( pos < 0 ) return -1;

  return (__int64)pos;
}

void CFileSMB::Close()
{
  if (m_fd != -1)
  {
    CSingleLock lock(smb);
    smbc_close(m_fd);
    
    smb.PurgeEx(m_url);
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

bool CFileSMB::Delete(const char* strFileName)
{
  CURL url(strFileName);
  CStdString strFile = g_passwordManager.GetSMBAuthFilename(smb.URLEncode(url));

  CSingleLock lock(smb);
  smb.Init();
  int result = smbc_unlink(strFile.c_str());
  return (result == 0);
}

bool CFileSMB::Rename(const char* strFileName, const char* strNewFileName)
{
  CURL strold(strFileName);
  CURL strnew(strNewFileName);

  CStdString strFile = g_passwordManager.GetSMBAuthFilename(smb.URLEncode(strFileName));
  CStdString strFileNew = g_passwordManager.GetSMBAuthFilename(smb.URLEncode(strNewFileName));

  CSingleLock lock(smb);
  smb.Init();
  int result = smbc_rename(strFile.c_str(), strFileNew.c_str());
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
    smb.PurgeEx(url);
    // write error to logfile
    int nt_error = map_nt_error_from_unix(errno);
    CLog::Log(LOGERROR, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'",
              strFileName.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
    return false;
  }

  // We've successfully opened the file!
  return true;
}

bool CFileSMB::IsValidFile(const CStdString& strFileName)
{
  if (strFileName.Find('/') < 0) return false;
  if (strFileName.at(0) == '.') return false;

  int i = 0;

  i = strFileName.Find("/.",0);
  while (i>=0)
  {
    i += 2;
    if ((uint)i>=strFileName.length()) // illegal if ends in "/."
      return false;

    if (strFileName.at(i) == '.')
      return false; // illegal if "/.."
    if (strFileName.at(i) == '/') 
      return false; // illegal if "/./"

    i = strFileName.Find("/.",i);
  }

  return true;
}
