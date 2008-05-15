#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Directory.h"

#include <set>

class CFileItem;

namespace DIRECTORY
{

class CDirectoryCache
{
  class CDir
  {
  public:
    CStdString m_strPath;
    CFileItemList* m_Items;
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
