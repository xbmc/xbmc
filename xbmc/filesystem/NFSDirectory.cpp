/*
 *      Copyright (C) 2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"

#ifdef HAS_FILESYSTEM_NFS
#include "DllLibNfs.h"
#include "NFSDirectory.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"
using namespace XFILE;
using namespace std;
#include <nfsc/libnfs-raw-mount.h>
#include <nfsc/libnfs-raw-nfs.h>

CNFSDirectory::CNFSDirectory(void)
{
  gNfsConnection.AddActiveConnection();
}

CNFSDirectory::~CNFSDirectory(void)
{
  gNfsConnection.AddIdleConnection();
}

bool CNFSDirectory::GetDirectoryFromExportList(const CStdString& strPath, CFileItemList &items)
{
  CURL url(strPath);
  CStdString nonConstStrPath(strPath);
  std::list<CStdString> exportList=gNfsConnection.GetExportList(url);
  std::list<CStdString>::iterator it;
  
  for(it=exportList.begin();it!=exportList.end();it++)
  {
      CStdString currentExport(*it);     
      URIUtils::RemoveSlashAtEnd(nonConstStrPath);
           
      CFileItemPtr pItem(new CFileItem(currentExport));
      CStdString path(nonConstStrPath + currentExport);
      URIUtils::AddSlashAtEnd(path);
      pItem->SetPath(path);
      pItem->m_dateTime=0;

      pItem->m_bIsFolder = true;
      items.Add(pItem);
  }
  
  return exportList.empty()? false : true;
}

bool CNFSDirectory::GetServerList(CFileItemList &items)
{
  struct nfs_server_list *srvrs;
  struct nfs_server_list *srv;
  bool ret = false;

  if(!gNfsConnection.HandleDyLoad())
  {
    return false;
  }

  srvrs = gNfsConnection.GetImpl()->nfs_find_local_servers();	

  for (srv=srvrs; srv; srv = srv->next) 
  {
      CStdString currentExport(srv->addr);

      CFileItemPtr pItem(new CFileItem(currentExport));
      CStdString path("nfs://" + currentExport);
      URIUtils::AddSlashAtEnd(path);
      pItem->m_dateTime=0;

      pItem->SetPath(path);
      pItem->m_bIsFolder = true;
      items.Add(pItem);
      ret = true; //added at least one entry
  }
  gNfsConnection.GetImpl()->free_nfs_srvr_list(srvrs);

  return ret;
}

bool CNFSDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // We accept nfs://server/path[/file]]]]
  int ret = 0;
  FILETIME fileTime, localTime;    
  CSingleLock lock(gNfsConnection); 
  CURL url(strPath);
  CStdString strDirName="";
   
  if(!gNfsConnection.Connect(url,strDirName))
  {
    //connect has failed - so try to get the exported filesystms if no path is given to the url
    if(url.GetShareName().Equals(""))
    {
      if(url.GetHostName().Equals(""))
      {
        return GetServerList(items);
      }
      else 
      {
        return GetDirectoryFromExportList(strPath, items); 
      }
    }
    else
    {
      return false;
    }    
  }
      
  struct nfsdir *nfsdir = NULL;
  struct nfsdirent *nfsdirent = NULL;

  ret = gNfsConnection.GetImpl()->nfs_opendir(gNfsConnection.GetNfsContext(), strDirName.c_str(), &nfsdir);

  if(ret != 0)
  {
    CLog::Log(LOGERROR, "Failed to open(%s) %s\n", strDirName.c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  lock.Leave();
  
  while((nfsdirent = gNfsConnection.GetImpl()->nfs_readdir(gNfsConnection.GetNfsContext(), nfsdir)) != NULL) 
  {
    CStdString strName = nfsdirent->name;
    int64_t iSize = nfsdirent->size;
    bool bIsDir = nfsdirent->type == NF3DIR;
    int64_t lTimeDate = nfsdirent->mtime.tv_sec;
    
    if (!strName.Equals(".") && !strName.Equals("..")
      && !strName.Equals("lost+found"))
    {
      if(lTimeDate == 0) // if modification date is missing, use create date
      {
        lTimeDate = nfsdirent->ctime.tv_sec;
      }

      LONGLONG ll = Int32x32To64(lTimeDate & 0xffffffff, 10000000) + 116444736000000000ll;
      fileTime.dwLowDateTime = (DWORD) (ll & 0xffffffff);
      fileTime.dwHighDateTime = (DWORD)(ll >> 32);
      FileTimeToLocalFileTime(&fileTime, &localTime);

      CFileItemPtr pItem(new CFileItem(strName));
      CStdString path(strPath + strName);
      pItem->m_dateTime=localTime;   

      if (bIsDir)
      {
        URIUtils::AddSlashAtEnd(path);
        pItem->m_bIsFolder = true;
        pItem->m_dwSize = iSize;        
      }
      else
      {
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = iSize;
      }

      if (strName[0] == '.')
      {
        pItem->SetProperty("file:hidden", true);
      }
      pItem->SetPath(path);
      items.Add(pItem);
    }
  }

  lock.Enter();
  gNfsConnection.GetImpl()->nfs_closedir(gNfsConnection.GetNfsContext(), nfsdir);//close the dir
  lock.Leave();
  return true;
}

bool CNFSDirectory::Create(const char* strPath)
{
  int ret = 0;
  bool success=true;
  
  CSingleLock lock(gNfsConnection);
  CStdString folderName(strPath);
  URIUtils::RemoveSlashAtEnd(folderName);//mkdir fails if a slash is at the end!!! 
  CURL url(folderName); 
  folderName = "";
  
  if(!gNfsConnection.Connect(url,folderName))
    return false;
  
  ret = gNfsConnection.GetImpl()->nfs_mkdir(gNfsConnection.GetNfsContext(), folderName.c_str());

  success = (ret == 0 || -EEXIST == ret);
  if(!success)
    CLog::Log(LOGERROR, "NFS: Failed to create(%s) %s\n", folderName.c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
  return success;
}

bool CNFSDirectory::Remove(const char* strPath)
{
  int ret = 0;

  CSingleLock lock(gNfsConnection);
  CStdString folderName(strPath);
  URIUtils::RemoveSlashAtEnd(folderName);//rmdir fails if a slash is at the end!!!   
  CURL url(folderName);
  folderName = "";
  
  if(!gNfsConnection.Connect(url,folderName))
    return false;
  
  ret = gNfsConnection.GetImpl()->nfs_rmdir(gNfsConnection.GetNfsContext(), folderName.c_str());

  if(ret != 0 && errno != ENOENT)
  {
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  return true;
}

bool CNFSDirectory::Exists(const char* strPath)
{
  int ret = 0;

  CSingleLock lock(gNfsConnection); 
  CStdString folderName(strPath);  
  URIUtils::RemoveSlashAtEnd(folderName);//remove slash at end or URIUtils::GetFileName won't return what we want...
  CURL url(folderName);
  folderName = "";
  
  if(!gNfsConnection.Connect(url,folderName))
    return false;
  
  struct stat info;
  ret = gNfsConnection.GetImpl()->nfs_stat(gNfsConnection.GetNfsContext(), folderName.c_str(), &info);
  
  if (ret != 0)
  {
    return false;
  }
  return (info.st_mode & S_IFDIR) ? true : false;
}

#endif
