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

class CFileItem;
class CFileItemList;

typedef struct bluray BLURAY;
typedef struct bd_title_info BLURAY_TITLE_INFO;

namespace XFILE
{

class CBlurayDirectory : public IDirectory
{
public:
  CBlurayDirectory() = default;
  ~CBlurayDirectory() override;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;

  bool InitializeBluray(const std::string& root);
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
  void GetTitles(GetTitlesJob job, CFileItemList& items, SortTitlesJob sort) const;
  void GetPlaylistInfo(ClipMap& clips, PlaylistMap& playlists) const;
  int GetUserPlaylists() const;
  std::shared_ptr<CFileItem> GetTitle(const BLURAY_TITLE_INFO* title,
                                      const std::string& label) const;

  CURL m_url;
  BLURAY* m_bd{nullptr};
  bool m_blurayInitialized{false};
  bool m_blurayMenuSupport{false};
};
} // namespace XFILE