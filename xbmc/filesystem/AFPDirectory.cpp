/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "system.h"

#if defined(HAS_FILESYSTEM_AFP)
#include "AFPDirectory.h"
#include "AFPFile.h"
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
#include "DllLibAfp.h"

struct CachedDirEntry
{
  unsigned int type;
  CStdString name;
};

using namespace XFILE;
using namespace std;

CAFPDirectory::CAFPDirectory(void)
{
  gAfpConnection.AddActiveConnection();
}

CAFPDirectory::~CAFPDirectory(void)
{
  gAfpConnection.AddIdleConnection();
}

bool CAFPDirectory::ResolveSymlink( const CStdString &dirName, const CStdString &fileName, 
                                    struct stat *stat, CURL &resolvedUrl)
{
  CSingleLock lock(gAfpConnection); 
  int ret = 0;  
  bool retVal = true;
  char resolvedLink[MAX_PATH];
  CStdString fullpath = dirName;
  URIUtils::AddSlashAtEnd(fullpath);
  fullpath += fileName;
  
  CPasswordManager::GetInstance().AuthenticateURL(resolvedUrl);
  resolvedUrl.SetProtocol("afp");
  resolvedUrl.SetHostName(gAfpConnection.GetConnectedIp());   
  
  ret = gAfpConnection.GetImpl()->afp_wrap_readlink(gAfpConnection.GetVolume(), fullpath.c_str(), resolvedLink, MAX_PATH);    
  
  if(ret == 0)
  {
    fullpath = dirName;
    URIUtils::AddSlashAtEnd(fullpath);
    fullpath.append(resolvedLink);
 
    if(resolvedLink[0] == '/')
    {
      //use the special stat function for using an extra context
      //because we are inside of a dir traversation
      //and just can't change the global nfs context here
      //without destroying something...    
      fullpath = resolvedLink;
      fullpath = fullpath.Right(fullpath.length()-1);
      resolvedUrl.SetFileName(fullpath);     
      ret = gAfpConnection.stat(resolvedUrl, stat);
      if(ret < 0)
      {
        URIUtils::AddSlashAtEnd(fullpath);
        resolvedUrl.SetFileName(fullpath);     
        ret = gAfpConnection.stat(resolvedUrl, stat);
      }
    }
    else
    {
      ret = gAfpConnection.GetImpl()->afp_wrap_getattr(gAfpConnection.GetVolume(), fullpath.c_str(), stat);
      resolvedUrl.SetFileName(gAfpConnection.GetUrl()->volumename + fullpath);            
    }

    if (ret != 0) 
    {
      CLog::Log(LOGERROR, "AFP: Failed to stat(%s) on link resolve %s\n", fullpath.c_str(), strerror(errno));
      retVal = false;;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "Failed to readlink(%s) %s\n", fullpath.c_str(), strerror(errno));
    retVal = false;
  }
  return retVal;
}


