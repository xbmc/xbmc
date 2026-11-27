/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NFSDirectory.h"

#include "FileItemList.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#ifdef TARGET_WINDOWS
#include <mutex>

#include <sys\stat.h>
#endif

#include <nfsc/libnfs-raw-nfs.h>
#include <nfsc/libnfs.h>

#if defined(TARGET_WINDOWS)
#define S_IFLNK 0120000
#define S_ISBLK(m) (0)
#define S_ISSOCK(m) (0)
#define S_ISLNK(m) ((m & S_IFLNK) != 0)
#define S_ISCHR(m) ((m & _S_IFCHR) != 0)
#define S_ISDIR(m) ((m & _S_IFDIR) != 0)
#define S_ISFIFO(m) ((m & _S_IFIFO) != 0)
#define S_ISREG(m) ((m & _S_IFREG) != 0)
#endif

using namespace XFILE;

namespace
{
constexpr int NFS_MAX_PATH = 4096;

KODI::TIME::FileTime GetDirEntryTime(const struct nfsdirent* dirent)
{
  // if modification date is missing, use create date
  const int64_t timeDate =
      (dirent->mtime.tv_sec == 0) ? dirent->ctime.tv_sec : dirent->mtime.tv_sec;

  long long ll = timeDate & 0xffffffff;
  ll *= 10000000ll;
  ll += 116444736000000000ll;

  KODI::TIME::FileTime fileTime{};
  fileTime.lowDateTime = static_cast<DWORD>(ll & 0xffffffff);
  fileTime.highDateTime = static_cast<DWORD>(ll >> 32);

  KODI::TIME::FileTime localTime{};
  KODI::TIME::FileTimeToLocalFileTime(&fileTime, &localTime);

  return localTime;
}

void SetDirentFromStat(struct nfsdirent& dirent, const nfs_stat_64& targetStat)
{
  dirent.inode = targetStat.nfs_ino;
  dirent.mode = static_cast<uint32_t>(targetStat.nfs_mode);
  dirent.size = targetStat.nfs_size;
  dirent.atime.tv_sec = targetStat.nfs_atime;
  dirent.mtime.tv_sec = targetStat.nfs_mtime;
  dirent.ctime.tv_sec = targetStat.nfs_ctime;

  // map stat mode to nf3type
  if (S_ISBLK(targetStat.nfs_mode))
  {
    dirent.type = NF3BLK;
  }
  else if (S_ISCHR(targetStat.nfs_mode))
  {
    dirent.type = NF3CHR;
  }
  else if (S_ISDIR(targetStat.nfs_mode))
  {
    dirent.type = NF3DIR;
  }
  else if (S_ISFIFO(targetStat.nfs_mode))
  {
    dirent.type = NF3FIFO;
  }
  else if (S_ISREG(targetStat.nfs_mode))
  {
    dirent.type = NF3REG;
  }
  else if (S_ISLNK(targetStat.nfs_mode))
  {
    dirent.type = NF3LNK;
  }
  else if (S_ISSOCK(targetStat.nfs_mode))
  {
    dirent.type = NF3SOCK;
  }
}

// Gives a symlink's dirent the type, size and times of its target. libnfs follows a relative link
// from the link's folder, anywhere in a path, so such a link keeps its own path, as in
// CPosixDirectory. An absolute link is stat'ed by its target's URL, on a context of its own, as it
// can point into another export, and targetUrl gets that URL to list it under. It keeps its own
// path only if it is a file that libnfs resolves by that path to the same file: libnfs resolves an
// absolute link from the link's folder on NFSv4, and on NFSv3 drops the rest of a path that goes
// through one, so a file inside a linked folder couldn't be opened.
bool ResolveSymlink(const std::string& dirName, struct nfsdirent& dirent, std::string& targetUrl)
{
  std::unique_lock lock(gNfsConnection);

  const std::string fullpath{dirName + dirent.name};
  nfs_stat_64 targetStat{};

  char link[NFS_MAX_PATH];
  if (nfs_readlink(gNfsConnection.GetNfsContext(), fullpath.c_str(), link, NFS_MAX_PATH) != 0)
  {
    CLog::LogF(LOGERROR, "Failed to readlink '{}' ({})", fullpath,
               nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }

  if (link[0] != '/')
  {
    if (nfs_stat64(gNfsConnection.GetNfsContext(), fullpath.c_str(), &targetStat) != 0)
    {
      CLog::LogF(LOGERROR, "Failed to stat '{}' on link resolve ({})", fullpath,
                 nfs_get_error(gNfsConnection.GetNfsContext()));
      return false;
    }
    SetDirentFromStat(dirent, targetStat);
    return true;
  }

  CURL url;
  url.SetPort(2049);
  url.SetProtocol("nfs");
  url.SetHostName(gNfsConnection.GetConnectedIp());
  url.SetFileName(link);
  if (gNfsConnection.stat(url, &targetStat) != 0)
  {
    CLog::LogF(LOGERROR, "Failed to stat '{}' on link resolve", link);
    return false;
  }
  SetDirentFromStat(dirent, targetStat);

  if (!S_ISDIR(targetStat.nfs_mode))
  {
    nfs_stat_64 linkStat{};
    // A fileid identifies a file only together with its filesystem's id, which libnfs 5.0.2
    // leaves at 0 on NFSv4.
    if (nfs_stat64(gNfsConnection.GetNfsContext(), fullpath.c_str(), &linkStat) == 0 &&
        linkStat.nfs_dev != 0 && linkStat.nfs_dev == targetStat.nfs_dev &&
        linkStat.nfs_ino == targetStat.nfs_ino)
      return true;
  }

  targetUrl = url.Get();
  return true;
}

} // Unnamed namespace

CNFSDirectory::CNFSDirectory(void)
{
  gNfsConnection.AddActiveConnection();
}

CNFSDirectory::~CNFSDirectory(void)
{
  gNfsConnection.AddIdleConnection();
}

std::vector<std::shared_ptr<CFileItem>> CNFSDirectory::GetDirectoryFromExportList(
    const CURL& inputURL) const
{
  std::string pathWithSlash(inputURL.Get());
  URIUtils::AddSlashAtEnd(pathWithSlash); //be sure the dir ends with a slash

  CURL url(pathWithSlash);

  std::string pathWithoutSlash(pathWithSlash);
  URIUtils::RemoveSlashAtEnd(pathWithoutSlash);

  std::list<std::string> exportList = gNfsConnection.GetExportList(url);

  std::vector<std::shared_ptr<CFileItem>> fileItems;
  for (const std::string& currentExport : exportList)
  {
    std::string path(pathWithoutSlash + currentExport);
    URIUtils::AddSlashAtEnd(path);

    const auto& item = fileItems.emplace_back(std::make_shared<CFileItem>(currentExport));
    item->SetPath(std::move(path));
    item->SetDateTime(0);
    item->SetFolder(true);
  }
  return fileItems;
}

std::vector<std::shared_ptr<CFileItem>> CNFSDirectory::GetServerList() const
{
  struct nfs_server_list* srvrs = nfs_find_local_servers();
  std::vector<std::shared_ptr<CFileItem>> fileItems;
  for (struct nfs_server_list* srv = srvrs; srv; srv = srv->next)
  {
    std::string serverAddress = srv->addr;

    std::string path("nfs://" + serverAddress);
    URIUtils::AddSlashAtEnd(path);

    const auto& item = fileItems.emplace_back(std::make_shared<CFileItem>(serverAddress));
    item->SetPath(std::move(path));
    item->SetDateTime(0);
    item->SetFolder(true);
  }
  free_nfs_srvr_list(srvrs);

  return fileItems;
}

bool CNFSDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // We accept nfs://server/path[/file]]]]
  std::unique_lock lock(gNfsConnection);

