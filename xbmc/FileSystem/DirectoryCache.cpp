
#include "stdafx.h"
#include "DirectoryCache.h"
#include "../util.h"

CDirectoryCache g_directoryCache;

CDirectoryCache::CDirectoryCache(void)
{
}

CDirectoryCache::~CDirectoryCache(void)
{
}


bool  CDirectoryCache::GetDirectory(const CStdString& strPath1,VECFILEITEMS &items) 
{
  {
    CFileItemList itemlist(items); // will clean up everything
  }

  CStdString strPath=strPath1;
	if (CUtil::HasSlashAtEnd(strPath))
		strPath.Delete(strPath.size()-1);

  ivecCache i=g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir& dir=*i;
    if (dir.m_strPath==strPath)
    {
      
      for (int i=0; i < (int) dir.m_Items.size(); ++i)
      {
        CFileItem* pItem= new CFileItem();
        (*pItem) = *(dir.m_Items[i]);
        items.push_back(pItem);
      }

      return true;
    }
    ++i;
  }
  return false;
}

void  CDirectoryCache::SetDirectory(const CStdString& strPath1,const VECFILEITEMS &items)
{
  CStdString strPath=strPath1;
	if (CUtil::HasSlashAtEnd(strPath))
		strPath.Delete(strPath.size()-1);

  g_directoryCache.ClearDirectory(strPath);
  CDir dir;
  dir.m_strPath=strPath;
  for (int i=0; i < (int) items.size(); ++i)
  {
    CFileItem* pItem = items[i];
    dir.m_Items.push_back(pItem);
  }
  g_directoryCache.m_vecCache.push_back(dir);
}

void  CDirectoryCache::ClearDirectory(const CStdString& strPath1)
{
  CStdString strPath=strPath1;
	if (CUtil::HasSlashAtEnd(strPath))
		strPath.Delete(strPath.size()-1);

  ivecCache i=g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir& dir=*i;
    if (dir.m_strPath==strPath)
    {
      {
        CFileItemList itemlist(dir.m_Items); // will clean up everything
      }

      g_directoryCache.m_vecCache.erase(i);
      return;
    }
    ++i;
  }
}

bool  CDirectoryCache::FileExists(const CStdString& strFile, bool& bInCache)
{
  CStdString strPath,strFileName;
  bInCache=false;
  CUtil::Split(strFile,strPath,strFileName);
  ivecCache i=g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir& dir=*i;
    if (dir.m_strPath==strPath)
    {
      bInCache=true;
      for (int i=0; i < (int) dir.m_Items.size(); ++i)
      {
        CFileItem* pItem=dir.m_Items[i];
        if ( CUtil::cmpnocase(pItem->m_strPath,strFile)==0)
        {
          return true;
        }
      }
    }
    ++i;
  }
  return false;
}

void  CDirectoryCache::Clear()
{
  ivecCache i=g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir& dir=*i;
    {
      CFileItemList itemlist(dir.m_Items); // will clean up everything
    }

    g_directoryCache.m_vecCache.erase(i);
  }
}
