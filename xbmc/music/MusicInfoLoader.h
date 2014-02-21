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
#include "BackgroundInfoLoader.h"
#include "MusicDatabase.h"

class CFileItemList;
class CMusicThumbLoader;

namespace MUSIC_INFO
{
class CMusicInfoLoader : public CBackgroundInfoLoader
{
public:
  CMusicInfoLoader();
  virtual ~CMusicInfoLoader();

  void UseCacheOnHD(const std::string& strFileName);
  virtual bool LoadItem(CFileItem* pItem);
  virtual bool LoadItemCached(CFileItem* pItem);
  virtual bool LoadItemLookup(CFileItem* pItem);
  static bool LoadAdditionalTagInfo(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
  void LoadCache(const std::string& strFileName, CFileItemList& items);
  void SaveCache(const std::string& strFileName, CFileItemList& items);
protected:
  std::string m_strCacheFileName;
  CFileItemList* m_mapFileItems;
  MAPSONGS m_songsMap;
  std::string m_strPrevPath;
  CMusicDatabase m_musicDatabase;
  unsigned int m_databaseHits;
  unsigned int m_tagReads;
  CMusicThumbLoader *m_thumbLoader;
};
}
