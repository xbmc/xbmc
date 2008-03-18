#pragma once

#include "Directory.h"

namespace DIRECTORY
{

class CDirectoryCache
{
  class CDir
  {
  public:
    CStdString m_strPath;
    CFileItemList m_Items;
  };
public:
  CDirectoryCache(void);
  virtual ~CDirectoryCache(void);
  static bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  static void SetDirectory(const CStdString& strPath, const CFileItemList &items);
  static void ClearDirectory(const CStdString& strPath);
  static void Clear();
  static bool FileExists(const CStdString& strPath, bool& bInCache);
  static void InitThumbCache();
  static void ClearThumbCache();
  static void InitMusicThumbCache();
  static void ClearMusicThumbCache();
protected:
  static void InitCache(std::set<CStdString>& dirs);
  static void ClearCache(std::set<CStdString>& dirs);
  static bool IsCacheDir(const CStdString &strPath);

  std::vector<CDir*> m_vecCache;
  typedef std::vector<CDir*>::iterator ivecCache;

  static CCriticalSection m_cs;
  std::set<CStdString> m_thumbDirs;
  std::set<CStdString> m_musicThumbDirs;
  int m_iThumbCacheRefCount;
  int m_iMusicThumbCacheRefCount;
};

}
extern DIRECTORY::CDirectoryCache g_directoryCache;
