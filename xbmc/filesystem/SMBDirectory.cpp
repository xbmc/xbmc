
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

/*
* know bugs:
* - when opening a server for the first time with ip adres and the second time
*   with server name, access to the server is denied.
* - when browsing entire network, user can't go back one step
*   share = smb://, user selects a workgroup, user selects a server.
*   doing ".." will go back to smb:// (entire network) and not to workgroup list.
*
* debugging is set to a max of 10 for release builds (see local.h)
*/

#include "system.h"

#include "SMBDirectory.h"
#include "Util.h"
#include "guilib/LocalizeStrings.h"
#include "Application.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"
#include "PasswordManager.h"

#include <libsmbclient.h>

#if defined(TARGET_DARWIN)
#define XBMC_SMB_MOUNT_PATH "Library/Application Support/XBMC/Mounts/"
#else
#define XBMC_SMB_MOUNT_PATH "/media/xbmc/smb/"
#endif

struct CachedDirEntry
{
  unsigned int type;
  CStdString name;
};

using namespace XFILE;
using namespace std;

CSMBDirectory::CSMBDirectory(void)
{
#ifdef _LINUX
  smb.AddActiveConnection();
#endif
}

CSMBDirectory::~CSMBDirectory(void)
{
#ifdef _LINUX
  smb.AddIdleConnection();
#endif
}

bool CSMBDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]

  /* samba isn't thread safe with old interface, always lock */
  CSingleLock lock(smb);

  smb.Init();

  /* we need an url to do proper escaping */
  CURL url(strPath);

  //Separate roots for the authentication and the containing items to allow browsing to work correctly
  CStdString strRoot = strPath;
  CStdString strAuth;

  lock.Leave(); // OpenDir is locked
  int fd = OpenDir(url, strAuth);
  if (fd < 0)
    return false;

  URIUtils::AddSlashAtEnd(strRoot);
  URIUtils::AddSlashAtEnd(strAuth);

  CStdString strFile;

  // need to keep the samba lock for as short as possible.
  // so we first cache all directory entries and then go over them again asking for stat
  // "stat" is locked each time. that way the lock is freed between stat requests
  vector<CachedDirEntry> vecEntries;
  struct smbc_dirent* dirEnt;

  lock.Enter();
  while ((dirEnt = smbc_readdir(fd)))
  {
    CachedDirEntry aDir;
    aDir.type = dirEnt->smbc_type;
    aDir.name = dirEnt->name;
    vecEntries.push_back(aDir);
  }
  smbc_closedir(fd);
  lock.Leave();

  for (size_t i=0; i<vecEntries.size(); i++)
  {
    CachedDirEntry aDir = vecEntries[i];

    // We use UTF-8 internally, as does SMB
    strFile = aDir.name;

    if (!strFile.Equals(".") && !strFile.Equals("..")
      && !strFile.Equals("lost+found")
      && aDir.type != SMBC_PRINTER_SHARE && aDir.type != SMBC_IPC_SHARE)
    {
     int64_t iSize = 0;
      bool bIsDir = true;
      int64_t lTimeDate = 0;
      bool hidden = false;

      if(strFile.Right(1).Equals("$") && aDir.type == SMBC_FILE_SHARE )
        continue;

      // only stat files that can give proper responses
      if ( aDir.type == SMBC_FILE ||
           aDir.type == SMBC_DIR )
      {
        // set this here to if the stat should fail
        bIsDir = (aDir.type == SMBC_DIR);

#ifdef TARGET_WINDOWS
        struct __stat64 info = {0};
#else
        struct stat info = {0};
#endif
        if ((m_flags & DIR_FLAG_NO_FILE_INFO)==0 && g_advancedSettings.m_sambastatfiles)
        {
          // make sure we use the authenticated path wich contains any default username
          CStdString strFullName = strAuth + smb.URLEncode(strFile);

          lock.Enter();

          if( smbc_stat(strFullName.c_str(), &info) == 0 )
          {

#ifdef TARGET_WINDOWS
            if ((info.st_mode & S_IXOTH))
              hidden = true;
#else
            char value[20];
            // We poll for extended attributes which symbolizes bits but split up into a string. Where 0x02 is hidden and 0x12 is hidden directory.
            // According to the libsmbclient.h it's supposed to return 0 if ok, or the length of the string. It seems always to return the length wich is 4
            if (smbc_getxattr(strFullName, "system.dos_attr.mode", value, sizeof(value)) > 0)
            {
              long longvalue = strtol(value, NULL, 16);
              if (longvalue & SMBC_DOS_MODE_HIDDEN)
                hidden = true;
            }
            else
              CLog::Log(LOGERROR, "Getting extended attributes for the share: '%s'\nunix_err:'%x' error: '%s'", strFullName.c_str(), errno, strerror(errno));
#endif

            bIsDir = (info.st_mode & S_IFDIR) ? true : false;
            lTimeDate = info.st_mtime;
            if(lTimeDate == 0) // if modification date is missing, use create date
              lTimeDate = info.st_ctime;
            iSize = info.st_size;
          }
          else
            CLog::Log(LOGERROR, "%s - Failed to stat file %s", __FUNCTION__, strFullName.c_str());

          lock.Leave();
        }
      }

      FILETIME fileTime, localTime;
      LONGLONG ll = Int32x32To64(lTimeDate & 0xffffffff, 10000000) + 116444736000000000ll;
      fileTime.dwLowDateTime = (DWORD) (ll & 0xffffffff);
      fileTime.dwHighDateTime = (DWORD)(ll >> 32);
      FileTimeToLocalFileTime(&fileTime, &localTime);

      if (bIsDir)
      {
        CFileItemPtr pItem(new CFileItem(strFile));
        CStdString path(strRoot);

        // needed for network / workgroup browsing
        // skip if root if we are given a server
        if (aDir.type == SMBC_SERVER)
        {
          /* create url with same options, user, pass.. but no filename or host*/
          CURL rooturl(strRoot);
          rooturl.SetFileName("");
          rooturl.SetHostName("");
          path = smb.URLEncode(rooturl);
        }
        path = URIUtils::AddFileToFolder(path,aDir.name);
        URIUtils::AddSlashAtEnd(path);
        pItem->SetPath(path);
        pItem->m_bIsFolder = true;
        pItem->m_dateTime=localTime;
        if (hidden)
          pItem->SetProperty("file:hidden", true);
        items.Add(pItem);
      }
      else
      {
        CFileItemPtr pItem(new CFileItem(strFile));
        pItem->SetPath(strRoot + aDir.name);
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = iSize;
        pItem->m_dateTime=localTime;
        if (hidden)
          pItem->SetProperty("file:hidden", true);
        items.Add(pItem);
      }
    }
  }

  return true;
}

