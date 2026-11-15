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

// locks the context for each call made on it
bool ResolveSymlink(CNfsContext& context,
                    const std::string& dirName,
                    struct nfsdirent* dirent,
                    std::string& resolvedPath)
{
  bool retVal{true};
  std::string fullpath{dirName + dirent->name};

  char resolvedLink[NFS_MAX_PATH];
  std::unique_lock lock(context);
  int ret{nfs_readlink(context.GetNfsContext(), fullpath.c_str(), resolvedLink, NFS_MAX_PATH)};

  if (ret == 0)
  {
    lock.unlock();
    nfs_stat_64 tmpBuffer{};
    std::string error;

    CURL resolvedUrl;
    resolvedUrl.SetPort(2049);
    resolvedUrl.SetProtocol("nfs");
    resolvedUrl.SetHostName(context.GetConnectedIp());

    // special case - if link target is absolute it could be even another export
    // intervolume symlinks baby ...
    if (resolvedLink[0] == '/')
    {
      // use the special stat function for using an extra context
      // because we are inside of a dir traversal
      // and just can't change the global nfs context here
      // without destroying something...
      fullpath = resolvedLink;
      resolvedUrl.SetFileName(fullpath);
      ret = gNfsConnection.stat(resolvedUrl, &tmpBuffer);
    }
    else
    {
      fullpath = dirName + resolvedLink;
      lock.lock();
      ret = nfs_stat64(context.GetNfsContext(), fullpath.c_str(), &tmpBuffer);
      if (ret != 0)
        error = nfs_get_error(context.GetNfsContext());
      lock.unlock();
      resolvedUrl.SetFileName(context.GetConnectedExport() + fullpath);
    }

    if (ret != 0)
    {
      CLog::LogF(LOGERROR, "Failed to stat '{}' on link resolve ({})", fullpath, error);
      retVal = false;
    }
    else
    {
      resolvedPath = resolvedUrl.Get();

      dirent->inode = tmpBuffer.nfs_ino;
      dirent->mode = static_cast<uint32_t>(tmpBuffer.nfs_mode);
      dirent->size = tmpBuffer.nfs_size;
      dirent->atime.tv_sec = tmpBuffer.nfs_atime;
      dirent->mtime.tv_sec = tmpBuffer.nfs_mtime;
      dirent->ctime.tv_sec = tmpBuffer.nfs_ctime;

      // map stat mode to nf3type
      if (S_ISBLK(tmpBuffer.nfs_mode))
      {
        dirent->type = NF3BLK;
      }
      else if (S_ISCHR(tmpBuffer.nfs_mode))
      {
        dirent->type = NF3CHR;
      }
      else if (S_ISDIR(tmpBuffer.nfs_mode))
      {
        dirent->type = NF3DIR;
      }
      else if (S_ISFIFO(tmpBuffer.nfs_mode))
      {
        dirent->type = NF3FIFO;
      }
      else if (S_ISREG(tmpBuffer.nfs_mode))
      {
        dirent->type = NF3REG;
      }
      else if (S_ISLNK(tmpBuffer.nfs_mode))
      {
        dirent->type = NF3LNK;
      }
      else if (S_ISSOCK(tmpBuffer.nfs_mode))
      {
        dirent->type = NF3SOCK;
      }
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "Failed to readlink '{}' ({})", fullpath,
               nfs_get_error(context.GetNfsContext()));
    retVal = false;
  }
  return retVal;
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
  std::string strDirName = "";
  const CNfsContextLease context = gNfsConnection.GetContext(url, strDirName);
  if (!context)
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
  std::unique_lock lock(*context);
  if (nfs_opendir(context->GetNfsContext(), strDirName.c_str(), &nfsdir) != 0)
  {
    CLog::LogF(LOGERROR, "Failed to open '{}' ({})", strDirName,
               nfs_get_error(context->GetNfsContext()));
    return false;
  }
  // nfs_readdir() only walks the entries nfs_opendir() read, in an nfsdir that nothing else on the
  // context touches until nfs_closedir() returns it to the context's cache. So the context is
  // locked only for the calls that use it, and other users of a shared context get in between.
  lock.unlock();

  std::string myStrPath(url.Get());
  URIUtils::AddSlashAtEnd(myStrPath);
  URIUtils::AddSlashAtEnd(strDirName);

  std::string resolvedPath;
  std::vector<std::shared_ptr<CFileItem>> fileItems;
  struct nfsdirent* dirent = nullptr;
  while ((dirent = nfs_readdir(context->GetNfsContext(), nfsdir)) != nullptr)
  {
    const std::string& name = dirent->name;

    //resolve symlinks
    //resolve symlink changes dirent and name
    const bool isSymLink = dirent->type == NF3LNK;
    if (isSymLink && !ResolveSymlink(*context, strDirName, dirent, resolvedPath))
      continue;

    if (name == "." || name == ".." || name == "lost+found")
      continue;

    const bool isDir = dirent->type == NF3DIR;

    std::string path = isSymLink ? resolvedPath : (myStrPath + name);
    if (isDir)
      URIUtils::AddSlashAtEnd(path);

    const auto& item = fileItems.emplace_back(std::make_shared<CFileItem>(name));
    item->SetPath(std::move(path));
    item->SetDateTime(GetDirEntryTime(dirent));
    item->SetFolder(isDir);
    item->SetSize(dirent->size);

    // raw values; the DateTime above is local-time converted
    item->SetProperty(DIR_PROPERTY_STAT_MTIME, static_cast<int64_t>(dirent->mtime.tv_sec));
    item->SetProperty(DIR_PROPERTY_STAT_CTIME, static_cast<int64_t>(dirent->ctime.tv_sec));

    if (name[0] == '.')
      item->SetProperty("file:hidden", true);
  }
  items.AddItems(std::move(fileItems));

  lock.lock();
  nfs_closedir(context->GetNfsContext(), nfsdir); //close the dir
  return true;
}

