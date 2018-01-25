/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FavouritesDirectory.h"
#include "favourites/FavouritesService.h"
#include "File.h"
#include "Directory.h"
#include "Util.h"
#include "profiles/ProfilesManager.h"
#include "ServiceBroker.h"
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
    if (XFILE::CFile::Exists("special://xbmc/system/favourites.xml"))
      return true;

    std::string favouritesXml = URIUtils::AddFileToFolder(m_profileManager.GetProfileUserDataFolder(),
        "favourites.xml");

    return XFILE::CFile::Exists(favouritesXml);
  }

  return XFILE::CDirectory::Exists(url);
}
} // namespace XFILE
