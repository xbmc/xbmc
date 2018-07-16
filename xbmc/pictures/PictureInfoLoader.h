/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

