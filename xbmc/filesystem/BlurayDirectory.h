/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DiscDirectoryHelper.h"
#include "IDirectory.h"
#include "URL.h"

#include <memory>

#include <libbluray/bluray.h>

class CFileItem;
class CFileItemList;

namespace XFILE
{

class CBlurayDirectory : public IDirectory
{
public:
  CBlurayDirectory() = default;
  ~CBlurayDirectory() override;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;

  bool InitializeBluray(const std::string &root);
  static std::string GetBasePath(const CURL& url);
  std::string GetBlurayTitle();
  std::string GetBlurayID();

private:
  enum class DiscInfo
  {
    TITLE,
    ID
  };

  void Dispose();
  std::string GetDiscInfoString(DiscInfo info) const;
  void GetPlaylistsInformation(ClipMap& clips, PlaylistMap& playlists) const;
  bool GetPlaylists(GetTitles job, CFileItemList& items, SortTitles sort) const;
  int GetMainPlaylistFromDisc() const;
  std::shared_ptr<CFileItem> GetFileItem(const BLURAY_TITLE_INFO& title,
                                         const std::string& label) const;

  std::string GetCachePath() const;
  const BLURAY_DISC_INFO* GetDiscInfo() const;
  bool GetPlaylistInfoFromDisc(unsigned int playlist, BLURAY_TITLE_INFO& p) const;

  CURL m_url;
  std::string m_realPath;
  BLURAY* m_bd{nullptr};
  bool m_blurayInitialized{false};
  bool m_blurayMenuSupport{false};
};
} // namespace XFILE
