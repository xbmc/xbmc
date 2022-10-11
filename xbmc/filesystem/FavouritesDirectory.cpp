/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesDirectory.h"

#include "Directory.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "favourites/FavouritesService.h"
#include "profiles/ProfileManager.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"

namespace XFILE
{

bool CFavouritesDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  items.Clear();
  CServiceBroker::GetFavouritesService().GetAll(items);
  return true;
}

bool CFavouritesDirectory::Exists(const CURL& url)
{
  if (url.IsProtocol("favourites"))
  {
    if (CFileUtils::Exists("special://xbmc/system/favourites.xml"))
      return true;

    const std::string favouritesXml = URIUtils::AddFileToFolder(m_profileManager->GetProfileUserDataFolder(), "favourites.xml");

    return CFileUtils::Exists(favouritesXml);
  }

  return XFILE::CDirectory::Exists(url);
}
} // namespace XFILE
