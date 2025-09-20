/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
* know bugs:
* - when opening a server for the first time with ip address and the second time
*   with server name, access to the server is denied.
* - when browsing entire network, user can't go back one step
*   share = smb://, user selects a workgroup, user selects a server.
*   doing ".." will go back to smb:// (entire network) and not to workgroup list.
*
* debugging is set to a max of 10 for release builds (see local.h)
*/

#include "SMBDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "PasswordManager.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include "platform/posix/filesystem/SMBWSDiscovery.h"

#include <mutex>

#include <fmt/format.h>
#include <libsmbclient.h>

using namespace XFILE;

namespace
{

KODI::TIME::FileTime GetDirEntryTime(const struct stat& st)
{
  int64_t timeDate = 0;

  // only stat files that can give proper responses
  if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))
  {
    // if modification date is missing, use create date
    timeDate = (st.st_mtime == 0) ? st.st_ctime : st.st_mtime;
  }

  KODI::TIME::FileTime fileTime{};
  KODI::TIME::FileTime localTime{};
  KODI::TIME::TimeTToFileTime(timeDate, &fileTime);
  KODI::TIME::FileTimeToLocalFileTime(&fileTime, &localTime);

  return localTime;
}

bool CanDiscoverServers()
{
  if (const auto settingsComponent = CServiceBroker::GetSettingsComponent(); settingsComponent)
    if (const auto settings = settingsComponent->GetSettings(); settings)
      // Check WS-Discovery daemon enabled, if not return as smb:// cant be handled further
      return settings->GetBool(CSettings::SETTING_SERVICES_WSDISCOVERY);

  return false;
}

} // Unnamed namespace

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
  std::unique_lock lock(smb);
  smb.Init();
  lock.unlock(); // OpenDir is locked

  // if url provided does not having anything except smb protocol
  // Do a WS-Discovery search to find possible smb servers to mimic smbv1 behaviour
  std::string strRoot = url.Get();
  if (strRoot == "smb://")
  {
    if (CanDiscoverServers())
    {
      WSDiscovery::CWSDiscoveryPosix& WSInstance =
          dynamic_cast<WSDiscovery::CWSDiscoveryPosix&>(CServiceBroker::GetWSDiscovery());
      return WSInstance.GetServerList(items);
    }

    return false;
  }

  std::string strAuth;
  int fd = OpenDir(url, strAuth);
  if (fd < 0)
    return false;

  lock.lock();
  if (!smb.IsSmbValid())
    return false;

  const libsmb_file_info* fi;
  struct stat st;
  std::vector<CFileItemPtr> fileItems;
  while ((fi = smbc_readdirplus2(fd, &st)))
  {
    // We use UTF-8 internally, as does SMB
    std::string name = fi->name;

    if (name == "." || name == ".." || name == "lost+found")
      continue;

    URIUtils::AddSlashAtEnd(strRoot);
    std::string path = strRoot + name;

    int64_t size = 0;
    const bool isDir = S_ISDIR(st.st_mode);
    bool hidden = name.starts_with('.');

    // only stat files that can give proper responses
    if (S_ISREG(st.st_mode) || isDir)
    {
      // This is also present in stuct stat using S_IXOTH but given
      // the potential for confusion, let's be explicit.
      hidden = hidden || (fi->attrs & SMBC_DOS_MODE_HIDDEN);

      size = st.st_size;
    }

    if (isDir)
      URIUtils::AddSlashAtEnd(path);

    const auto& item = fileItems.emplace_back(std::make_shared<CFileItem>(name));
    item->SetPath(path);
    item->SetDateTime(GetDirEntryTime(st));
    item->SetFolder(isDir);
    if (!isDir)
      item->SetSize(size);
    if (hidden)
      item->SetProperty("file:hidden", true);
  }

  // No results from smbc_readdirplus2() suggests server or share browsing.
  // Use smbclient's legacy API for this.
  if (fileItems.empty())
  {
    if (smbc_lseekdir(fd, 0) < 0)
    {
      CLog::LogF(LOGERROR, "Unable to seek directory : '{}'\nunix_err:'{:x}' error: '{}'",
                 CURL::GetRedacted(strAuth), errno, strerror(errno));
      return false;
    }

    struct smbc_dirent* dirent;
    while ((dirent = smbc_readdir(fd)))
    {
      if (dirent->smbc_type != SMBC_FILE_SHARE && dirent->smbc_type != SMBC_SERVER)
        continue;

      std::string path(strRoot);

      // needed for network / workgroup browsing
      // skip if root if we are given a server
      if (dirent->smbc_type == SMBC_SERVER)
      {
        /* create url with same options, user, pass.. but no filename or host*/
        CURL rooturl(strRoot);
        rooturl.SetFileName("");
        rooturl.SetHostName("");
        path = smb.URLEncode(rooturl);
      }
      path = URIUtils::AddFileToFolder(path, dirent->name);
      URIUtils::AddSlashAtEnd(path);

      const auto& item = fileItems.emplace_back(std::make_shared<CFileItem>(dirent->name));
      item->SetPath(path);
      item->SetFolder(true);
    }
  }
  items.AddItems(std::move(fileItems));

  if (smbc_closedir(fd) < 0)
  {
    CLog::LogF(LOGERROR, "Unable to close directory : '{}'\nunix_err:'{:x}' error: '{}'",
               CURL::GetRedacted(strAuth), errno, strerror(errno));
    return true; // we already got our listing
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
  bool guest = false;

  /* make a writeable copy */
  CURL urlIn = CSMB::GetResolvedUrl(url);

  CPasswordManager::GetInstance().AuthenticateURL(urlIn);

  // set the username to "guest" when not provided username
  // which is required for passwordless or everyone access
  if (urlIn.GetUserName().empty())
  {
    urlIn.SetUserName("guest");
    urlIn.SetPassword(" ");
    guest = true;
  }

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

  CLog::LogFC(LOGDEBUG, LOGSAMBA, "Using authentication url {}", CURL::GetRedacted(s));

  {
    std::unique_lock lock(smb);
    if (!smb.IsSmbValid())
      return -1;
    fd = smbc_opendir(s.c_str());
  }

  while (fd < 0) /* only to avoid goto in following code */
  {
    std::string cError;

    if (errno == EACCES || errno == EPERM || errno == EAGAIN || errno == EINVAL)
    {
      if (m_flags & DIR_FLAG_ALLOW_PROMPT)
      {
        if (guest) // clean user/pass if tried guest access and failed
        {
          urlIn.SetUserName("");
          urlIn.SetPassword("");
        }
        RequireAuthentication(urlIn);
      }
      break;
    }

    if (errno == ENODEV || errno == ENOENT)
      cError = fmt::format(fmt::runtime(g_localizeStrings.Get(770)), errno);
    else
      cError = strerror(errno);

    if (m_flags & DIR_FLAG_ALLOW_PROMPT)
      SetErrorDialog(257, cError.c_str());
    break;
  }

  if (fd < 0)
  {
    // write error to logfile
    CLog::LogF(LOGERROR, "Unable to open directory : '{}'\nunix_err:'{:x}' error : '{}'",
               CURL::GetRedacted(strAuth), errno, strerror(errno));
  }

  return fd;
}

