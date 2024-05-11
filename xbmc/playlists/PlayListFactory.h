/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CFileItem;
class CURL;

namespace KODI::PLAYLIST
{
  class CPlayList;

  class CPlayListFactory
  {
  public:
    static CPlayList* Create(const std::string& filename);
    static CPlayList* Create(const CFileItem& item);
    static bool IsPlaylist(const CURL& url);
    static bool IsPlaylist(const std::string& filename);
    static bool IsPlaylist(const CFileItem& item);
  };
}
