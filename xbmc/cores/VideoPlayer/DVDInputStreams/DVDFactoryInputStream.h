/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CDVDInputStream;
class IVideoPlayer;

class CDVDFactoryInputStream
{
public:
  /*!
   \brief Create an input stream for the given file item.
   \param flags Additional XFILE:: open flags for file based input streams, for example
   XFILE::READ_NO_CACHE to read without a file cache.
   */
  static std::shared_ptr<CDVDInputStream> CreateInputStream(IVideoPlayer* pPlayer,
                                                            const CFileItem& fileitem,
                                                            bool scanforextaudio = false,
                                                            unsigned int flags = 0);
  static std::shared_ptr<CDVDInputStream> CreateInputStream(IVideoPlayer* pPlayer, const CFileItem &fileitem, const std::vector<std::string>& filenames);
};
