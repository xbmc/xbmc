/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  ~CMusicInfoLoader() override;

  void UseCacheOnHD(const std::string& strFileName);
  bool LoadItem(CFileItem* pItem) override;
  bool LoadItemCached(CFileItem* pItem) override;
  bool LoadItemLookup(CFileItem* pItem) override;
  static bool LoadAdditionalTagInfo(CFileItem* pItem);

protected:
  void OnLoaderStart() override;
  void OnLoaderFinish() override;
  void LoadCache(const std::string& strFileName, CFileItemList& items);
  void SaveCache(const std::string& strFileName, CFileItemList& items);
protected:
  std::string m_strCacheFileName;
  CFileItemList* m_mapFileItems;
  MAPSONGS m_songsMap;
  std::string m_strPrevPath;
  CMusicDatabase m_musicDatabase;
  unsigned int m_databaseHits = 0;
  unsigned int m_tagReads = 0;
  CMusicThumbLoader *m_thumbLoader;
};
}
