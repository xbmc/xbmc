/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/Directory.h"
#include "playlists/PlayListTypes.h"

#include <optional>
#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

class CGUIDialogSimpleMenu
{
public:

  /*! \brief Show dialog allowing selection of wanted playback item */
  static bool GetOrShowPlaylistSelection(
      CFileItem& item,
      KODI::PLAYLIST::ForcePlaylistSelection forceSelection =
          KODI::PLAYLIST::ForcePlaylistSelection::DONT_FORCE_PLAYLIST_SELECTION,
      KODI::PLAYLIST::ExcludeUsedPlaylists excludeUsedPlaylists =
          KODI::PLAYLIST::ExcludeUsedPlaylists::EXCLUDE_USED_PLAYLISTS);

protected:
  static bool GetDirectoryItems(const std::string& path,
                                CFileItemList& items,
                                const XFILE::CDirectory::CHints& hints);

private:
  static bool GetItems(const CFileItem& item,
                       CFileItemList& items,
                       const std::string& directory,
                       const std::optional<std::vector<int>>& excludePlaylists);
};
