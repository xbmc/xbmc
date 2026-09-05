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
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayListFactory.h"
#include "pvr/PVRItem.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoFileItemClassify.h"

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

Id PlaylistIdOf(const CFileItem& item)
{
  if (item.IsPVRChannel() || item.IsPVRRecording() || item.IsEPG())
    return PVR::CPVRItem(item).IsRadio() ? Id::TYPE_MUSIC : Id::TYPE_VIDEO;

  const bool isVideo{VIDEO::IsVideo(item)};
  const bool isAudio{MUSIC::IsAudio(item)};

  if (isVideo && !isAudio)
    return Id::TYPE_VIDEO;

  if (isAudio && !isVideo)
    return Id::TYPE_MUSIC;

  // Neither classifier decides. Both answer true for an extension on both lists (.strm), and
  // both answer false for a generic path carrying no tags, so only the hint can say.
  if (item.HasProperty("playlist_type_hint"))
    return Id{item.GetProperty("playlist_type_hint").asInteger32(static_cast<int>(Id::TYPE_NONE))};

  return Id::TYPE_NONE;
}

} // namespace KODI::PLAYLIST