int CSMBDirectory::Open(const CURL &url)
{
  smb.Init();
  CStdString strAuth;
  return OpenDir(url, strAuth);
}

/// \brief Checks authentication against SAMBA share and prompts for username and password if needed
/// \param strAuth The SMB style path
/// \return SMB file descriptor
int CSMBDirectory::OpenDir(const CURL& url, CStdString& strAuth)
{
  int fd = -1;
#ifdef TARGET_WINDOWS
  int nt_error;
#endif

  /* make a writeable copy */
  CURL urlIn(url);

  CPasswordManager::GetInstance().AuthenticateURL(urlIn);
  strAuth = smb.URLEncode(urlIn);

  // remove the / or \ at the end. the samba library does not strip them off
  // don't do this for smb:// !!
  CStdString s = strAuth;
  int len = s.length();
  if (len > 1 && s.at(len - 2) != '/' &&
      (s.at(len - 1) == '/' || s.at(len - 1) == '\\'))
  {
    s.erase(len - 1, 1);
  }

  CLog::Log(LOGDEBUG, "%s - Using authentication url %s", __FUNCTION__, s.c_str());
  { CSingleLock lock(smb);
    fd = smbc_opendir(s.c_str());
  }

  while (fd < 0) /* only to avoid goto in following code */
  {
    CStdString cError;

#ifdef TARGET_WINDOWS
    nt_error = smb.ConvertUnixToNT(errno);

    // if we have an 'invalid handle' error we don't display the error
    // because most of the time this means there is no cdrom in the server's
    // cdrom drive.
    if (nt_error == NT_STATUS_INVALID_HANDLE)
      break;

    if (nt_error == NT_STATUS_ACCESS_DENIED)
    {
      if (m_flags & DIR_FLAG_ALLOW_PROMPT)
        RequireAuthentication(urlIn.Get());
      break;
    }

    if (nt_error == NT_STATUS_OBJECT_NAME_NOT_FOUND)
      cError.Format(g_localizeStrings.Get(770).c_str(),nt_error);
    else
      cError = get_friendly_nt_error_msg(nt_error);

#else

    if (errno == EACCES)
    {
      if (m_flags & DIR_FLAG_ALLOW_PROMPT)
        RequireAuthentication(urlIn.Get());
      break;
    }

    if (errno == ENODEV || errno == ENOENT)
      cError.Format(g_localizeStrings.Get(770).c_str(),errno);
    else
      cError = strerror(errno);

#endif

    if (m_flags & DIR_FLAG_ALLOW_PROMPT)
      SetErrorDialog(257, cError.c_str());
    break;
  }

  if (fd < 0)
  {
    // write error to logfile
#ifdef TARGET_WINDOWS
    CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'", strAuth.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
#else
    CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' error : '%s'", strAuth.c_str(), errno, strerror(errno));
#endif
  }

  return fd;
}

