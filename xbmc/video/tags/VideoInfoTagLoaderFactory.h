#pragma once
/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
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

#include "IVideoInfoTagLoader.h"
#include "addons/Scraper.h"

class CFileItem;  // forward

namespace VIDEO
{
  class CVideoInfoTagLoaderFactory
  {
  public:
    //! \brief Returns a tag loader for the given item.
    //! \param item The item to find tag loader for
    //! \param type Type of tag loader. In particular used for tvshows
    static IVideoInfoTagLoader* CreateLoader(const CFileItem& item,
                                             ADDON::ScraperPtr info,
                                             bool lookInFolder,
                                             bool forceRefresh = false);

  protected:
    // No instancing of this class
    CVideoInfoTagLoaderFactory(void) = delete;
    virtual ~CVideoInfoTagLoaderFactory() = delete;
  };
}