  std::string strDirName = "";
  if (!gNfsConnection.Connect(url, strDirName))
  {
    //connect has failed - so try to get the exported filesystems if no path is given to the url
    if (url.GetShareName().empty())
    {
      std::vector<std::shared_ptr<CFileItem>> fileItems =
          url.GetHostName().empty() ? GetServerList() : GetDirectoryFromExportList(url);
      bool ret = !fileItems.empty();
      items.AddItems(std::move(fileItems));
      return ret;
    }
    return false;
  }

  struct nfsdir* nfsdir = nullptr;
  if (nfs_opendir(gNfsConnection.GetNfsContext(), strDirName.c_str(), &nfsdir) != 0)
  {
    CLog::LogF(LOGERROR, "Failed to open '{}' ({})", strDirName,
               nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  lock.unlock();

  std::string myStrPath(url.Get());
  URIUtils::AddSlashAtEnd(myStrPath);
  URIUtils::AddSlashAtEnd(strDirName);

  std::vector<std::shared_ptr<CFileItem>> fileItems;
  struct nfsdirent* cachedDirent = nullptr;
  while ((cachedDirent = nfs_readdir(gNfsConnection.GetNfsContext(), nfsdir)) != nullptr)
  {
    // The next nfs_opendir() of this folder gets the same entries from the context's cache while
    // the folder is unchanged, so a symlink is resolved into a copy.
    struct nfsdirent dirent = *cachedDirent;
    const std::string name{dirent.name};

    if (name == "." || name == ".." || name == "lost+found")
      continue;

    std::string path{myStrPath + name};
    if (dirent.type == NF3LNK)
    {
      std::string targetUrl;
      if (!ResolveSymlink(strDirName, dirent, targetUrl))
        continue;
      if (!targetUrl.empty())
        path = std::move(targetUrl);
    }

    const bool isDir = dirent.type == NF3DIR;
    if (isDir)
      URIUtils::AddSlashAtEnd(path);

    const auto& item = fileItems.emplace_back(std::make_shared<CFileItem>(name));
    item->SetPath(std::move(path));
    item->SetDateTime(GetDirEntryTime(&dirent));
    item->SetFolder(isDir);
    item->SetSize(dirent.size);

    // raw values; the DateTime above is local-time converted
    item->SetProperty(DIR_PROPERTY_STAT_MTIME, static_cast<int64_t>(dirent.mtime.tv_sec));
    item->SetProperty(DIR_PROPERTY_STAT_CTIME, static_cast<int64_t>(dirent.ctime.tv_sec));

    if (name[0] == '.')
      item->SetProperty("file:hidden", true);
  }
  items.AddItems(std::move(fileItems));

  lock.lock();
  nfs_closedir(gNfsConnection.GetNfsContext(), nfsdir); //close the dir
  lock.unlock();
  return true;
}

bool CNFSDirectory::Create(const CURL& url2)
{
  int ret = 0;
  bool success=true;

  std::unique_lock lock(gNfsConnection);
  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//mkdir fails if a slash is at the end!!!
  CURL url(folderName);
  folderName = "";

  if(!gNfsConnection.Connect(url,folderName))
    return false;

  ret = nfs_mkdir(gNfsConnection.GetNfsContext(), folderName.c_str());

  success = (ret == 0 || -EEXIST == ret);
  if (!success)
    CLog::LogF(LOGERROR, "Failed to create '{}' ({})", folderName,
               nfs_get_error(gNfsConnection.GetNfsContext()));
  return success;
}

bool CNFSDirectory::Remove(const CURL& url2)
{
  int ret = 0;

  std::unique_lock lock(gNfsConnection);
  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//rmdir fails if a slash is at the end!!!
  CURL url(folderName);
  folderName = "";

  if(!gNfsConnection.Connect(url,folderName))
    return false;

  ret = nfs_rmdir(gNfsConnection.GetNfsContext(), folderName.c_str());

  if (ret != 0 && errno != ENOENT)
  {
    CLog::LogF(LOGERROR, "Failed to remove '{}' ({})", folderName,
               nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  return true;
}

bool CNFSDirectory::Exists(const CURL& url2)
{
  int ret = 0;

  std::unique_lock lock(gNfsConnection);
  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//remove slash at end or URIUtils::GetFileName won't return what we want...
  CURL url(folderName);
  folderName = "";

  if(!gNfsConnection.Connect(url,folderName))
    return false;

  nfs_stat_64 info;
  ret = nfs_stat64(gNfsConnection.GetNfsContext(), folderName.c_str(), &info);

  if (ret != 0)
  {
    return false;
  }
  return S_ISDIR(info.nfs_mode) ? true : false;
}
