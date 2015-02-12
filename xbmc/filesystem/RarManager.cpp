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

#include "system.h"
#include "RarManager.h"
#include "Util.h"
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#include "threads/SingleLock.h"
#include "Directory.h"
#include "SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "URL.h"

#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIWindowManager.h"
#include "utils/StringUtils.h"

#include <set>

#define EXTRACTION_WARN_SIZE 50*1024*1024

using namespace std;
using namespace XFILE;

CFileInfo::CFileInfo()
{
  m_strCachedPath.clear();
  m_bAutoDel = true;
  m_iUsed = 0;
  m_iIsSeekable = -1;
  m_iOffset = 0;
}

CFileInfo::~CFileInfo()
{
}

/////////////////////////////////////////////////
CRarManager::CRarManager()
{
}

CRarManager::~CRarManager()
{
  ClearCache(true);
}

class progress_info
{
public:
  progress_info(const std::string &file) : heading(file), shown(false), showTime(200) // 200ms to show...
  {
  }
  ~progress_info()
  {
    if (shown)
    {
      // close progress dialog
      CGUIDialogProgress* dlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (dlg)
        dlg->Close();
    }
  }
  /*! \brief Progress callback from rar manager.
   \return true to continue processing, false to cancel.
   */
  bool progress(int progress, const char *text)
  {
    bool cont(true);
    if (shown || showTime.IsTimePast())
    {
      // grab the busy and show it
      CGUIDialogProgress* dlg = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (dlg)
      {
        if (!shown)
        {
          dlg->SetHeading(heading);
          dlg->StartModal();
        }
        if (progress >= 0)
        {
          dlg->ShowProgressBar(true);
          dlg->SetPercentage(progress);
        }
        if (text)
          dlg->SetLine(1, text);
        cont = !dlg->IsCanceled();
        shown = true;
        // tell render loop to spin
        dlg->Progress();
      }
    }
    return cont;
  };
private:
  std::string          heading;
  bool                 shown;
  XbmcThreads::EndTime showTime;
};

/*! \brief Rar progress callback.
  \return false to halt progress, true to continue
 */
bool ProgressCallback(void *context, int progress, const char *text)
{
  progress_info* info = (progress_info*)context;
  if (info)
    return info->progress(progress, text);
  return true;
}

bool CRarManager::CacheRarredFile(std::string& strPathInCache, const std::string& strRarPath, const std::string& strPathInRar, BYTE  bOptions, const std::string& strDir, const int64_t iSize)
{
#ifdef HAS_FILESYSTEM_RAR
  CSingleLock lock(m_CritSection);

  //If file is listed in the cache, then use listed copy or cleanup before overwriting.
  bool bOverwrite = (bOptions & EXFILE_OVERWRITE) != 0;
  map<std::string, pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find( strRarPath );
  CFileInfo* pFile=NULL;
  if( j != m_ExFiles.end() )
  {
    pFile = GetFileInRar(strRarPath,strPathInRar);
    if (pFile)
    {
      if (pFile->m_bIsCanceled())
        return false;

      if( CFile::Exists( pFile->m_strCachedPath) )
      {
        if( !bOverwrite )
        {
          strPathInCache = pFile->m_strCachedPath;
          pFile->m_iUsed++;
          return true;
        }

        CFile::Delete(pFile->m_strCachedPath);
        pFile->m_iUsed++;
      }
    }
  }

  int iRes = 0;
  if (iSize > EXTRACTION_WARN_SIZE)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (pDialog)
    {
      pDialog->SetHeading(120);
      pDialog->SetLine(0, 645);
      pDialog->SetLine(1, URIUtils::GetFileName(strPathInRar));
      pDialog->SetLine(2, "");
      pDialog->DoModal();
      if (!pDialog->IsConfirmed())
        iRes = 2; // pretend to be canceled
    }
  }
  if (CheckFreeSpace(strDir) < iSize && iRes != 2)
  {
    ClearCache();
    if (CheckFreeSpace(strDir) < iSize)
    {
      CFileItemList items;
      CDirectory::GetDirectory(g_advancedSettings.m_cachePath,items);
      items.Sort(SortBySize, SortOrderDescending);
      while (items.Size() && CheckFreeSpace(strDir) < iSize)
      {
        if (!items[0]->m_bIsFolder)
          if (!CFile::Delete(items[0]->GetPath()))
            break;

        items.Remove(0);
      }
      if (!items.Size())
        return false;
    }
  }

  std::string strPath = strPathInRar;
#ifndef TARGET_POSIX
  StringUtils::Replace(strPath, '/', '\\');
