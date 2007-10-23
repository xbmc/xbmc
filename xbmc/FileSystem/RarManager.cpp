#include "stdafx.h"
#include "RarManager.h"
#ifdef HAS_RAR
#include "../lib/UnrarXLib/rar.hpp"
#endif
#include "../Util.h"
#include "../utils/SingleLock.h"
#include "../FileItem.h"

#include <set>

#define EXTRACTION_WARN_SIZE 50*1024*1024

using namespace XFILE;
using namespace DIRECTORY;

CRarManager g_RarManager;

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
  m_bWipe = true;
}

CRarManager::~CRarManager()
{
	ClearCache(true);
}

bool CRarManager::CacheRarredFile(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar, BYTE  bOptions, const CStdString& strDir, const __int64 iSize)
{
#ifdef HAS_RAR
	CSingleLock lock(m_CritSection);
    //If file is listed in the cache, then use listed copy or cleanup before overwriting.
  bool bOverwrite = (bOptions & EXFILE_OVERWRITE) != 0;
  std::map<CStdString, std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator j = m_ExFiles.find( strRarPath );
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
  //Extract archived file, using existing local copy or overwriting if wanted...
  if (iSize > EXTRACTION_WARN_SIZE)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (pDialog)
    {
      pDialog->SetHeading(120);
      pDialog->SetLine(0, 645);
      pDialog->SetLine(1, CUtil::GetFileName(strPathInRar));
      pDialog->SetLine(2, "");
      pDialog->DoModal();
      if (!pDialog->IsConfirmed())
        iRes = 2; // pretend to be canceled
    }
  }

  if (CheckFreeSpace(strDir.Left(3)) < iSize && iRes != 2)
  {
    ClearCache();
    if (CheckFreeSpace(strDir.Left(3)) < iSize)
    {
      // wipe at will - if allowed. fixes the evil file manager bug
      if (!m_bWipe)
        return false;

      CFileItemList items;
      CDirectory::GetDirectory(g_advancedSettings.m_cachePath,items);
      items.Sort(SORT_METHOD_SIZE, SORT_ORDER_DESC);
      while (items.Size() && CheckFreeSpace(strDir.Left(3)) < iSize)
      {
        CStdString strPath = items[0]->m_strPath;
        if (!items[0]->m_bIsFolder)
          if (!CFile::Delete(items[0]->m_strPath))
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

  __int64 iOffset = -1;
  if (iRes != 2)
  {
    if (pFile)
    {
      if (pFile->m_iOffset != -1)
        iOffset = pFile->m_iOffset;
    }


    if (iOffset == -1)  // grab from list
    {
      for( ArchiveList_struct* pIterator = j->second.first; pIterator  ; pIterator ? pIterator = pIterator->next : NULL)
      {
        CStdString strName;

        /* convert to utf8 */
        if( pIterator->item.NameW && wcslen(pIterator->item.NameW) > 0)
          g_charsetConverter.wToUTF8(pIterator->item.NameW, strName);
        else
          g_charsetConverter.stringCharsetToUtf8(pIterator->item.Name, strName);

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

    CStdString strDir2(strDir);
    CUtil::RemoveSlashAtEnd(strDir2);
    iRes = urarlib_get(const_cast<char*>(strRarPath.c_str()), const_cast<char*>(strDir2.c_str()),const_cast<char*>(strPath.c_str()),NULL,&iOffset,bShowProgress);
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
        m_ExFiles.insert(std::make_pair<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >(strRarPath,std::make_pair<ArchiveList_struct*,std::vector<CFileInfo> >(pArchiveList,std::vector<CFileInfo>())));
        j = m_ExFiles.find(strRarPath);
      }
      else
        return false;
    } 
    j->second.second.push_back(fileInfo);
    pFile = &(j->second.second[j->second.second.size()-1]);
    pFile->m_iUsed = 1;
  }
  CUtil::AddFileToFolder(strDir,CUtil::GetFileName(strPathInRar),pFile->m_strCachedPath); // GetFileName
  CUtil::GetFatXQualifiedPath(pFile->m_strCachedPath);
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
bool CRarManager::GetFilesInRar(CFileItemList& vecpItems, const CStdString& strRarPath, bool bMask, const CStdString& strPathInRar)
{
#ifdef HAS_RAR
  CSingleLock lock(m_CritSection);

  ArchiveList_struct* pFileList = NULL;
  std::map<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator it = m_ExFiles.find(strRarPath);
  if (it == m_ExFiles.end())
  {
    if( urarlib_list((char*) strRarPath.c_str(), &pFileList, NULL) ) 
      m_ExFiles.insert(std::make_pair<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >(strRarPath,std::make_pair<ArchiveList_struct*,std::vector<CFileInfo> >(pFileList,std::vector<CFileInfo>())));
    else
    {
      if( pFileList ) urarlib_freelist(pFileList);
      return false;
    }
  }
  else
    pFileList = it->second.first;

	CFileItem* pFileItem = NULL;
  vector<CStdString> vec;
  std::set<CStdString> dirSet;
  CUtil::Tokenize(strPathInRar,vec,"/");
  unsigned int iDepth = vec.size();
  
  ArchiveList_struct* pIterator;
  CStdString strMatch;
  CStdString strCompare = strPathInRar;
  if (!CUtil::HasSlashAtEnd(strCompare) && !strCompare.IsEmpty())
    strCompare += '/';
  for( pIterator = pFileList; pIterator  ; pIterator ? pIterator = pIterator->next : NULL)
	{
    CStdString strDirDelimiter = (pIterator->item.HostOS==3 ? "/":"\\"); // win32 or unix paths?
    CStdString strName;
    
    /* convert to utf8 */
    if( pIterator->item.NameW && wcslen(pIterator->item.NameW) > 0)
      g_charsetConverter.wToUTF8(pIterator->item.NameW, strName);
    else
      g_charsetConverter.stringCharsetToUtf8(pIterator->item.Name, strName);

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
        pFileItem = new CFileItem(vec[iDepth]);
        pFileItem->m_strPath = vec[iDepth];
        pFileItem->m_strPath += '/';
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
          pFileItem = new CFileItem(strName);
        else
          pFileItem = new CFileItem(vec[iDepth]);
        pFileItem->m_strPath = strName.c_str()+strPathInRar.size();
        pFileItem->m_dwSize = pIterator->item.UnpSize;
        pFileItem->m_idepth = pIterator->item.Method;
        pFileItem->m_iDriveType = pIterator->item.HostOS;
        //pFileItem->m_lEndOffset = long(pIterator->item.iOffset);
      }
    }
    if (pFileItem)
      vecpItems.Add(pFileItem);

    pFileItem = NULL;
	}
  return vecpItems.Size() > 0; 
#else
  return false;
#endif
}

bool CRarManager::ListArchive(const CStdString& strRarPath, ArchiveList_struct* &pArchiveList)
{
#ifdef HAS_RAR
 return urarlib_list((char*) strRarPath.c_str(), &pArchiveList, NULL) == 1;
#else
 return false;
#endif
}

CFileInfo* CRarManager::GetFileInRar(const CStdString& strRarPath, const CStdString& strPathInRar)
{
#ifdef HAS_RAR
  std::map<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return NULL;

  for (std::vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return &(*it2);
#endif
  return NULL;
}

bool CRarManager::GetPathInCache(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar)
{
#ifdef HAS_RAR
  std::map<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return false;

  for (std::vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return CFile::Exists(it2->m_strCachedPath);
#endif 
  return false;
}

bool CRarManager::IsFileInRar(bool& bResult, const CStdString& strRarPath, const CStdString& strPathInRar)
{
#ifdef HAS_RAR
	bResult = false;
	CFileItemList ItemList;
	
  if (!GetFilesInRar(ItemList,strRarPath,false))	
    return false;
	
  int it;
  for (it=0;it<ItemList.Size();++it) 
	{
		if (strPathInRar.compare(ItemList[it]->m_strPath) == 0)
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
#ifdef HAS_RAR
  CSingleLock lock(m_CritSection);
  std::map<CStdString, std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator j;
  for (j = m_ExFiles.begin() ; j != m_ExFiles.end() ; j++)
  {
    
    for (std::vector<CFileInfo>::iterator it2 = j->second.second.begin(); it2 != j->second.second.end(); ++it2)
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
#ifdef HAS_RAR
  CSingleLock lock(m_CritSection);
  
  std::map<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
  {
    return; // no such subpath
  }
  
  for (std::vector<CFileInfo>::iterator it = j->second.second.begin(); it != j->second.second.end(); ++it)
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
#ifdef HAS_RAR
  CStdString strPath2(strPath);
  CUtil::RemoveSlashAtEnd(strPath2);
  if (!urarlib_get(const_cast<char*>(_P(strArchive).c_str()), const_cast<char*>(_P(strPath2).c_str()),NULL))
  {
    CLog::Log(LOGERROR,"rarmanager::extractarchive error while extracting %s",_P(strArchive).c_str());
    return;
  }
#endif
}

__int64 CRarManager::CheckFreeSpace(const CStdString& strDrive)
{
  ULARGE_INTEGER lTotalFreeBytes;
  if (GetDiskFreeSpaceEx(strDrive.c_str(), NULL, NULL, &lTotalFreeBytes))
    return lTotalFreeBytes.QuadPart;

  return 0;
}

bool CRarManager::HasMultipleEntries(const CStdString& strPath)
{
#ifdef HAS_RAR
  return urarlib_hasmultiple(strPath.c_str());
#else
  return false;
#endif
}

