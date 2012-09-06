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

#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"

#include <set>

#define EXTRACTION_WARN_SIZE 50*1024*1024

using namespace std;
using namespace XFILE;

CFileInfo::CFileInfo()
{
  m_strCachedPath.Empty();
  m_bAutoDel = true;
  m_iUsed = 0;
  m_iIsSeekable = -1;
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

bool CRarManager::CacheRarredFile(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar, BYTE  bOptions, const CStdString& strDir, const int64_t iSize)
{
#ifdef HAS_FILESYSTEM_RAR
  CSingleLock lock(m_CritSection);

  //If file is listed in the cache, then use listed copy or cleanup before overwriting.
  bool bOverwrite = (bOptions & EXFILE_OVERWRITE) != 0;
  map<CStdString, pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find( strRarPath );
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
      items.Sort(SORT_METHOD_SIZE, SortOrderDescending);
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

  CStdString strPath = strPathInRar;
#ifndef _LINUX
  strPath.Replace('/', '\\');
#endif
  //g_charsetConverter.unknownToUTF8(strPath);
  CStdString strCachedPath;
  URIUtils::AddFileToFolder(strDir + "rarfolder%04d", URIUtils::GetFileName(strPathInRar), strCachedPath);
  strCachedPath = CUtil::GetNextPathname(strCachedPath, 9999);
  if (strCachedPath.IsEmpty())
  {
    CLog::Log(LOGWARNING, "Could not cache file %s", (strRarPath + strPathInRar).c_str());
    return false;
  }
  strCachedPath = CUtil::MakeLegalPath(strCachedPath);
  CStdString strCachedDir;
  URIUtils::GetDirectory(strCachedPath, strCachedDir);
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
      for( ArchiveList_struct* pIterator = j->second.first; pIterator  ; pIterator ? pIterator = pIterator->next : NULL)
      {
        CStdString strName;

        /* convert to utf8 */
        if( pIterator->item.NameW && wcslen(pIterator->item.NameW) > 0)
          g_charsetConverter.wToUTF8(pIterator->item.NameW, strName);
        else
          g_charsetConverter.unknownToUTF8(pIterator->item.Name, strName);
        if (strName.Equals(strPath))
        {
          iOffset = pIterator->item.iOffset;
          break;
        }
      }
    }
    bool bShowProgress=false;
    if (iSize > 1024*1024 || iSize == -2) // 1MB
      bShowProgress=true;

    CStdString strDir2(strCachedDir);
    URIUtils::RemoveSlashAtEnd(strDir2);
    if (!CDirectory::Exists(strDir2))
      CDirectory::Create(strDir2);
    iRes = urarlib_get(const_cast<char*>(strRarPath.c_str()), const_cast<char*>(strDir2.c_str()),
                       const_cast<char*>(strPath.c_str()),NULL,&iOffset,bShowProgress);
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
bool CRarManager::GetFilesInRar(CFileItemList& vecpItems, const CStdString& strRarPath,
                                bool bMask, const CStdString& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  CSingleLock lock(m_CritSection);

  ArchiveList_struct* pFileList = NULL;
  map<CStdString,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator it = m_ExFiles.find(strRarPath);
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
  vector<CStdString> vec;
  set<CStdString> dirSet;
  CUtil::Tokenize(strPathInRar,vec,"/");
  unsigned int iDepth = vec.size();

  ArchiveList_struct* pIterator;
  CStdString strCompare = strPathInRar;
  if (!URIUtils::HasSlashAtEnd(strCompare) && !strCompare.IsEmpty())
    strCompare += '/';
  for( pIterator = pFileList; pIterator  ; pIterator ? pIterator = pIterator->next : NULL)
  {
    CStdString strDirDelimiter = (pIterator->item.HostOS==3 ? "/":"\\"); // win32 or unix paths?
    CStdString strName;

    /* convert to utf8 */
    if( pIterator->item.NameW && wcslen(pIterator->item.NameW) > 0)
      g_charsetConverter.wToUTF8(pIterator->item.NameW, strName);
    else
      g_charsetConverter.unknownToUTF8(pIterator->item.Name, strName);

    /* replace back slashes into forward slashes */
    /* this could get us into troubles, file could two different files, one with / and one with \ */
    strName.Replace('\\', '/');

    if (bMask)
    {
      if (!strstr(strName.c_str(),strCompare.c_str()))
        continue;

      vec.clear();
      CUtil::Tokenize(strName,vec,"/");
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

bool CRarManager::ListArchive(const CStdString& strRarPath, ArchiveList_struct* &pArchiveList)
{
#ifdef HAS_FILESYSTEM_RAR
 return urarlib_list((char*) strRarPath.c_str(), &pArchiveList, NULL) == 1;
#else
 return false;
#endif
}

CFileInfo* CRarManager::GetFileInRar(const CStdString& strRarPath, const CStdString& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  map<CStdString,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return NULL;

  for (vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return &(*it2);
#endif
  return NULL;
}

bool CRarManager::GetPathInCache(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  map<CStdString,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return false;

  for (vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return CFile::Exists(it2->m_strCachedPath);
#endif
  return false;
}

bool CRarManager::IsFileInRar(bool& bResult, const CStdString& strRarPath, const CStdString& strPathInRar)
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
  map<CStdString, pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j;
  for (j = m_ExFiles.begin() ; j != m_ExFiles.end() ; j++)
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

void CRarManager::ClearCachedFile(const CStdString& strRarPath, const CStdString& strPathInRar)
{
#ifdef HAS_FILESYSTEM_RAR
  CSingleLock lock(m_CritSection);

  map<CStdString,pair<ArchiveList_struct*,vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
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

void CRarManager::ExtractArchive(const CStdString& strArchive, const CStdString& strPath)
{
#ifdef HAS_FILESYSTEM_RAR
  CStdString strPath2(strPath);
  URIUtils::RemoveSlashAtEnd(strPath2);
  if (!urarlib_get(const_cast<char*>(strArchive.c_str()), const_cast<char*>(strPath2.c_str()),NULL))
  {
    CLog::Log(LOGERROR,"rarmanager::extractarchive error while extracting %s", strArchive.c_str());
    return;
  }
#endif
}

int64_t CRarManager::CheckFreeSpace(const CStdString& strDrive)
{
  ULARGE_INTEGER lTotalFreeBytes;
  if (GetDiskFreeSpaceEx(CSpecialProtocol::TranslatePath(strDrive.c_str()), NULL, NULL, &lTotalFreeBytes))
    return lTotalFreeBytes.QuadPart;

  return 0;
}
