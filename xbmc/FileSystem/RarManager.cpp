#include "../stdafx.h"
#include "RarManager.h"
#include "../lib/UnrarXLib/UnrarX.hpp"
#include "../util.h"
#include "../utils/singlelock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRarManager g_RarManager;

CFileInfo::CFileInfo()
{
	m_strCachedPath.Empty();
	m_bAutoDel = true;
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
	ClearCache();
}

bool CRarManager::CacheRarredFile(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar, BYTE  bOptions, const CStdString& strDir)
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
				return true;
			}
      CFile::Delete( pFile->m_strCachedPath );
		}
	}
  //Extract archived file, using existing local copy or overwriting if wanted...
  if( !urarlib_get(const_cast<char*>(strRarPath.c_str()), const_cast<char*>(strDir.c_str()),const_cast<char*>(strPathInRar.c_str()), "") )
    return false;
	if(!pFile) 
  {
    CFileInfo fileInfo;
    m_ExFiles.insert(std::make_pair<CStdString,CFileInfo>(strPathInRar,fileInfo));
    pFile = &(m_ExFiles[strPathInRar]);
  }
	pFile->m_strCachedPath = strDir+strPathInRar;
	pFile->m_bAutoDel = (bOptions & EXFILE_AUTODELETE) != 0;
	strPathInCache = pFile->m_strCachedPath;

	return true;
}

bool CRarManager::GetFilesInRar(CFileItemList& vecpItems, const CStdString& strRarPath, bool bMask, const CStdString& strPathInRar)
{
	CSingleLock lock(m_CritSection);
	ArchiveList_struct* pFileList;
	//if( !urarlib_list((char*) pRar->m_strCachedPath.c_str(), &pFileList, NULL) ) return false;
  if( !urarlib_list((char*) strRarPath.c_str(), &pFileList, NULL) ) return false;

	CFileItem* pFileItem = NULL;
  vector<CStdString> vec;
  CUtil::Tokenize(strPathInRar,vec,"\\");
  unsigned int iDepth = vec.size();
  
  for( ; pFileList  ; pFileList?pFileList = pFileList->next:pFileList)
	{
    vec.clear();
    CUtil::Tokenize(pFileList->item.Name,vec,"\\");
    if ((vec.size() > iDepth+1) || (vec.size() < iDepth))
      continue;
    char cDirDelimiter = (pFileList->item.HostOS==3 ? '/':'\\'); // win32 or unix paths?
    if (!strstr(pFileList->item.Name,strPathInRar.c_str()))
      continue;
    if ((pFileList->item.FileAttr & 16) ) // we have a directory
    {
      if (vec.size() == iDepth)
        continue; // remove root of listing
      pFileItem = new CFileItem(pFileList->item.Name+strPathInRar.size());
      pFileItem->m_strPath = pFileList->item.Name+strPathInRar.size();
      pFileItem->m_strPath += cDirDelimiter;
      pFileItem->m_bIsFolder = true;
      CStdString strDirName = pFileList->item.Name;
    }
    else
    {
      pFileItem = new CFileItem(pFileList->item.Name+strPathInRar.size());
		  pFileItem->m_strPath = pFileList->item.Name+strPathInRar.size();
		  pFileItem->m_dwSize = pFileList->item.UnpSize;
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
	if( !GetFilesInRar(ItemList,strRarPath) )	return false;
	int it;
  for( it=0;it<ItemList.Size();++it ) 
	{
		if( strPathInRar.compare(ItemList[it]->m_strPath) == 0)
			break;
	}
  if ( it != ItemList.Size() ) bResult = true;
	return true;
}
void CRarManager::MakeCachedPath(CStdString& strFileCachedPath, const CStdString& strCachePath, const CStdString& strFilePath)
{
		CStdString strIllegalPath =  CUtil::GetFileName(strFilePath);
		char* szPath = strIllegalPath.GetBuffer();
		MakeNameUsable(szPath, true,true);
		strIllegalPath.ReleaseBuffer();
		strFileCachedPath = strCachePath + strIllegalPath;
}

void CRarManager::ClearCache()
{
  CSingleLock lock(m_CritSection);
  std::map<CStdString, CFileInfo>::iterator j;
  for (j = m_ExFiles.begin() ; j != m_ExFiles.end() ; j++)
  {
    CFileInfo* pFile = &(j->second);
    if( pFile->m_bAutoDel )
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
  if (CFile::Delete(j->second.m_strCachedPath))
    m_ExFiles.erase(j);
}

void CRarManager::ExtractArchive(const CStdString& strArchive, const CStdString& strPath)
{
  urarlib_get(const_cast<char*>(strArchive.c_str()),const_cast<char*>(strPath.c_str()),NULL);
}