bool CNFSDirectory::Create(const CURL& url2)
{
  int ret = 0;
  bool success=true;

  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//mkdir fails if a slash is at the end!!!
  CURL url(folderName);
  folderName = "";

  const CNfsContextLease context = gNfsConnection.GetContext(url, folderName);
  if (!context)
    return false;

  std::unique_lock lock(*context);
  ret = nfs_mkdir(context->GetNfsContext(), folderName.c_str());

  success = (ret == 0 || -EEXIST == ret);
  if (!success)
    CLog::LogF(LOGERROR, "Failed to create '{}' ({})", folderName,
               nfs_get_error(context->GetNfsContext()));
  return success;
}

bool CNFSDirectory::Remove(const CURL& url2)
{
  int ret = 0;

  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//rmdir fails if a slash is at the end!!!
  CURL url(folderName);
  folderName = "";

  const CNfsContextLease context = gNfsConnection.GetContext(url, folderName);
  if (!context)
    return false;

  std::unique_lock lock(*context);
  ret = nfs_rmdir(context->GetNfsContext(), folderName.c_str());

  if (ret != 0 && ret != -ENOENT)
  {
    CLog::LogF(LOGERROR, "Failed to remove '{}' ({})", folderName,
               nfs_get_error(context->GetNfsContext()));
    return false;
  }
  return true;
}

bool CNFSDirectory::Exists(const CURL& url2)
{
  int ret = 0;

  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//remove slash at end or URIUtils::GetFileName won't return what we want...
  CURL url(folderName);
  folderName = "";

  const CNfsContextLease context = gNfsConnection.GetContext(url, folderName);
  if (!context)
    return false;

  nfs_stat_64 info;
  std::unique_lock lock(*context);
  ret = nfs_stat64(context->GetNfsContext(), folderName.c_str(), &info);

  if (ret != 0)
  {
    return false;
  }
  return S_ISDIR(info.nfs_mode) ? true : false;
}
