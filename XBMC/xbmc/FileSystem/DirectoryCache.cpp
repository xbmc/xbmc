
#include "stdafx.h"
#include "DirectoryCache.h"
#include "../util.h"
#include "../utils/SingleLock.h"
#include "../settings.h"

CDirectoryCache g_directoryCache;

CCriticalSection CDirectoryCache::m_cs;

CDirectoryCache::CDirectoryCache(void)
{
	m_iThumbCacheRefCount=0;
	m_iMusicThumbCacheRefCount=0;
}

CDirectoryCache::~CDirectoryCache(void)
{
}


bool  CDirectoryCache::GetDirectory(const CStdString& strPath1,VECFILEITEMS &items) 
{
	CSingleLock lock(m_cs);

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
	CSingleLock lock(m_cs);

  CStdString strPath=strPath1;
	if (CUtil::HasSlashAtEnd(strPath))
		strPath.Delete(strPath.size()-1);

  g_directoryCache.ClearDirectory(strPath);
  CDir dir;
  dir.m_strPath=strPath;
	dir.m_Items.reserve(items.size());

  for (int i=0; i < (int) items.size(); ++i)
  {
    CFileItem* pItem = items[i];
    dir.m_Items.push_back(pItem);
  }
  g_directoryCache.m_vecCache.push_back(dir);
}

void  CDirectoryCache::ClearDirectory(const CStdString& strPath1)
{
	CSingleLock lock(m_cs);

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
	CSingleLock lock(m_cs);

  CStdString strPath;
  bInCache=false;
  CUtil::GetDirectory(strFile,strPath);
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
	CSingleLock lock(m_cs);

  ivecCache i=g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir& dir=*i;
		if (!IsCacheDir(dir.m_strPath))
    {
			{
				CFileItemList itemlist(dir.m_Items); // will clean up everything
			}
			g_directoryCache.m_vecCache.erase(i);
    }

  }
}

void  CDirectoryCache::InitCache(set<CStdString>& dirs)
{
	CDirectory dir;

	set<CStdString>::iterator it;
	for (it=dirs.begin(); it!=dirs.end(); ++it)
	{
    CStdString& strDir=*it;
		VECFILEITEMS items;
		CFileItemList itemList(items);
		dir.GetDirectory(strDir, items);
	}
}

void  CDirectoryCache::ClearCache(set<CStdString>& dirs)
{
  ivecCache i=g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir& dir=*i;
		if (dirs.find(dir.m_strPath)!=dirs.end())
    {
			{
				CFileItemList itemlist(dir.m_Items); // will clean up everything
			}
			g_directoryCache.m_vecCache.erase(i);
    }
		else
			i++;
  }
}

bool  CDirectoryCache::IsCacheDir(CStdString strPath)
{
	if (g_directoryCache.m_thumbDirs.find(strPath)==g_directoryCache.m_thumbDirs.end())
		return false;
	if (g_directoryCache.m_musicThumbDirs.find(strPath)==g_directoryCache.m_musicThumbDirs.end())
		return false;

	return true;
}

void  CDirectoryCache::InitThumbCache()
{
	CSingleLock lock(m_cs);

	if (g_directoryCache.m_iThumbCacheRefCount>0)
	{
		g_directoryCache.m_iThumbCacheRefCount++;
		return;
	}
	g_directoryCache.m_iThumbCacheRefCount++;

	//	Init video, pictures cache directories
	if (g_directoryCache.m_thumbDirs.size()==0)
	{
		//	thumbnails directories
		CStdString strThumb=g_stSettings.szThumbnailsDirectory;
		g_directoryCache.m_thumbDirs.insert(strThumb);
		strThumb+="\\imdb";
		g_directoryCache.m_thumbDirs.insert(strThumb);
	}

	InitCache(g_directoryCache.m_thumbDirs);
}

void  CDirectoryCache::ClearThumbCache()
{
	CSingleLock lock(m_cs);

	if (g_directoryCache.m_iThumbCacheRefCount>1)
	{
		g_directoryCache.m_iThumbCacheRefCount--;
		return;
	}

	g_directoryCache.m_iThumbCacheRefCount--;
	ClearCache(g_directoryCache.m_thumbDirs);
}

void  CDirectoryCache::InitMusicThumbCache()
{
	CSingleLock lock(m_cs);

	if (g_directoryCache.m_iMusicThumbCacheRefCount>0)
	{
		g_directoryCache.m_iMusicThumbCacheRefCount++;
		return;
	}
	g_directoryCache.m_iMusicThumbCacheRefCount++;

	//	Init music cache directories
	if (g_directoryCache.m_musicThumbDirs.size()==0)
	{
		//	music thumbnails directories
		CStdString strThumb=g_stSettings.m_szAlbumDirectory;
		strThumb+="\\thumbs";
		g_directoryCache.m_musicThumbDirs.insert(strThumb);
		strThumb+="\\temp";
		g_directoryCache.m_musicThumbDirs.insert(strThumb);
	}

	InitCache(g_directoryCache.m_musicThumbDirs);
}

void  CDirectoryCache::ClearMusicThumbCache()
{
	CSingleLock lock(m_cs);

	if (g_directoryCache.m_iMusicThumbCacheRefCount>1)
	{
		g_directoryCache.m_iMusicThumbCacheRefCount--;
		return;
	}

	g_directoryCache.m_iMusicThumbCacheRefCount--;
	ClearCache(g_directoryCache.m_musicThumbDirs);
}
