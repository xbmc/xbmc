#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "BackgroundInfoLoader.h"
#include <string>

class CPictureInfoLoader : public CBackgroundInfoLoader
{
public:
  CPictureInfoLoader();
  ~CPictureInfoLoader() override;

  void UseCacheOnHD(const std::string& strFileName);
  bool LoadItem(CFileItem* pItem) override;
  bool LoadItemCached(CFileItem* pItem) override;
  bool LoadItemLookup(CFileItem* pItem) override;

protected:
  void OnLoaderStart() override;
  void OnLoaderFinish() override;

  CFileItemList* m_mapFileItems;
  unsigned int m_tagReads;
  bool m_loadTags;
};

