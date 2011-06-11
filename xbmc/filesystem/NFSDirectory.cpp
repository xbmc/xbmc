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
#include "Util.h"
#include "guilib/LocalizeStrings.h"
#include "Application.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"

using namespace XFILE;
using namespace std;

CNFSDirectory::CNFSDirectory(void)
{
  gNfsConnection.AddActiveConnection();
}

CNFSDirectory::~CNFSDirectory(void)
{
  gNfsConnection.AddIdleConnection();
}

bool CNFSDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // We accept nfs://server/share/path[/file]]]]
  int ret = 0;
  FILETIME fileTime, localTime;    
  CSingleLock lock(gNfsConnection);
  
  CURL url(strPath);
  
  if(!gNfsConnection.Connect(url))
  {
    return false;
  }
  
  CStdString strDirName="//";//relative to the strPath we connected - we want to get the "/" directory then
    
  vector<CStdString> vecEntries;
  struct nfsdir *nfsdir = NULL;
  struct nfsdirent *nfsdirent = NULL;

  ret = gNfsConnection.GetImpl()->nfs_opendir_sync(gNfsConnection.GetNfsContext(), strDirName.c_str(), &nfsdir);
	
  if(ret != 0)
  {
    CLog::Log(LOGERROR, "Failed to open(%s) %s\n", strDirName.c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
    return false;
  }
  lock.Leave();
  
  while((nfsdirent = gNfsConnection.GetImpl()->nfs_readdir(gNfsConnection.GetNfsContext(), nfsdir)) != NULL) 
  {
    vecEntries.push_back(nfsdirent->name);
  }
  
  lock.Enter();
  gNfsConnection.GetImpl()->nfs_closedir(gNfsConnection.GetNfsContext(), nfsdir);//close the dir
  lock.Leave();
      
  for (size_t i=0; i<vecEntries.size(); i++)
  {
    CStdString strName = vecEntries[i];
   
    if (!strName.Equals(".") && !strName.Equals("..")
      && !strName.Equals("lost+found"))
    {
      int64_t iSize = 0;
      bool bIsDir = false;
      int64_t lTimeDate = 0;
      struct stat info = {0};

      CStdString strFullName = strDirName + strName;          

      lock.Enter();
      ret = gNfsConnection.GetImpl()->nfs_stat_sync(gNfsConnection.GetNfsContext(), strFullName.c_str(), &info);
      lock.Leave();
      
      if( ret == 0 )
      {
        bIsDir = (info.st_mode & S_IFDIR) ? true : false;
        lTimeDate = info.st_mtime;
        if(lTimeDate == 0) // if modification date is missing, use create date
          lTimeDate = info.st_ctime;
        iSize = info.st_size;
      }
      else
        CLog::Log(LOGERROR, "NFS; Failed to stat(%s) %s\n", strFullName.c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));

      LONGLONG ll = Int32x32To64(lTimeDate & 0xffffffff, 10000000) + 116444736000000000ll;
      fileTime.dwLowDateTime = (DWORD) (ll & 0xffffffff);
      fileTime.dwHighDateTime = (DWORD)(ll >> 32);
      FileTimeToLocalFileTime(&fileTime, &localTime);

      CFileItemPtr pItem(new CFileItem(strName));
      pItem->m_strPath = strPath + strName;
      pItem->m_dateTime=localTime;      

      if (bIsDir)
      {
        URIUtils::AddSlashAtEnd(pItem->m_strPath);
        pItem->m_bIsFolder = true;
      }
      else
      {
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = iSize;
      }
      items.Add(pItem);
    }
  }
  return true;
}

bool CNFSDirectory::Create(const char* strPath)
{
  int ret = 0;
  int newFolderLen = 0;
  
  CSingleLock lock(gNfsConnection);

  CURL url(URIUtils::GetParentPath(strPath)); 
  CStdString folderName(strPath);
  newFolderLen = folderName.length() - URIUtils::GetParentPath(strPath).length();
  folderName = "//" + folderName.Right(newFolderLen);
  
  URIUtils::RemoveSlashAtEnd(folderName);//mkdir fails if a slash is at the end!!!
  
  if(!gNfsConnection.Connect(url))
    return false;
  
  ret = gNfsConnection.GetImpl()->nfs_mkdir_sync(gNfsConnection.GetNfsContext(), folderName.c_str());

  if(ret != 0)
    CLog::Log(LOGERROR, "NFS: Failed to create(%s) %s\n", folderName.c_str(), gNfsConnection.GetImpl()->nfs_get_error(gNfsConnection.GetNfsContext()));
  return (ret == 0 || EEXIST == ret);
}

bool CNFSDirectory::Remove(const char* strPath)
{
  int ret = 0;
  int delFolderLen = 0;

  CSingleLock lock(gNfsConnection);
  
  CURL url(URIUtils::GetParentPath(strPath));
  CStdString folderName(strPath);
  delFolderLen = folderName.length() - URIUtils::GetParentPath(strPath).length();
  folderName = "//" + folderName.Right(delFolderLen);
  
  URIUtils::RemoveSlashAtEnd(folderName);//rmdir fails if a slash is at the end!!!  
  
  
  if(!gNfsConnection.Connect(url))
    return false;
  
  ret = gNfsConnection.GetImpl()->nfs_rmdir_sync(gNfsConnection.GetNfsContext(), folderName.c_str());

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
  int existFolderLen = 0;
  CSingleLock lock(gNfsConnection);
  
  CURL url(URIUtils::GetParentPath(strPath));
  CStdString folderName(strPath);
  existFolderLen = folderName.length() - URIUtils::GetParentPath(strPath).length();
  folderName = "//" + folderName.Right(existFolderLen);
  
  if(!gNfsConnection.Connect(url))
    return false;
  
  struct stat info;
  ret = gNfsConnection.GetImpl()->nfs_stat_sync(gNfsConnection.GetNfsContext(), folderName.c_str(), &info);
  
  if (ret != 0)
  {
    return false;
  }
  return (info.st_mode & S_IFDIR) ? true : false;
}

#endif
