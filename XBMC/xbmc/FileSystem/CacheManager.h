/*
* XBMC
* CacheManager
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "../../guilib/StdString.h"
#include <stdlib.h>
#include <vector>
#include "../FileItem.h"
#include "../Util.h"

#define TIMEOUT 480000

class CCache
{
private:
  CStdString m_Path;
  bool m_IsFolder;
  __int64 m_Size;
public:
  CCache(CStdString Path, bool IsFolder, __int64 Size = 0);
  ~CCache() {}
  bool IsFolder() const;
  CStdString Path() const;
  const __int64 Size() const;
  bool Equals(const CStdString& comp) const;
};

class CCacheEntry
{
private:
  std::vector<CCache *> m_FilesInCache;
  CStdString m_ID;
  unsigned long m_Time;
  bool  m_AutoDelete;
public:
  CCacheEntry();
  CCacheEntry(CStdString ID);
  ~CCacheEntry();

  void CheckIn();
  unsigned int GetTimeOut() const;

  bool Contains(const CStdString& Path) const;
  void Add(CCache *Path);

  void Clear();

  void Print() const;

  const CCache *GetEntry(const CStdString& Path);

  bool AutoDelete() const;
  bool AutoDelete(bool OK);

  const std::vector<CCache *> List(const CStdString& strPathInCacheEntry);

  CStdString GetID() const;
};


class CCacheManager
{
private:
  std::vector<CCacheEntry *> m_Cached;
public:
  CCacheManager();
  ~CCacheManager();
  bool  List(CFileItemList& vecpItems, const CStdString& strCacheEntry, const CStdString& strPathInCacheEntry);
  bool  IsInCache(const CStdString& strCacheEntry, const CStdString& strPathInCacheEntry) const;
  bool  IsCached(const CStdString& strCacheEntry) const;
  bool  AddToCache(const CStdString& strCacheEntry, const CStdString& strPathInCacheEntry, bool IsFolder);
  bool  AddToCache(const CStdString& strCacheEntry, CCache *Cache);
  void  Print() const;
  void  Clear(int Pos);
  void  Clear(const CStdString& strCacheEntry);
  void  Clear();
  const CCache* GetCached(const CStdString& strCacheEntry, const CStdString& strPathInCacheEntry) const;

  void  AutoDelete(const CStdString& strCacheEntry, bool AutoDelete);

  void  CheckIn();
protected:
  CCacheEntry *AddCacheEntry(const CStdString& strCacheEntry);
  int   GetCacheEntry(const CStdString& strCacheEntry) const;
};

extern CCacheManager g_CacheManager;
