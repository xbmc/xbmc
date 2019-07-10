/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "threads/CriticalSection.h"
#include "utils/EventStream.h"

#include <string>
#include <vector>


class CFavouritesService
{
public:
  explicit CFavouritesService(std::string userDataFolder);
  virtual ~CFavouritesService() = default;

  /** For profiles*/
  void ReInit(std::string userDataFolder);

  bool IsFavourited(const CFileItem& item, int contextWindow) const;
  void GetAll(CFileItemList& items) const;
  std::string GetExecutePath(const CFileItem &item, int contextWindow) const;
  std::string GetExecutePath(const CFileItem &item, const std::string &contextWindow) const;
  bool AddOrRemove(const CFileItem& item, int contextWindow);
  bool Save(const CFileItemList& items);

  struct FavouritesUpdated { };

  CEventStream<FavouritesUpdated>& Events() { return m_events; }

private:
  CFavouritesService() = delete;
  CFavouritesService(const CFavouritesService&) = delete;
  CFavouritesService& operator=(const CFavouritesService&) = delete;
  CFavouritesService(CFavouritesService&&) = delete;
  CFavouritesService& operator=(CFavouritesService&&) = delete;

  void OnUpdated();
  bool Persist();
  std::string GetFavouritesUrl(const CFileItem &item, int contextWindow) const;

  std::string m_userDataFolder;
  CFileItemList m_favourites;
  CEventSource<FavouritesUpdated> m_events;
  mutable CCriticalSection m_criticalSection;
};

