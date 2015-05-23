#if !defined(AFX_RARMANAGER_H__06BA7C2E_3FCA_11D9_8186_0050FC718317__INCLUDED_)
#define AFX_RARMANAGER_H__06BA7C2E_3FCA_11D9_8186_0050FC718317__INCLUDED_

#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include "threads/CriticalSection.h"
#include <map>
#include <vector>
#include "UnrarXLib/UnrarX.hpp"
#include "utils/Stopwatch.h"

class CFileItemList;

#define EXFILE_OVERWRITE 1
#define EXFILE_AUTODELETE 2
#define EXFILE_UNIXPATH 4
#define EXFILE_NOCACHE 8
#define RAR_DEFAULT_CACHE "special://temp/"
#define RAR_DEFAULT_PASSWORD ""

class CFileInfo{
public:
  CFileInfo();
  ~CFileInfo();
  std::string m_strCachedPath;
  std::string m_strPathInRar;
  bool  m_bAutoDel;
  int m_iUsed;
  int64_t m_iOffset;

  bool m_bIsCanceled()
  {
    if (watch.IsRunning())
      if (watch.GetElapsedSeconds() < 3)
        return true;

    watch.Stop();
    return false;
  }
  CStopWatch watch;
  int m_iIsSeekable;
};

class CRarManager
{
public:
  CRarManager();
  ~CRarManager();
  bool CacheRarredFile(std::string& strPathInCache, const std::string& strRarPath,
                       const std::string& strPathInRar, uint8_t bOptions = EXFILE_AUTODELETE,
                       const std::string& strDir =RAR_DEFAULT_CACHE, const int64_t iSize=-1);
  bool GetPathInCache(std::string& strPathInCache, const std::string& strRarPath,
                      const std::string& strPathInRar = "");
  bool GetFilesInRar(CFileItemList& vecpItems, const std::string& strRarPath,
                     bool bMask=true, const std::string& strPathInRar="");
  CFileInfo* GetFileInRar(const std::string& strRarPath, const std::string& strPathInRar);
  bool IsFileInRar(bool& bResult, const std::string& strRarPath, const std::string& strPathInRar);
  void ClearCache(bool force=false);
  void ClearCachedFile(const std::string& strRarPath, const std::string& strPathInRar);
  void ExtractArchive(const std::string& strArchive, const std::string& strPath);
protected:

  bool ListArchive(const std::string& strRarPath, ArchiveList_struct* &pArchiveList);
  std::map<std::string, std::pair<ArchiveList_struct*,std::vector<CFileInfo> > > m_ExFiles;
  CCriticalSection m_CritSection;

  int64_t CheckFreeSpace(const std::string& strDrive);
};

extern CRarManager g_RarManager;
#endif

