#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "BackgroundInfoLoader.h"
#include "MusicDatabase.h"

class CFileItemList;

namespace MUSIC_INFO
{
class CMusicInfoLoader : public CBackgroundInfoLoader
{
public:
  CMusicInfoLoader();
  virtual ~CMusicInfoLoader();

  void UseCacheOnHD(const CStdString& strFileName);
  virtual bool LoadItem(CFileItem* pItem);
  static bool LoadAdditionalTagInfo(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
  void LoadCache(const CStdString& strFileName, CFileItemList& items);
  void SaveCache(const CStdString& strFileName, CFileItemList& items);
protected:
  CStdString m_strCacheFileName;
  CFileItemList* m_mapFileItems;
  CSongMap m_songsMap;
  CStdString m_strPrevPath;
  CMusicDatabase m_musicDatabase;
  unsigned int m_databaseHits;
  unsigned int m_tagReads;
};
}