bool CSMBDirectory::Create(const char* strPath)
{
  bool success = true;
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  CStdString strFileName = smb.URLEncode(url);

  int result = smbc_mkdir(strFileName.c_str(), 0);
  success = (result == 0 || EEXIST == errno);
  if(!success)
#ifdef TARGET_WINDOWS
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return success;
}

bool CSMBDirectory::Remove(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  CStdString strFileName = smb.URLEncode(url);

  int result = smbc_rmdir(strFileName.c_str());

  if(result != 0 && errno != ENOENT)
  {
#ifdef TARGET_WINDOWS
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif
    return false;
  }

  return true;
}

bool CSMBDirectory::Exists(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(strPath);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  CStdString strFileName = smb.URLEncode(url);

#ifdef TARGET_WINDOWS
  SMB_STRUCT_STAT info;
#else
  struct stat info;
#endif
  if (smbc_stat(strFileName.c_str(), &info) != 0)
    return false;

  return (info.st_mode & S_IFDIR) ? true : false;
}

CStdString CSMBDirectory::MountShare(const CStdString &smbPath, const CStdString &strType, const CStdString &strName,
    const CStdString &strUser, const CStdString &strPass)
{
#ifdef _LINUX
  UnMountShare(strType, strName);

  CStdString strMountPoint = GetMountPoint(strType, strName);

#if defined(TARGET_DARWIN)
  // Create the directory.
  CURL::Decode(strMountPoint);
  CreateDirectory(strMountPoint, NULL);

  // Massage the path.
  CStdString smbFullPath = "//";
  if (smbFullPath.length() > 0)
  {
    smbFullPath += strUser;
    if (strPass.length() > 0)
      smbFullPath += ":" + strPass;

    smbFullPath += "@";
  }

  CStdString newPath = smbPath;
  newPath.TrimLeft("/");
  smbFullPath += newPath;

  // Make the mount command.
  CStdStringArray args;
  args.push_back("/sbin/mount_smbfs");
  args.push_back("-o");
  args.push_back("nobrowse");
  args.push_back(smbFullPath);
  args.push_back(strMountPoint);

  // Execute it.
  if (CUtil::Command(args))
    return strMountPoint;
#else
  CUtil::SudoCommand("mkdir -p " + strMountPoint);

  CStdString strCmd = "mount -t cifs " + smbPath + " " + strMountPoint +
    " -o rw,nobrl,directio";
  if (!strUser.IsEmpty())
    strCmd += ",user=" + strUser + ",password=" + strPass;
  else
    strCmd += ",guest";

  if (CUtil::SudoCommand(strCmd))
    return strMountPoint;
#endif
#endif
  return StringUtils::EmptyString;
}

void CSMBDirectory::UnMountShare(const CStdString &strType, const CStdString &strName)
{
#if defined(TARGET_DARWIN)
  // Decode the path.
  CStdString strMountPoint = GetMountPoint(strType, strName);
  CURL::Decode(strMountPoint);

  // Make the unmount command.
  CStdStringArray args;
  args.push_back("/sbin/umount");
  args.push_back(strMountPoint);

  // Execute command.
  CUtil::Command(args);
#elif defined(_LINUX)
  CStdString strCmd = "umount " + GetMountPoint(strType, strName);
  CUtil::SudoCommand(strCmd);
#endif
}

CStdString CSMBDirectory::GetMountPoint(const CStdString &strType, const CStdString &strName)
{
  CStdString strPath = strType + strName;
  CURL::Encode(strPath);

#if defined(TARGET_DARWIN)
  CStdString str = getenv("HOME");
  return str + "/" + XBMC_SMB_MOUNT_PATH + strPath;
#else
  return XBMC_SMB_MOUNT_PATH + strPath;
#endif
}