bool CSMBDirectory::Create(const CURL& url2)
{
  std::unique_lock lock(smb);
  smb.Init();

  CURL url = CSMB::GetResolvedUrl(url2);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string strFileName = smb.URLEncode(url);

  int result = smbc_mkdir(strFileName.c_str(), 0);
  bool success = (result == 0 || EEXIST == errno);
  if(!success)
    CLog::LogF(LOGERROR, "Error( {} )", strerror(errno));

  return success;
}

bool CSMBDirectory::Remove(const CURL& url2)
{
  std::unique_lock lock(smb);
  smb.Init();

  CURL url = CSMB::GetResolvedUrl(url2);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string strFileName = smb.URLEncode(url);

  int result = smbc_rmdir(strFileName.c_str());

  if(result != 0 && errno != ENOENT)
  {
    CLog::LogF(LOGERROR, "Error( {} )", strerror(errno));
    return false;
  }

  return true;
}

bool CSMBDirectory::Exists(const CURL& url2)
{
  std::unique_lock lock(smb);
  smb.Init();

  CURL url = CSMB::GetResolvedUrl(url2);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string strFileName = smb.URLEncode(url);

  struct stat info;
  if (smbc_stat(strFileName.c_str(), &info) != 0)
    return false;

  return S_ISDIR(info.st_mode);
}

