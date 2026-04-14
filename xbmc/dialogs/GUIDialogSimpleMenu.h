/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/VideoDatabase.h"

#include <vector>

class CFileItem;
class CFileItemList;

class CGUIDialogSimpleMenu
{
public:
  /*! \brief Show dialog allowing selection of wanted playback item */
  static bool ShowPlaylistSelection(const CFileItem& item,
                                    CFileItem& selectedItem,
                                    const CFileItemList& items,
                                    const std::vector<CVideoDatabase::PlaylistInfo>& usedPlaylists);
};