/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DiscDirectoryHelper.h"
#include "threads/CriticalSection.h"

#include <map>

#include <libbluray/bluray.h>

namespace XFILE
{
class CBlurayDiscCache
{
  class CDisc
  {
  public:
    explicit CDisc(const BLURAY_DISC_INFO* discInfo);
    CDisc(CDisc&& dir) = default;
    CDisc& operator=(CDisc&& dir) = default;
    virtual ~CDisc();

    BLURAY_DISC_INFO m_disc_info;
    std::map<unsigned int, BLURAY_TITLE_INFO> m_playlists;
    bool m_mapsSet{false};
    PlaylistMap m_playlistmap;
    ClipMap m_clipmap;

  private:
    CDisc(const CDisc&) = delete;
    CDisc& operator=(const CDisc&) = delete;
  };

public:
  CBlurayDiscCache();
  virtual ~CBlurayDiscCache();
  void Clear();

  void SetDisc(const std::string& path, const BLURAY_DISC_INFO* discInfo);
  void SetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       BLURAY_TITLE_INFO* playlistInfo);
  void SetMaps(const std::string& path, const PlaylistMap& playlistmap, const ClipMap& clipmap);

  bool GetDiscInfo(const std::string& path, const BLURAY_DISC_INFO*& discInfo);
  bool GetPlaylistInfo(const std::string& path,
                       unsigned int playlist,
                       BLURAY_TITLE_INFO& playlistInfo);
  bool GetMaps(const std::string& path, PlaylistMap& playlistmap, ClipMap& clipmap);

  void ClearDisc(const std::string& path);

protected:
  std::map<std::string, CDisc> m_cache;

  mutable CCriticalSection m_cs;
};
} // namespace XFILE
extern XFILE::CBlurayDiscCache g_blurayDiscCache;
