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

class CFileItem;
class CURL;

namespace KODI::PLAYLIST
{
  class CPlayListFile;

  class CPlayListFactory
  {
  public:
    static CPlayListFile* Create(const CURL& url);
    static CPlayListFile* Create(const std::string& filename);
    static CPlayListFile* Create(const CFileItem& item);

  /*!
   * \return The playlist file at the item's path, read, or nullptr if it is not a playlist Kodi
   * can read or reading it failed.
   */
  static std::unique_ptr<CPlayListFile> Load(const CFileItem& item);
  static std::unique_ptr<CPlayListFile> Load(const std::string& filename);
    static bool IsPlaylist(const CURL& url);
    static bool IsPlaylist(const std::string& filename);
    static bool IsPlaylist(const CFileItem& item);
  };
}
