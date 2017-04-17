/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>
#include <vector>
#include "FileItem.h"
#include "threads/CriticalSection.h"
#include "utils/EventStream.h"


class CFavouritesService
{
public:
  explicit CFavouritesService(std::string userDataFolder);
  virtual ~CFavouritesService() = default;

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

  const std::string m_userDataFolder;
  CFileItemList m_favourites;
  CEventSource<FavouritesUpdated> m_events;
  CCriticalSection m_criticalSection;
};

