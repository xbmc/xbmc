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
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"
#include "PasswordManager.h"

#include <libsmbclient.h>

struct CachedDirEntry
{
  unsigned int type;
  std::string name;
};

using namespace XFILE;
using namespace std;

CSMBDirectory::CSMBDirectory(void)
{
  smb.AddActiveConnection();
}

CSMBDirectory::~CSMBDirectory(void)
{
  smb.AddIdleConnection();
}

bool CSMBDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]

  /* samba isn't thread safe with old interface, always lock */
  CSingleLock lock(smb);

  smb.Init();

  //Separate roots for the authentication and the containing items to allow browsing to work correctly
  std::string strRoot = url.Get();
  std::string strAuth;

  lock.Leave(); // OpenDir is locked
  int fd = OpenDir(url, strAuth);
  if (fd < 0)
    return false;

  URIUtils::AddSlashAtEnd(strRoot);
  URIUtils::AddSlashAtEnd(strAuth);

  std::string strFile;

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

    if (!strFile.empty() && strFile != "." && strFile != ".."
      && strFile != "lost+found"
      && aDir.type != SMBC_PRINTER_SHARE && aDir.type != SMBC_IPC_SHARE)
    {
     int64_t iSize = 0;
      bool bIsDir = true;
      int64_t lTimeDate = 0;
      bool hidden = false;

      if(StringUtils::EndsWith(strFile, "$") && aDir.type == SMBC_FILE_SHARE )
        continue;

      if (StringUtils::StartsWith(strFile, "."))
        hidden = true;

      // only stat files that can give proper responses
      if ( aDir.type == SMBC_FILE ||
           aDir.type == SMBC_DIR )
      {
        // set this here to if the stat should fail
        bIsDir = (aDir.type == SMBC_DIR);

        struct stat info = {0};
        if ((m_flags & DIR_FLAG_NO_FILE_INFO)==0 && g_advancedSettings.m_sambastatfiles)
        {
          // make sure we use the authenticated path wich contains any default username
          const std::string strFullName = strAuth + smb.URLEncode(strFile);

          lock.Enter();

          if( smbc_stat(strFullName.c_str(), &info) == 0 )
          {

            char value[20];
            // We poll for extended attributes which symbolizes bits but split up into a string. Where 0x02 is hidden and 0x12 is hidden directory.
            // According to the libsmbclient.h it's supposed to return 0 if ok, or the length of the string. It seems always to return the length wich is 4
            if (smbc_getxattr(strFullName.c_str(), "system.dos_attr.mode", value, sizeof(value)) > 0)
            {
              long longvalue = strtol(value, NULL, 16);
              if (longvalue & SMBC_DOS_MODE_HIDDEN)
                hidden = true;
            }
            else
              CLog::Log(LOGERROR, "Getting extended attributes for the share: '%s'\nunix_err:'%x' error: '%s'", CURL::GetRedacted(strFullName).c_str(), errno, strerror(errno));

            bIsDir = S_ISDIR(info.st_mode);
            lTimeDate = info.st_mtime;
            if(lTimeDate == 0) // if modification date is missing, use create date
              lTimeDate = info.st_ctime;
            iSize = info.st_size;
          }
          else
            CLog::Log(LOGERROR, "%s - Failed to stat file %s", __FUNCTION__, CURL::GetRedacted(strFullName).c_str());

          lock.Leave();
        }
      }

      FILETIME fileTime, localTime;
      TimeTToFileTime(lTimeDate, &fileTime);
      FileTimeToLocalFileTime(&fileTime, &localTime);

      if (bIsDir)
      {
        CFileItemPtr pItem(new CFileItem(strFile));
        std::string path(strRoot);

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
  std::string strAuth;
  return OpenDir(url, strAuth);
}

/// \brief Checks authentication against SAMBA share and prompts for username and password if needed
/// \param strAuth The SMB style path
/// \return SMB file descriptor
int CSMBDirectory::OpenDir(const CURL& url, std::string& strAuth)
{
  int fd = -1;

  /* make a writeable copy */
  CURL urlIn(url);

  CPasswordManager::GetInstance().AuthenticateURL(urlIn);
  strAuth = smb.URLEncode(urlIn);

  // remove the / or \ at the end. the samba library does not strip them off
  // don't do this for smb:// !!
  std::string s = strAuth;
  int len = s.length();
  if (len > 1 && s.at(len - 2) != '/' &&
      (s.at(len - 1) == '/' || s.at(len - 1) == '\\'))
  {
    s.erase(len - 1, 1);
  }

  if (g_advancedSettings.CanLogComponent(LOGSAMBA))
    CLog::LogFunction(LOGDEBUG, __FUNCTION__, "Using authentication url %s", CURL::GetRedacted(s).c_str());

  { CSingleLock lock(smb);
    fd = smbc_opendir(s.c_str());
  }

  while (fd < 0) /* only to avoid goto in following code */
  {
    std::string cError;

    if (errno == EACCES)
    {
      if (m_flags & DIR_FLAG_ALLOW_PROMPT)
        RequireAuthentication(urlIn);
      break;
    }

    if (errno == ENODEV || errno == ENOENT)
      cError = StringUtils::Format(g_localizeStrings.Get(770).c_str(),errno);
    else
      cError = strerror(errno);

    if (m_flags & DIR_FLAG_ALLOW_PROMPT)
      SetErrorDialog(257, cError.c_str());
    break;
  }

  if (fd < 0)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' error : '%s'", CURL::GetRedacted(strAuth).c_str(), errno, strerror(errno));
  }

  return fd;
}

bool CSMBDirectory::Create(const CURL& url2)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(url2);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string strFileName = smb.URLEncode(url);

  int result = smbc_mkdir(strFileName.c_str(), 0);
  bool success = (result == 0 || EEXIST == errno);
  if(!success)
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return success;
}

bool CSMBDirectory::Remove(const CURL& url2)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(url2);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string strFileName = smb.URLEncode(url);

  int result = smbc_rmdir(strFileName.c_str());

  if(result != 0 && errno != ENOENT)
  {
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
    return false;
  }

  return true;
}

bool CSMBDirectory::Exists(const CURL& url2)
{
  CSingleLock lock(smb);
  smb.Init();

  CURL url(url2);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string strFileName = smb.URLEncode(url);

  struct stat info;
  if (smbc_stat(strFileName.c_str(), &info) != 0)
    return false;

  return S_ISDIR(info.st_mode);
}

