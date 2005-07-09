
#include "../stdafx.h"
#include "DirectoryCache.h"
#include "../util.h"

CDirectoryCache g_directoryCache;
CCriticalSection CDirectoryCache::m_cs;

CDirectoryCache::CDirectoryCache(void)
{
  m_iThumbCacheRefCount = 0;
  m_iMusicThumbCacheRefCount = 0;
}
CDirectoryCache::~CDirectoryCache(void)
{}
bool CDirectoryCache::GetDirectory(const CStdString& strPath1, CFileItemList &items)
{
  CSingleLock lock (m_cs);

  items.Clear();

  CStdString strPath = strPath1;
  if (CUtil::HasSlashAtEnd(strPath))
    strPath.Delete(strPath.size() - 1);

  ivecCache i = g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir* dir = *i;
    if (dir->m_strPath == strPath)
    {

      for (int i = 0; i < (int) dir->m_Items.Size(); ++i)
      {
        CFileItem* pItem = new CFileItem();
        (*pItem) = *(dir->m_Items[i]);
        items.Add(pItem);
      }

      return true;
    }
    ++i;
  }
  return false;
}

void CDirectoryCache::SetDirectory(const CStdString& strPath1, const CFileItemList &items)
{
  CSingleLock lock (m_cs);

  CStdString strPath = strPath1;
  if (CUtil::HasSlashAtEnd(strPath))
    strPath.Delete(strPath.size() - 1);

  g_directoryCache.ClearDirectory(strPath);
  CDir* dir = new CDir;
  dir->m_strPath = strPath;
  dir->m_Items.SetFastLookup(true);
  dir->m_Items.Append(items);
  g_directoryCache.m_vecCache.push_back(dir);
}

void CDirectoryCache::ClearDirectory(const CStdString& strPath1)
{
  CSingleLock lock (m_cs);

  CStdString strPath = strPath1;
  if (CUtil::HasSlashAtEnd(strPath))
    strPath.Delete(strPath.size() - 1);

  ivecCache i = g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir* dir = *i;
    if (dir->m_strPath == strPath)
    {
      dir->m_Items.Clear(); // will clean up everything
      delete dir;
      g_directoryCache.m_vecCache.erase(i);
      return ;
    }
    ++i;
  }
}

bool CDirectoryCache::FileExists(const CStdString& strFile, bool& bInCache)
{
  CSingleLock lock (m_cs);
  CStdString strPath, strFixedFile(strFile);
  bInCache = false;
  if ( strFixedFile.Mid(1, 1) == ":" )  strFixedFile.Replace('/', '\\');
  CUtil::GetDirectory(strFixedFile, strPath);
  ivecCache i = g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir* dir = *i;
    if (dir->m_strPath == strPath)
    {
      bInCache = true;
      for (int i = 0; i < (int) dir->m_Items.Size(); ++i)
      {
        CFileItem* pItem = dir->m_Items[i];
        if ( CUtil::CmpNoCase(pItem->m_strPath, strFixedFile) ) return true;
      }
    }
    ++i;
  }
  return false;
}

void CDirectoryCache::Clear()
{
  CSingleLock lock (m_cs);

  ivecCache i = g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir* dir = *i;
    if (!IsCacheDir(dir->m_strPath))
    {
      dir->m_Items.Clear(); // will clean up everything
      delete dir;
      g_directoryCache.m_vecCache.erase(i);
    }

  }
}

void CDirectoryCache::InitCache(set<CStdString>& dirs)
{
  set<CStdString>::iterator it;
  for (it = dirs.begin(); it != dirs.end(); ++it)
  {
    CStdString& strDir = *it;
    CFileItemList items;
    CDirectory::GetDirectory(strDir, items);
    items.Clear();
  }
}

void CDirectoryCache::ClearCache(set<CStdString>& dirs)
{
  ivecCache i = g_directoryCache.m_vecCache.begin();
  while (i != g_directoryCache.m_vecCache.end() )
  {
    CDir* dir = *i;
    if (dirs.find(dir->m_strPath) != dirs.end())
    {
      dir->m_Items.Clear(); // will clean up everything
      delete dir;
      g_directoryCache.m_vecCache.erase(i);
    }
    else
      i++;
  }
}

bool CDirectoryCache::IsCacheDir(CStdString strPath)
{
  if (g_directoryCache.m_thumbDirs.find(strPath) == g_directoryCache.m_thumbDirs.end())
    return false;
  if (g_directoryCache.m_musicThumbDirs.find(strPath) == g_directoryCache.m_musicThumbDirs.end())
    return false;

  return true;
}

void CDirectoryCache::InitThumbCache()
{
  CSingleLock lock (m_cs);

  if (g_directoryCache.m_iThumbCacheRefCount > 0)
  {
    g_directoryCache.m_iThumbCacheRefCount++;
    return ;
  }
  g_directoryCache.m_iThumbCacheRefCount++;

  // Init video, pictures cache directories
  if (g_directoryCache.m_thumbDirs.size() == 0)
  {
    // thumbnails directories
    CStdString strThumb = g_stSettings.szThumbnailsDirectory;
    g_directoryCache.m_thumbDirs.insert(strThumb);
    strThumb += "\\imdb";
    g_directoryCache.m_thumbDirs.insert(strThumb);
    for (unsigned int hex=0; hex < 16; hex++)
    {
      CStdString strThumbLoc = g_stSettings.szThumbnailsDirectory;
      CStdString strHex;
      strHex.Format("%x",hex);
      strThumbLoc += "\\" + strHex;
      g_directoryCache.m_thumbDirs.insert(strThumbLoc);
    }
  }

  InitCache(g_directoryCache.m_thumbDirs);
}

void CDirectoryCache::ClearThumbCache()
{
  CSingleLock lock (m_cs);

  if (g_directoryCache.m_iThumbCacheRefCount > 1)
  {
    g_directoryCache.m_iThumbCacheRefCount--;
    return ;
  }

  g_directoryCache.m_iThumbCacheRefCount--;
  ClearCache(g_directoryCache.m_thumbDirs);
}

void CDirectoryCache::InitMusicThumbCache()
{
  CSingleLock lock (m_cs);

  if (g_directoryCache.m_iMusicThumbCacheRefCount > 0)
  {
    g_directoryCache.m_iMusicThumbCacheRefCount++;
    return ;
  }
  g_directoryCache.m_iMusicThumbCacheRefCount++;

  // Init music cache directories
  if (g_directoryCache.m_musicThumbDirs.size() == 0)
  {
    // music thumbnails directories
    CStdString strThumb = g_stSettings.m_szAlbumDirectory;
    strThumb += "\\thumbs";
    g_directoryCache.m_musicThumbDirs.insert(strThumb);
    strThumb += "\\temp";
    g_directoryCache.m_musicThumbDirs.insert(strThumb);
  }

  InitCache(g_directoryCache.m_musicThumbDirs);
}

void CDirectoryCache::ClearMusicThumbCache()
{
  CSingleLock lock (m_cs);

  if (g_directoryCache.m_iMusicThumbCacheRefCount > 1)
  {
    g_directoryCache.m_iMusicThumbCacheRefCount--;
    return ;
  }

  g_directoryCache.m_iMusicThumbCacheRefCount--;
  ClearCache(g_directoryCache.m_musicThumbDirs);
}
