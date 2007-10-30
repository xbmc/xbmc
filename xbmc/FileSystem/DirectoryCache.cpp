
#include "stdafx.h"
#include "DirectoryCache.h"
#include "../Util.h"

using namespace DIRECTORY;

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
  CUtil::RemoveSlashAtEnd(strPath);   // we cache without slash at end
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
        if ( strcmpi(pItem->m_strPath.c_str(), strFixedFile.c_str()) == 0 ) return true;
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
      i=g_directoryCache.m_vecCache.erase(i);
    }
    else
      i++;

  }
}

void CDirectoryCache::InitCache(set<CStdString>& dirs)
{
  set<CStdString>::iterator it;
  for (it = dirs.begin(); it != dirs.end(); ++it)
  {
    const CStdString& strDir = *it;
    CFileItemList items;
    CDirectory::GetDirectory(strDir, items, "", false);
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

bool CDirectoryCache::IsCacheDir(const CStdString &strPath)
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
/*    g_directoryCache.m_thumbDirs.insert(g_settings.GetThumbnailsFolder());
    for (unsigned int hex=0; hex < 16; hex++)
    {
      CStdString strHex;
      strHex.Format("\\%x",hex);
      g_directoryCache.m_thumbDirs.insert(g_settings.GetThumbnailsFolder() + strHex);
    }*/
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
    for (int i = 0; i < 16; i++)
    {
      CStdString hex, folder;
      hex.Format("%x", i);
      CUtil::AddFileToFolder(g_settings.GetMusicThumbFolder(), hex, folder);
      g_directoryCache.m_musicThumbDirs.insert(folder);
    }
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
