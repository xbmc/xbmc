#include "../stdafx.h"
#include "RarManager.h"
#include "../lib/UnrarXLib/UnrarX.hpp"
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
	std::map<CStdString, CFileInfo>::iterator j = m_ExFiles.find( strPathInRar );
	CFileInfo* pFile=NULL;
  if( j != m_ExFiles.end() )
	{
		pFile = &(j->second);
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
  
  if( !urarlib_get(const_cast<char*>(strRarPath.c_str()), const_cast<char*>(strDir.c_str()),const_cast<char*>(strPathInRar.c_str()), "") )
  {
    CLog::Log(LOGDEBUG,"failed to extract file: %s",strPathInRar.c_str());
    return false;
  }
	
  if(!pFile) 
  {
    CFileInfo fileInfo;
    m_ExFiles.insert(std::make_pair<CStdString,CFileInfo>(strPathInRar,fileInfo));
    pFile = &(m_ExFiles[strPathInRar]);
    pFile->m_iUsed = 1;
  }
  if (CUtil::HasSlashAtEnd(strDir))
    pFile->m_strCachedPath = strDir+CUtil::GetFileName(strPathInRar);
  else
    pFile->m_strCachedPath = strDir+"\\"+CUtil::GetFileName(strPathInRar);
  CUtil::GetFatXQualifiedPath(pFile->m_strCachedPath);
  pFile->m_bAutoDel = (bOptions & EXFILE_AUTODELETE) != 0;
	strPathInCache = pFile->m_strCachedPath;

	return true;
}

bool CRarManager::GetFilesInRar(CFileItemList& vecpItems, const CStdString& strRarPath, bool bMask, const CStdString& strPathInRar)
{
  CSingleLock lock(m_CritSection);
	ArchiveList_struct* pFileList;
  if( !urarlib_list((char*) strRarPath.c_str(), &pFileList, NULL) ) return false;

	CFileItem* pFileItem = NULL;
  vector<CStdString> vec;
  CUtil::Tokenize(strPathInRar,vec,"\\");
  unsigned int iDepth = vec.size();
  
  ArchiveList_struct* pIterator;
  for( pIterator = pFileList; pIterator  ; pIterator ? pIterator = pIterator->next : NULL)
	{
    char cDirDelimiter = (pIterator->item.HostOS==3 ? '/':'\\'); // win32 or unix paths?
    if (bMask)
    {
      vec.clear();
      CUtil::Tokenize(pIterator->item.Name,vec,"\\");
      if ((vec.size() > iDepth+1) || (vec.size() < iDepth))
        continue;
      if (!strstr(pIterator->item.Name,strPathInRar.c_str()))
        continue;
    }
    int iMask = (pIterator->item.HostOS==3 ? 0x0040000:16); // win32 or unix attribs?
    if (((pIterator->item.FileAttr & iMask) == iMask)) // we have a directory
    {
      if (!bMask) continue;
      if (vec.size() == iDepth)
        continue; // remove root of listing
      pFileItem = new CFileItem(pIterator->item.Name+strPathInRar.size());
      pFileItem->m_strPath = pIterator->item.Name+strPathInRar.size();
      pFileItem->m_strPath += '\\';
      pFileItem->m_strPath.Replace("/","\\");
      pFileItem->m_bIsFolder = true;
      pFileItem->m_lStartOffset = pIterator->item.Method;
    }
    else
    {
      pFileItem = new CFileItem(pIterator->item.Name+strPathInRar.size());
		  pFileItem->m_strPath = pIterator->item.Name+strPathInRar.size();
      pFileItem->m_strPath.Replace("/","\\");
      pFileItem->m_dwSize = pIterator->item.UnpSize;
      pFileItem->m_lStartOffset = pIterator->item.Method;
    }
    if (pFileItem)
      vecpItems.Add(pFileItem);
    pFileItem = NULL;
	}
	urarlib_freelist(pFileList);
	return true;
}
bool CRarManager::GetPathInCache(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar)
{
	std::map<CStdString, CFileInfo>::iterator j = m_ExFiles.find(strPathInRar );
	if( j == m_ExFiles.end() ) return false;
	strPathInCache = j->second.m_strCachedPath;
	if( !CFile::Exists(strPathInCache) ) return false;
	return true;
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
  std::map<CStdString, CFileInfo>::iterator j;
  for (j = m_ExFiles.begin() ; j != m_ExFiles.end() ; j++)
  {
    CFileInfo* pFile = &(j->second);
    if (pFile->m_bAutoDel && (pFile->m_iUsed < 1 || force))
      CFile::Delete( pFile->m_strCachedPath );
  }
  m_ExFiles.clear();
}

void CRarManager::ClearCachedFile(const CStdString& strRarPath, const CStdString& strPathInRar)
{
  CSingleLock lock(m_CritSection);
  
  std::map<CStdString, CFileInfo>::iterator j = m_ExFiles.find(strPathInRar);
  if (j == m_ExFiles.end())
  {
    return; // no such subpath
  }
  
  if (j->second.m_iUsed > 0)
    j->second.m_iUsed--;  
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