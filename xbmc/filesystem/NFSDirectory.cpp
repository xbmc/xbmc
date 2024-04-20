/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifdef TARGET_WINDOWS
#include <mutex>

#include <sys\stat.h>
#endif

#include "FileItem.h"
#include "FileItemList.h"
#include "NFSDirectory.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#ifdef TARGET_WINDOWS
#include <sys\stat.h>
#endif

using namespace XFILE;
#include <limits.h>
#include <nfsc/libnfs.h>
#include <nfsc/libnfs-raw-nfs.h>

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

CNFSDirectory::CNFSDirectory(void)
{
  gNfsConnection.AddActiveConnection();
}

CNFSDirectory::~CNFSDirectory(void)
{
  gNfsConnection.AddIdleConnection();
}

bool CNFSDirectory::GetDirectoryFromExportList(const std::string& strPath, CFileItemList &items)
{
  CURL url(strPath);
  std::string nonConstStrPath(strPath);
  std::list<std::string> exportList=gNfsConnection.GetExportList(url);

  for (const std::string& it : exportList)
  {
    const std::string& currentExport(it);
    URIUtils::RemoveSlashAtEnd(nonConstStrPath);

    CFileItemPtr pItem(new CFileItem(currentExport));
    std::string path(nonConstStrPath + currentExport);
    URIUtils::AddSlashAtEnd(path);
    pItem->SetPath(path);
    pItem->m_dateTime = 0;

    pItem->m_bIsFolder = true;
    items.Add(pItem);
  }

  return exportList.empty() ? false : true;
}

bool CNFSDirectory::GetServerList(CFileItemList &items)
{
  struct nfs_server_list *srvrs;
  struct nfs_server_list *srv;
  bool ret = false;

  srvrs = nfs_find_local_servers();

  for (srv=srvrs; srv; srv = srv->next)
  {
      std::string currentExport(srv->addr);

      CFileItemPtr pItem(new CFileItem(currentExport));
      std::string path("nfs://" + currentExport);
      URIUtils::AddSlashAtEnd(path);
      pItem->m_dateTime=0;

      pItem->SetPath(path);
      pItem->m_bIsFolder = true;
      items.Add(pItem);
      ret = true; //added at least one entry
  }
  free_nfs_srvr_list(srvrs);

  return ret;
}