#endif
  //g_charsetConverter.unknownToUTF8(strPath);
  std::string strCachedPath = URIUtils::AddFileToFolder(strDir + "rarfolder%04d",
                                           URIUtils::GetFileName(strPathInRar));
  strCachedPath = CUtil::GetNextPathname(strCachedPath, 9999);
  if (strCachedPath.empty())
  {
    CLog::Log(LOGWARNING, "Could not cache file %s", (strRarPath + strPathInRar).c_str());
    return false;
  }
  strCachedPath = CUtil::MakeLegalPath(strCachedPath);
  int64_t iOffset = -1;
  if (iRes != 2)
  {
    if (pFile)
    {
      if (pFile->m_iOffset != -1)
        iOffset = pFile->m_iOffset;
    }


    if (iOffset == -1 && j != m_ExFiles.end())  // grab from list
    {
      for( ArchiveList_struct* pIterator = j->second.first; pIterator; pIterator = pIterator->next)
      {
        std::string strName;

        /* convert to utf8 */
        if( pIterator->item.NameW && wcslen(pIterator->item.NameW) > 0)
          g_charsetConverter.wToUTF8(pIterator->item.NameW, strName);
        else
          g_charsetConverter.unknownToUTF8(pIterator->item.Name, strName);
        if (strName == strPath)
        {
          iOffset = pIterator->item.iOffset;
          break;
        }
      }
    }
    bool bShowProgress=false;
    if (iSize > 1024*1024 || iSize == -2) // 1MB
      bShowProgress=true;

    std::string strDir2 = URIUtils::GetDirectory(strCachedPath);
    URIUtils::RemoveSlashAtEnd(strDir2);
    if (!CDirectory::Exists(strDir2))
      CDirectory::Create(strDir2);
    progress_info info(CURL(strPath).GetWithoutUserDetails());
    iRes = urarlib_get(const_cast<char*>(strRarPath.c_str()), const_cast<char*>(strDir2.c_str()),
                       const_cast<char*>(strPath.c_str()),NULL,&iOffset,bShowProgress ? ProgressCallback : NULL, &info);
  }
  if (iRes == 0)
  {
    CLog::Log(LOGERROR,"failed to extract file: %s",strPathInRar.c_str());
    return false;
  }

  if(!pFile)
  {
    CFileInfo fileInfo;
    fileInfo.m_strPathInRar = strPathInRar;
    if (j == m_ExFiles.end())
    {
      ArchiveList_struct* pArchiveList;
      if(ListArchive(strRarPath,pArchiveList))
      {
        m_ExFiles.insert(make_pair(strRarPath,make_pair(pArchiveList,vector<CFileInfo>())));
        j = m_ExFiles.find(strRarPath);
      }
      else
        return false;
    }
    j->second.second.push_back(fileInfo);
    pFile = &(j->second.second[j->second.second.size()-1]);
    pFile->m_iUsed = 1;
  }
  pFile->m_strCachedPath = strCachedPath;
  pFile->m_bAutoDel = (bOptions & EXFILE_AUTODELETE) != 0;
  pFile->m_iOffset = iOffset;
  strPathInCache = pFile->m_strCachedPath;

  if (iRes == 2) //canceled
  {
    pFile->watch.StartZero();
    CFile::Delete(pFile->m_strCachedPath);
    return false;
  }
#endif
  return true;
}

// NB: The rar manager expects paths in rars to be terminated with a "\".
bool CRarManager::GetFilesInRar(CFileItemList& vecpItems, const std::string& strRarPath,
                                bool bMask, const std::string& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  CSingleLock lock(m_CritSection);

  ArchiveList_struct* pFileList = NULL;
  map<std::string,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator it = m_ExFiles.find(strRarPath);
  if (it == m_ExFiles.end())
  {
    if( urarlib_list((char*) strRarPath.c_str(), &pFileList, NULL) )
      m_ExFiles.insert(make_pair(strRarPath,make_pair(pFileList,vector<CFileInfo>())));
    else
    {
      if( pFileList ) urarlib_freelist(pFileList);
      return false;
    }
  }
  else
    pFileList = it->second.first;

  CFileItemPtr pFileItem;
  vector<std::string> vec;
  set<std::string> dirSet;
  StringUtils::Tokenize(strPathInRar,vec,"/");
  unsigned int iDepth = vec.size();

  ArchiveList_struct* pIterator;
  std::string strCompare = strPathInRar;
  if (!URIUtils::HasSlashAtEnd(strCompare) && !strCompare.empty())
    strCompare += '/';
  for( pIterator = pFileList; pIterator  ; pIterator = pIterator->next )
  {
    std::string strName;

    /* convert to utf8 */
    if( pIterator->item.NameW && wcslen(pIterator->item.NameW) > 0)
      g_charsetConverter.wToUTF8(pIterator->item.NameW, strName);
    else
      g_charsetConverter.unknownToUTF8(pIterator->item.Name, strName);

    /* replace back slashes into forward slashes */
    /* this could get us into troubles, file could two different files, one with / and one with \ */
    StringUtils::Replace(strName, '\\', '/');

    if (bMask)
    {
      if (!strstr(strName.c_str(),strCompare.c_str()))
        continue;

      vec.clear();
      StringUtils::Tokenize(strName,vec,"/");
      if (vec.size() < iDepth)
        continue;
    }

    unsigned int iMask = (pIterator->item.HostOS==3 ? 0x0040000:16); // win32 or unix attribs?
    if (((pIterator->item.FileAttr & iMask) == iMask) || (vec.size() > iDepth+1 && bMask)) // we have a directory
    {
      if (!bMask) continue;
      if (vec.size() == iDepth)
        continue; // remove root of listing

      if (dirSet.find(vec[iDepth]) == dirSet.end())
      {
        dirSet.insert(vec[iDepth]);
        pFileItem.reset(new CFileItem(vec[iDepth]));
        pFileItem->SetPath(vec[iDepth] + '/');
        pFileItem->m_bIsFolder = true;
        pFileItem->m_idepth = pIterator->item.Method;
        pFileItem->m_iDriveType = pIterator->item.HostOS;
        //pFileItem->m_lEndOffset = long(pIterator->item.iOffset);
      }
    }
    else
    {
      if (vec.size() == iDepth+1 || !bMask)
      {
        if (vec.size() == 0)
          pFileItem.reset(new CFileItem(strName));
        else
          pFileItem.reset(new CFileItem(vec[iDepth]));
        pFileItem->SetPath(strName.c_str()+strPathInRar.size());
        pFileItem->m_dwSize = pIterator->item.UnpSize;
        pFileItem->m_idepth = pIterator->item.Method;
        pFileItem->m_iDriveType = pIterator->item.HostOS;
        //pFileItem->m_lEndOffset = long(pIterator->item.iOffset);
      }
    }
    if (pFileItem)
      vecpItems.Add(pFileItem);

    pFileItem.reset();
  }
  return vecpItems.Size() > 0;
#else
  return false;
#endif
}