bool CAFPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // We accept afp://[[user[:password@]]server[/share[/path[/file]]]]
  // silence gdb breaking on signal SIGUSR2 with "handle SIGUSR2 nostop noprint"
  bool bListVolumes = false;
  FILETIME fileTime, localTime;

  CSingleLock lock(gAfpConnection);
  // we need an url to do proper escaping
  CURL url(strPath);
  CAfpConnection::afpConnnectError afpError = gAfpConnection.Connect(url);

  if (afpError != CAfpConnection::AfpOk || (!url.GetShareName().IsEmpty() && !gAfpConnection.GetVolume()))
  {
    if (afpError == CAfpConnection::AfpAuth)
    {
       if (m_flags & DIR_FLAG_ALLOW_PROMPT)
       {
         RequireAuthentication(url.Get());
       }
    }
    return false;
  }
  CStdString strDirName = gAfpConnection.GetPath(url);

  vector<CachedDirEntry> vecEntries;
  struct afp_file_info *dirEnt = NULL;
  struct afp_file_info *curDirPtr = NULL;

  // if no share name in url - try to fetch the volumes on the server and treat them like folders
  if (url.GetShareName().IsEmpty())
  {
    bListVolumes = true;
    struct afp_server *serv = gAfpConnection.GetServer();
    for (int i = 0; i < serv->num_volumes; i++)
    {
      CachedDirEntry aDir;
      aDir.type = 1;
      aDir.name = serv->volumes[i].volume_name;
      vecEntries.push_back(aDir);
    }
  }

  // if we not only list volumes - read the dir
  if (!bListVolumes)
  {
    if (gAfpConnection.GetImpl()->afp_wrap_readdir(gAfpConnection.GetVolume(), strDirName.c_str(), &dirEnt))
      return false;
    lock.Leave();

    for (curDirPtr = dirEnt; curDirPtr; curDirPtr = curDirPtr->next)
    {
      CachedDirEntry aDir;
      aDir.type = curDirPtr->isdir;
#ifdef USE_CVS_AFPFS
      aDir.name = curDirPtr->basic.name;
#else
      aDir.name = curDirPtr->name;
#endif
      vecEntries.push_back(aDir);
    }
    gAfpConnection.GetImpl()->afp_ml_filebase_free(&dirEnt);
  }

  for (size_t i = 0; i < vecEntries.size(); i++)
  {
    CachedDirEntry aDir = vecEntries[i];
    // We use UTF-8 internally, as does AFP
    CStdString strFile = aDir.name;
    CStdString myStrPath(strPath);
    URIUtils::AddSlashAtEnd(myStrPath); //be sure the dir ends with a slash    
    CStdString path(myStrPath + strFile);

    if (!strFile.Equals(".") && !strFile.Equals("..") && !strFile.Equals("lost+found"))
    {
      int64_t iSize = 0;
      bool bIsDir = aDir.type;
      int64_t lTimeDate = 0;

      // if we not only list volumes - stat the files in folder
      if (!bListVolumes)
      {
        struct stat info = {0};

        if ((m_flags & DIR_FLAG_NO_FILE_INFO)==0 && g_advancedSettings.m_sambastatfiles)
        {
          // make sure we use the authenticated path wich contains any default username
          CStdString strFullName = strDirName + strFile;

          lock.Enter();

          if (gAfpConnection.GetImpl()->afp_wrap_getattr(gAfpConnection.GetVolume(), strFullName.c_str(), &info) == 0)
          {                       
            //resolve symlinks
            if(S_ISLNK(info.st_mode))
            {
              CURL linkUrl(url);
              if(!ResolveSymlink(strDirName, strFile, &info, linkUrl))
              {
                lock.Leave();              
                continue;
              }
              path = linkUrl.Get();
              bIsDir = info.st_mode & S_IFDIR;            
            }
            lTimeDate = info.st_mtime;
            if (lTimeDate == 0) // if modification date is missing, use create date
              lTimeDate = info.st_ctime;
            iSize = info.st_size;
          }
          else
          {
            CLog::Log(LOGERROR, "%s - Failed to stat file %s (%s)", __FUNCTION__, strFullName.c_str(),strerror(errno));
          }

          lock.Leave();
        }
        LONGLONG ll = Int32x32To64(lTimeDate & 0xffffffff, 10000000) + 116444736000000000ll;
        fileTime.dwLowDateTime  = (DWORD)(ll & 0xffffffff);
        fileTime.dwHighDateTime = (DWORD)(ll >> 32);
        FileTimeToLocalFileTime(&fileTime, &localTime);
      }
      else
      {
        bIsDir = true;
        localTime.dwHighDateTime = 0;
        localTime.dwLowDateTime = 0;
      }
      
      CFileItemPtr pItem(new CFileItem(strFile));      
      pItem->m_dateTime  = localTime;    
      pItem->m_dwSize    = iSize;
      
      if (bIsDir)
      {
        URIUtils::AddSlashAtEnd(path);
        pItem->m_bIsFolder = true;
      }
      else
      {
        pItem->m_bIsFolder = false;
      }
 
      if (!aDir.name.empty() && aDir.name[0] == '.')
      {
        pItem->SetProperty("file:hidden", true);
      }

      pItem->SetPath(path);      
      items.Add(pItem);      
    }
  }

  return true;
}

bool CAFPDirectory::Create(const char* strPath)
{
  CSingleLock lock(gAfpConnection);

  CURL url(strPath);

  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return false;

  CStdString strFilename = gAfpConnection.GetPath(url);

  int result = gAfpConnection.GetImpl()->afp_wrap_mkdir(gAfpConnection.GetVolume(), strFilename.c_str(), 0);

  if (result != 0)
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0 || EEXIST == result);
}

bool CAFPDirectory::Remove(const char *strPath)
{
  CSingleLock lock(gAfpConnection);

  CURL url(strPath);
  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return false;

  CStdString strFileName = gAfpConnection.GetPath(url);

  int result = gAfpConnection.GetImpl()->afp_wrap_rmdir(gAfpConnection.GetVolume(), strFileName.c_str());

  if (result != 0 && errno != ENOENT)
  {
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
    return false;
  }

  return true;
}

bool CAFPDirectory::Exists(const char *strPath)
{
  CSingleLock lock(gAfpConnection);

  CURL url(strPath);
  if (gAfpConnection.Connect(url) != CAfpConnection::AfpOk || !gAfpConnection.GetVolume())
    return false;

  CStdString strFileName(gAfpConnection.GetPath(url));

  struct stat info;
  if (gAfpConnection.GetImpl()->afp_wrap_getattr(gAfpConnection.GetVolume(), strFileName.c_str(), &info) != 0)
    return false;

  return (info.st_mode & S_IFDIR) ? true : false;
}
#endif
