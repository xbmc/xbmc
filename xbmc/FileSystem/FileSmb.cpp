// FileSmb.cpp: implementation of the CFileSMB class.
//
//////////////////////////////////////////////////////////////////////

#include "../stdafx.h"
#include "FileSmb.h"
#include "../GUIPassword.h"
#include "SMBDirectory.h"


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
  InitializeCriticalSection(&m_critSection);
  binitialized = false;
}

CSMB::~CSMB()
{
  DeleteCriticalSection(&m_critSection);
}

void CSMB::Init()
{
  if (!binitialized)
  {
    // set ip and subnet
    char szIPAddress[20];
    char szNetMask[20];
    strcpy(szIPAddress, g_guiSettings.GetString("Network.IPAddress").c_str());
    strcpy(szNetMask, g_guiSettings.GetString("Network.IPAddress").c_str());
    set_xbox_interface(szIPAddress, szNetMask);
    // set log function
    set_log_callback(xb_smbc_log);

    // set workgroup for samba, after smbc_init it can be freed();
    xb_setSambaWorkgroup(g_stSettings.m_strSambaWorkgroup);

    // initialize samba and do some hacking into the settings
    if (!smbc_init(xb_smbc_auth, g_stSettings.m_iSambaDebugLevel))
    {
      // if a wins-server is set, we have to change name resolve order to
      if (strlen(g_stSettings.m_strSambaWinsServer) > 1)
      {
        lp_do_parameter( -1, "wins server", g_stSettings.m_strSambaWinsServer);
        lp_do_parameter( -1, "name resolve order", "bcast wins");
      }
      else lp_do_parameter( -1, "name resolve order", "bcast");

      binitialized = true;
    }
  }
}

void CSMB::Purge()
{
  Lock();
  smbc_purge();
  Unlock();
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
  CStdString strShare = url.GetFileName().substr(0, url.GetFileName().Find('/'));

  if (m_strLastShare.length() > 0 && (m_strLastShare != strShare || m_strLastHost != url.GetHostName()))
    smbc_purge();

  m_strLastShare = strShare;
  m_strLastHost = url.GetHostName();
}

void CSMB::Lock()
{
  ::EnterCriticalSection(&m_critSection);
}

void CSMB::Unlock()
{
  ::LeaveCriticalSection(&m_critSection);
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
  smb.Lock();
  __int64 pos = smbc_lseek(m_fd, 0, SEEK_CUR);
  smb.Unlock();
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

  CStdString strFileName;
  url.GetURL(strFileName);

  smb.Lock();

  // opening a file to another computer share will create a new session
  // when opening smb://server xbms will try to find folder.jpg in all shares
  // listed, which will create lot's of open sessions.

  m_fd = OpenFile(strFileName);

  if (m_fd == -1)
  {
    smb.PurgeEx(url);
    smb.Unlock();
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
    smb.Unlock();
    return false;
  }

  m_fileSize = ret;
  ret = smbc_lseek(m_fd, 0, SEEK_SET);
  if ( ret < 0 )
  {
    smbc_close(m_fd);
    smb.PurgeEx(url);
    m_fd = -1;
    smb.Unlock();
    return false;
  }
  // We've successfully opened the file!
  smb.Unlock();
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
  fd = smbc_open(strPath.c_str(), O_RDONLY, 0);

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
      fd = smbDir.Open(strPath);

      // directory open worked, try opening the file again
      if (fd >= 0)
      {
        // close current directory filehandle
        // dont need to purge since its the same server and share
        smb.Lock();
        smbc_closedir(fd);
        smb.Unlock();

        // set up new filehandle (as CFileSMB::Open does)
        smb.Init();
        smb.Lock();

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

  CStdString strFileName;
  url.GetURL(strFileName);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  struct __stat64 info;

  smb.Lock();
  int iResult = smbc_stat(strFileName, &info);
  smb.PurgeEx(url);
  smb.Unlock();

  if (iResult < 0) return false;
  return true;
}

int CFileSMB::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFileName;
  url.GetURL(strFileName);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  smb.Lock();
  int iResult = smbc_stat(strFileName, buffer);
  smb.PurgeEx(url);
  smb.Unlock();

  return iResult;
}

unsigned int CFileSMB::Read(void *lpBuf, __int64 uiBufSize)
{
  if (m_fd == -1) return 0;
  smb.Lock();
  int bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
  smb.Unlock();
  if ( bytesRead <= 0 )
  {
    char szTmp[128];
    sprintf(szTmp, "SMB returned %i errno:%i\n", bytesRead, errno);
    OutputDebugString(szTmp);
    return 0;
  }
  return (unsigned int)bytesRead;
}

bool CFileSMB::ReadString(char *szLine, int iLineLength)
{
  if (m_fd == -1) return false;
  __int64 iFilePos = GetPosition();

  smb.Lock();
  int iBytesRead = smbc_read(m_fd, (unsigned char*)szLine, iLineLength);
  smb.Unlock();
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

  smb.Lock();
  INT64 pos = smbc_lseek(m_fd, iFilePosition, iWhence);
  smb.Unlock();

  if ( pos < 0 ) return -1;

  return (__int64)pos;
}

void CFileSMB::Close()
{
  if (m_fd != -1)
  {
    smb.Lock();
    smbc_close(m_fd);
    // file is not needed anymore, just purge all open connections
    smbc_purge();
    smb.Unlock();
  }
  m_fd = -1;
}

int CFileSMB::Write(const void* lpBuf, __int64 uiBufSize)
{
  if (m_fd == -1) return -1;
  DWORD dwNumberOfBytesWritten = 0;

  // lpBuf can be safely casted to void* since xmbc_write will only read from it.
  smb.Lock();
  dwNumberOfBytesWritten = smbc_write(m_fd, (void*)lpBuf, (DWORD)uiBufSize);
  smb.Unlock();

  return (int)dwNumberOfBytesWritten;
}

bool CFileSMB::Delete(const char* strFileName)
{
  smb.Init();
  smb.Lock();
  int result = smbc_unlink(strFileName);
  smb.Unlock();
  return (result == 0);
}

bool CFileSMB::Rename(const char* strFileName, const char* strNewFileName)
{
  smb.Init();
  smb.Lock();
  int result = smbc_rename(strFileName, strNewFileName);
  smb.Unlock();
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

  CStdString strFileName;
  url.GetURL(strFileName);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  smb.Lock();

  if (!bOverWrite)
    CLog::Log(LOGWARNING, "FileSmb::OpenForWrite() called with no overwriting, yet we are overwriting anyway!");
  m_fd = smbc_creat(strFileName.c_str(), 0);

  if (m_fd == -1)
  {
    smb.PurgeEx(url);
    smb.Unlock();
    // write error to logfile
    int nt_error = map_nt_error_from_unix(errno);
    CLog::Log(LOGERROR, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'",
              strFileName.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
    return false;
  }

  // We've successfully opened the file!
  smb.Unlock();
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