bool CRarManager::ListArchive(const std::string& strRarPath, ArchiveList_struct* &pArchiveList)
{
#ifdef HAS_FILESYSTEM_RAR
 return urarlib_list((char*) strRarPath.c_str(), &pArchiveList, NULL) == 1;
#else
 return false;
#endif
}

CFileInfo* CRarManager::GetFileInRar(const std::string& strRarPath, const std::string& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  map<std::string,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return NULL;

  for (vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return &(*it2);
#endif
  return NULL;
}

bool CRarManager::GetPathInCache(std::string& strPathInCache, const std::string& strRarPath, const std::string& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  map<std::string,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return false;

  for (vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return CFile::Exists(it2->m_strCachedPath);
#endif
  return false;
}

bool CRarManager::IsFileInRar(bool& bResult, const std::string& strRarPath, const std::string& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  bResult = false;
  CFileItemList ItemList;

  if (!GetFilesInRar(ItemList,strRarPath,false))
    return false;

  int it;
  for (it=0;it<ItemList.Size();++it)
  {
    if (strPathInRar.compare(ItemList[it]->GetPath()) == 0)
      break;
  }
  if (it != ItemList.Size())
    bResult = true;

  return true;
#else
  return false;
#endif
}

void CRarManager::ClearCache(bool force)
{
#ifdef HAS_FILESYSTEM_RAR
  CSingleLock lock(m_CritSection);
  map<std::string, pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j;
  for (j = m_ExFiles.begin() ; j != m_ExFiles.end() ; ++j)
  {

    for (vector<CFileInfo>::iterator it2 = j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    {
      CFileInfo* pFile = &(*it2);
      if (pFile->m_bAutoDel && (pFile->m_iUsed < 1 || force))
        CFile::Delete( pFile->m_strCachedPath );
    }
    urarlib_freelist(j->second.first);
  }

  m_ExFiles.clear();
#endif
}

void CRarManager::ClearCachedFile(const std::string& strRarPath, const std::string& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  CSingleLock lock(m_CritSection);

  map<std::string,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
  {
    return; // no such subpath
  }

  for (vector<CFileInfo>::iterator it = j->second.second.begin(); it != j->second.second.end(); ++it)
  {
    if (it->m_strPathInRar == strPathInRar)
      if (it->m_iUsed > 0)
      {
        it->m_iUsed--;
        break;
      }
  }
#endif
}

void CRarManager::ExtractArchive(const std::string& strArchive, const std::string& strPath)
{
#ifdef HAS_FILESYSTEM_RAR
  std::string strPath2(strPath);
  URIUtils::RemoveSlashAtEnd(strPath2);
  if (!urarlib_get(const_cast<char*>(strArchive.c_str()), const_cast<char*>(strPath2.c_str()),NULL))
  {
    CLog::Log(LOGERROR,"rarmanager::extractarchive error while extracting %s", strArchive.c_str());
    return;
  }
#endif
}

int64_t CRarManager::CheckFreeSpace(const std::string& strDrive)
{
  ULARGE_INTEGER lTotalFreeBytes;
  if (GetDiskFreeSpaceEx(CSpecialProtocol::TranslatePath(strDrive).c_str(), NULL, NULL, &lTotalFreeBytes))
    return lTotalFreeBytes.QuadPart;

  return 0;
}