bool CNFSDirectory::ResolveSymlink( const std::string &dirName, struct nfsdirent *dirent, CURL &resolvedUrl)
{
  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  int ret = 0;
  bool retVal = true;
  std::string fullpath = dirName;
  char resolvedLink[MAX_PATH];

  URIUtils::AddSlashAtEnd(fullpath);
  fullpath.append(dirent->name);

  resolvedUrl.Reset();
  resolvedUrl.SetPort(2049);
  resolvedUrl.SetProtocol("nfs");
  resolvedUrl.SetHostName(gNfsConnection.GetConnectedIp());

  ret = nfs_readlink(gNfsConnection.GetNfsContext(), fullpath.c_str(), resolvedLink, MAX_PATH);

  if(ret == 0)
  {
    nfs_stat_64 tmpBuffer = {};
    fullpath = dirName;
    URIUtils::AddSlashAtEnd(fullpath);
    fullpath.append(resolvedLink);

    //special case - if link target is absolute it could be even another export
    //intervolume symlinks baby ...
    if(resolvedLink[0] == '/')
    {
      //use the special stat function for using an extra context
      //because we are inside of a dir traversal
      //and just can't change the global nfs context here
      //without destroying something...
      fullpath = resolvedLink;
      resolvedUrl.SetFileName(fullpath);
      ret = gNfsConnection.stat(resolvedUrl, &tmpBuffer);
    }
    else
    {
      ret = nfs_stat64(gNfsConnection.GetNfsContext(), fullpath.c_str(), &tmpBuffer);
      resolvedUrl.SetFileName(gNfsConnection.GetConnectedExport() + fullpath);
    }

    if (ret != 0)
    {
      CLog::Log(LOGERROR, "NFS: Failed to stat({}) on link resolve {}", fullpath,
                nfs_get_error(gNfsConnection.GetNfsContext()));
      retVal = false;
    }
    else
    {
      dirent->inode = tmpBuffer.nfs_ino;
      dirent->mode = tmpBuffer.nfs_mode;
      dirent->size = tmpBuffer.nfs_size;
      dirent->atime.tv_sec = tmpBuffer.nfs_atime;
      dirent->mtime.tv_sec = tmpBuffer.nfs_mtime;
      dirent->ctime.tv_sec = tmpBuffer.nfs_ctime;

      //map stat mode to nf3type
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
    CLog::Log(LOGERROR, "Failed to readlink({}) {}", fullpath,
              nfs_get_error(gNfsConnection.GetNfsContext()));
    retVal = false;
  }
  return retVal;
}

bool CNFSDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // We accept nfs://server/path[/file]]]]
  int ret = 0;
  KODI::TIME::FileTime fileTime, localTime;
  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  std::string strDirName="";
  std::string myStrPath(url.Get());
  URIUtils::AddSlashAtEnd(myStrPath); //be sure the dir ends with a slash

  if(!gNfsConnection.Connect(url,strDirName))
  {
    //connect has failed - so try to get the exported filesystems if no path is given to the url
    if(url.GetShareName().empty())
    {
      if(url.GetHostName().empty())
      {
        return GetServerList(items);
      }
      else
      {
        return GetDirectoryFromExportList(myStrPath, items);
      }
    }
    else
    {
      return false;
    }
  }

  struct nfsdir *nfsdir = NULL;
  struct nfsdirent *nfsdirent = NULL;

  ret = nfs_opendir(gNfsConnection.GetNfsContext(), strDirName.c_str(), &nfsdir);

  if(ret != 0)
  {
    CLog::Log(LOGERROR, "Failed to open({}) {}", strDirName,
              nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  lock.unlock();

  while((nfsdirent = nfs_readdir(gNfsConnection.GetNfsContext(), nfsdir)) != NULL)
  {
    struct nfsdirent tmpDirent = *nfsdirent;
    std::string strName = tmpDirent.name;
    std::string path(myStrPath + strName);
    int64_t iSize = 0;
    bool bIsDir = false;
    int64_t lTimeDate = 0;

    //reslove symlinks
    if(tmpDirent.type == NF3LNK)
    {
      CURL linkUrl;
      //resolve symlink changes tmpDirent and strName
      if(!ResolveSymlink(strDirName,&tmpDirent,linkUrl))
      {
        continue;
      }

      path = linkUrl.Get();
    }

    iSize = tmpDirent.size;
    bIsDir = tmpDirent.type == NF3DIR;
    lTimeDate = tmpDirent.mtime.tv_sec;

    if (!StringUtils::EqualsNoCase(strName,".") && !StringUtils::EqualsNoCase(strName,"..")
        && !StringUtils::EqualsNoCase(strName,"lost+found"))
    {
      if(lTimeDate == 0) // if modification date is missing, use create date
      {
        lTimeDate = tmpDirent.ctime.tv_sec;
      }

      long long ll = lTimeDate & 0xffffffff;
      ll *= 10000000ll;
      ll += 116444736000000000ll;
      fileTime.lowDateTime = (DWORD)(ll & 0xffffffff);
      fileTime.highDateTime = (DWORD)(ll >> 32);
      KODI::TIME::FileTimeToLocalFileTime(&fileTime, &localTime);

      CFileItemPtr pItem(new CFileItem(tmpDirent.name));
      pItem->m_dateTime=localTime;
      pItem->m_dwSize = iSize;

      if (bIsDir)
      {
        URIUtils::AddSlashAtEnd(path);
        pItem->m_bIsFolder = true;
      }
      else
      {
        pItem->m_bIsFolder = false;
      }

      if (strName[0] == '.')
      {
        pItem->SetProperty("file:hidden", true);
      }
      pItem->SetPath(path);
      items.Add(pItem);
    }
  }

  lock.lock();
  nfs_closedir(gNfsConnection.GetNfsContext(), nfsdir);//close the dir
  lock.unlock();
  return true;
}

bool CNFSDirectory::Create(const CURL& url2)
{
  int ret = 0;
  bool success=true;

  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//mkdir fails if a slash is at the end!!!
  CURL url(folderName);
  folderName = "";

  if(!gNfsConnection.Connect(url,folderName))
    return false;

  ret = nfs_mkdir(gNfsConnection.GetNfsContext(), folderName.c_str());

  success = (ret == 0 || -EEXIST == ret);
  if(!success)
    CLog::Log(LOGERROR, "NFS: Failed to create({}) {}", folderName,
              nfs_get_error(gNfsConnection.GetNfsContext()));
  return success;
}

bool CNFSDirectory::Remove(const CURL& url2)
{
  int ret = 0;

  std::unique_lock<CCriticalSection> lock(gNfsConnection);
  std::string folderName(url2.Get());
  URIUtils::RemoveSlashAtEnd(folderName);//rmdir fails if a slash is at the end!!!
  CURL url(folderName);
  folderName = "";

  if(!gNfsConnection.Connect(url,folderName))
    return false;

  ret = nfs_rmdir(gNfsConnection.GetNfsContext(), folderName.c_str());

  if(ret != 0 && errno != ENOENT)
  {
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__,
              nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  return true;
}

bool CNFSDirectory::Exists(const CURL& url2)
{
  int ret = 0;

  std::unique_lock<CCriticalSection> lock(gNfsConnection);
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
