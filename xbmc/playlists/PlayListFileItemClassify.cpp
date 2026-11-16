/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "playlists/PlayListFileItemClassify.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayListFactory.h"
#include "pvr/PVRItem.h"
#include "utils/ItemProperties.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoFileItemClassify.h"

#include <algorithm>
#include <optional>

namespace KODI::PLAYLIST
{

bool IsPlayList(const CFileItem& item)
{
  return CPlayListFactory::IsPlaylist(item);
}

bool IsSmartPlayList(const CFileItem& item)
{
  if (item.GetProperty(ITEM::PROPERTY::LIBRARY_SMARTPLAYLIST).asBoolean(false))
    return true;

  return item.GetURL().HasExtension(".xsp");
}

namespace
{
std::optional<Type> TypeSaidBy(const CFileItemList& items)
{
  if (std::ranges::any_of(items, [](const auto& item) { return VIDEO::IsVideo(*item); }))
    return Video;
  if (std::ranges::any_of(items, [](const auto& item) { return MUSIC::IsAudio(*item); }))
    return Audio;
  return std::nullopt;
}
} // namespace

Type TypeFor(const CFileItemList& items)
{
  return TypeSaidBy(items).value_or(Video);
}

Type TypeFor(const CFileItemList& entries, const CFileItem& source)
{
  if (const std::optional<Type> said = TypeSaidBy(entries); said)
    return *said;
  return TypeFor(source);
}

Type TypeFor(const CFileItem& item)
{
  return StreamsOf(item) == MEDIA::Streams::Audio ? Audio : Video;
}

std::optional<MEDIA::Streams> StreamsOf(const CFileItem& item)
{
  // a PVR item answers from its tag: a radio channel or recording can carry a video stream
  if (item.IsPVRChannel() || item.IsPVRRecording() || item.IsEPG())
    return PVR::CPVRItem(item).IsRadio() ? MEDIA::Streams::Audio : MEDIA::Streams::VideoAndAudio;
  if (item.IsPicture())
    return MEDIA::Streams::Video;
  if (VIDEO::IsVideo(item) || item.HasGameInfoTag())
    return MEDIA::Streams::VideoAndAudio;
  if (MUSIC::IsAudio(item))
    return MEDIA::Streams::Audio;
  return std::nullopt;
}

bool YieldsNoEntries(const CFileItem& item)
{
  return item.IsParentFolder() || item.IsZIP() || item.IsRAR();
}

bool HoldsEntries(const CFileItem& item)
{
  // a game's list of discs is known by its tag; IsGame() would also claim any .m3u that some
  // installable emulator accepts
  return IsSmartPlayList(item) || (IsPlayList(item) && !item.HasGameInfoTag());
}

bool CanBeEntry(const CFileItem& item)
{
  return !item.IsFolder() && !item.IsNFO() && !YieldsNoEntries(item);
}

} // namespace KODI::PLAYLIST
