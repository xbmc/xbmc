/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IVideoInfoTagLoader.h"
#include "addons/Scraper.h"

class CFileItem;  // forward

namespace KODI::VIDEO
{
  class CVideoInfoTagLoaderFactory
  {
  public:
    //! \brief Returns a tag loader for the given item.
    //! \param item The item to find tag loader for
    //! \param type Type of tag loader. In particular used for tvshows
    static IVideoInfoTagLoader* CreateLoader(const CFileItem& item,
                                             const ADDON::ScraperPtr& info,
                                             bool lookInFolder,
                                             bool forceRefresh = false);

  protected:
    // No instancing of this class
    CVideoInfoTagLoaderFactory(void) = delete;
    virtual ~CVideoInfoTagLoaderFactory() = delete;
  };
  } // namespace KODI::VIDEO
