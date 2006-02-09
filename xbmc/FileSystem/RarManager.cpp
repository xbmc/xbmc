#include "../stdafx.h"
#include "RarManager.h"
#include "../lib/UnrarXLib/rar.hpp"
#include "../util.h"
#include "../utils/singlelock.h"

#define EXTRACTION_WARN_SIZE 50*1024*1024

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
}
CRarManager::~CRarManager()
{
	ClearCache(true);
}

bool CRarManager::CacheRarredFile(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar, BYTE  bOptions, const CStdString& strDir, __int64 iSize)
{
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
  
  //Extract archived file, using existing local copy or overwriting if wanted...
  if (iSize > EXTRACTION_WARN_SIZE)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (pDialog)
    {
      pDialog->SetHeading(120);
      pDialog->SetLine(0, 645);
      pDialog->SetLine(1, CUtil::GetFileName(strPathInRar));
      pDialog->SetLine(2, L"");
      pDialog->DoModal(m_gWindowManager.GetActiveWindow());
      if (!pDialog->IsConfirmed())
        return false;
    }
  }

  if (CheckFreeSpace(strDir.Left(3)) < iSize)
  {
    ClearCache();
    if (CheckFreeSpace(strDir.Left(3)) < iSize)
    {
      CLog::Log(LOGERROR,"rarmanager::cache out of disk space!");
      return false;
    }
  }
  
  int iRes = urarlib_get(const_cast<char*>(strRarPath.c_str()), const_cast<char*>(strDir.c_str()),const_cast<char*>(strPathInRar.c_str()),"");
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
    pFile = &(j->second.second.back());
    pFile->m_iUsed = 1;
  }
  if (CUtil::HasSlashAtEnd(strDir))
    pFile->m_strCachedPath = strDir+CUtil::GetFileName(strPathInRar);
  else
    pFile->m_strCachedPath = strDir+"\\"+CUtil::GetFileName(strPathInRar);
  CUtil::GetFatXQualifiedPath(pFile->m_strCachedPath);
  pFile->m_bAutoDel = (bOptions & EXFILE_AUTODELETE) != 0;
	strPathInCache = pFile->m_strCachedPath;

  if (iRes == 2) //canceled
  {
    pFile->watch.StartZero();
    CFile::Delete(pFile->m_strCachedPath);
    return false;
  }

	return true;
}

// NB: The rar manager expects paths in rars to be terminated with a "\".
bool CRarManager::GetFilesInRar(CFileItemList& vecpItems, const CStdString& strRarPath, bool bMask, const CStdString& strPathInRar)
{
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
  CUtil::Tokenize(strPathInRar,vec,"\\/");
  unsigned int iDepth = vec.size();
  
  ArchiveList_struct* pIterator;
  CStdString strMatch;
  for( pIterator = pFileList; pIterator  ; pIterator ? pIterator = pIterator->next : NULL)
	{
    CStdString strDirDelimiter = (pIterator->item.HostOS==3 ? "/":"\\"); // win32 or unix paths?
    if (bMask)
    {
      vec.clear();
      CUtil::Tokenize(pIterator->item.Name,vec,strDirDelimiter);
      if (!strstr(pIterator->item.Name,strPathInRar.c_str()))
        continue;
      if (vec.size() < iDepth)
        continue;
    }
    int iMask = (pIterator->item.HostOS==3 ? 0x0040000:16); // win32 or unix attribs?
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
        pFileItem->m_strPath += '\\';
        pFileItem->m_strPath.Replace("/","\\");
        pFileItem->m_bIsFolder = true;
        pFileItem->m_idepth = pIterator->item.Method;
        //pFileItem->m_lEndOffset = long(pIterator->item.iOffset);
      }
    }
    else
    {
      if (vec.size() == iDepth+1 || !bMask)
      {
        pFileItem = new CFileItem(pIterator->item.Name+strPathInRar.size());
		    pFileItem->m_strPath = pIterator->item.Name+strPathInRar.size();
        pFileItem->m_strPath.Replace("/","\\");
        pFileItem->m_dwSize = pIterator->item.UnpSize;
        pFileItem->m_idepth = pIterator->item.Method;
        //pFileItem->m_lEndOffset = long(pIterator->item.iOffset);
      }
    }
    if (pFileItem)
      vecpItems.Add(pFileItem);

    pFileItem = NULL;
	}
  return true;
}

bool CRarManager::ListArchive(const CStdString& strRarPath, ArchiveList_struct* &pArchiveList)
{
 return urarlib_list((char*) strRarPath.c_str(), &pArchiveList, NULL) == 1;
}

CFileInfo* CRarManager::GetFileInRar(const CStdString& strRarPath, const CStdString& strPathInRar)
{
  std::map<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return NULL;

  for (std::vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return &(*it2);

  return NULL;
}

bool CRarManager::GetPathInCache(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar)
{
	std::map<CStdString,std::pair<ArchiveList_struct*,std::vector<CFileInfo> > >::iterator j = m_ExFiles.find(strRarPath);
  if (j == m_ExFiles.end())
    return false;

  for (std::vector<CFileInfo>::iterator it2=j->second.second.begin(); it2 != j->second.second.end(); ++it2)
    if (it2->m_strPathInRar == strPathInRar)
      return CFile::Exists(it2->m_strCachedPath);
  
  return false;
}

bool CRarManager::IsFileInRar(bool& bResult, const CStdString& strRarPath, const CStdString& strPathInRar)
{
	bResult = false;
	CFileItemList ItemList;
	
  if (!GetFilesInRar(ItemList,strRarPath))	
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
}

void CRarManager::ClearCache(bool force)
{
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
}

void CRarManager::ClearCachedFile(const CStdString& strRarPath, const CStdString& strPathInRar)
{
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
}

void CRarManager::ExtractArchive(const CStdString& strArchive, const CStdString& strPath)
{
  if (!urarlib_get(const_cast<char*>(strArchive.c_str()), const_cast<char*>(strPath.c_str()),NULL))
  {
    CLog::Log(LOGERROR,"rarmanager::extractarchive error while extracting %s",strArchive.c_str());
    return;
  }
 }

__int64 CRarManager::CheckFreeSpace(const CStdString& strDrive)
{
  ULARGE_INTEGER lTotalFreeBytes;
  if (GetDiskFreeSpaceEx(strDrive.c_str(), NULL, NULL, &lTotalFreeBytes))
    return lTotalFreeBytes.QuadPart;

  return 0;
}