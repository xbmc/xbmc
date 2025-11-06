/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "playlists/PlayListFileItemClassify.h"

#include "FileItem.h"
#include "URL.h"
#include "playlists/PlayListFactory.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

namespace KODI::PLAYLIST
{

bool IsPlayList(const CFileItem& item)
{
  return CPlayListFactory::IsPlaylist(item);
}

bool IsSmartPlayList(const CFileItem& item)
{
  if (item.GetProperty("library.smartplaylist").asBoolean(false))
    return true;

  return item.GetURL().HasExtension(".xsp");
}

} // namespace KODI::PLAYLIST
