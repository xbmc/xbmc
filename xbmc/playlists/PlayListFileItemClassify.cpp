/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "playlists/PlayListFileItemClassify.h"

#include "FileItem.h"
#include "playlists/PlayListFactory.h"

namespace KODI::PLAYLIST
{

bool IsPlayList(const CFileItem& item)
{
  return CPlayListFactory::IsPlaylist(item);
}

} // namespace KODI::PLAYLIST
