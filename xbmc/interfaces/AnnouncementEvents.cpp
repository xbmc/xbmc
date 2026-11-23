/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AnnouncementEvents.h"

#include "FileItem.h"
#include "interfaces/AnnouncementManager.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"

#include <type_traits>

namespace ANNOUNCEMENT
{

bool EVENT::PLAYER::IsPicture(const CFileItem* item)
{
  return item && !item->HasPVRChannelInfoTag() &&
         !(item->HasVideoInfoTag() && !item->HasPVRRecordingInfoTag()) &&
         !item->HasMusicInfoTag() && !KODI::VIDEO::IsVideo(*item) && item->HasPictureInfoTag();
}

bool IsTransaction(const LibraryEvent& event)
{
  if (const auto* update = std::get_if<EVENT::LIBRARY::Update>(&event))
    return update->transaction;
  if (const auto* remove = std::get_if<EVENT::LIBRARY::Remove>(&event))
    return remove->transaction;
  return false;
}

AnnouncementFlag FlagOf(const Announcement& announcement)
{
  return std::visit([](const auto& flagEvent) { return std::decay_t<decltype(flagEvent)>::FLAG; },
                    static_cast<const Announcement::variant&>(announcement));
}

const char* MessageOf(const Announcement& announcement)
{
  return std::visit(
      [](const auto& flagEvent) -> const char*
      {
        using FlagEvent = std::decay_t<decltype(flagEvent)>;
        if constexpr (std::is_same_v<FlagEvent, OtherEvent>)
          return flagEvent.message.c_str();
        else
          return std::visit([](const auto& event)
                            { return std::decay_t<decltype(event)>::MESSAGE; },
                            static_cast<const typename FlagEvent::variant&>(flagEvent));
      },
      static_cast<const Announcement::variant&>(announcement));
}

const std::string& SenderOf(const Announcement& announcement)
{
  if (const auto* other = std::get_if<OtherEvent>(&announcement))
    return other->sender;
  return CAnnouncementManager::ANNOUNCEMENT_SENDER;
}

std::shared_ptr<const CFileItem> ItemOf(const Announcement& announcement)
{
  return std::visit(
      [](const auto& flagEvent) -> std::shared_ptr<const CFileItem>
      {
        using FlagEvent = std::decay_t<decltype(flagEvent)>;
        if constexpr (std::is_same_v<FlagEvent, OtherEvent>)
          return nullptr;
        else
          return std::visit(
              [](const auto& event) -> std::shared_ptr<const CFileItem>
              {
                if constexpr (requires { event.item; })
                  return event.item;
                else
                  return nullptr;
              },
              static_cast<const typename FlagEvent::variant&>(flagEvent));
      },
      static_cast<const Announcement::variant&>(announcement));
}

Announcement WithLibraryDetails(Announcement announcement)
{
  const std::shared_ptr<const CFileItem> item = ItemOf(announcement);
  if (!item || item->GetPath().empty() || item->HasPVRChannelInfoTag())
    return announcement;

  //! @todo Can be removed once this is properly handled when starting playback of a file
  if (item->HasVideoInfoTag() && !item->HasPVRRecordingInfoTag())
  {
    if (item->GetVideoInfoTag()->GetDatabaseId() > 0)
      return announcement;

    CVideoDatabase videodatabase;
    if (!videodatabase.Open())
    {
      CLog::LogFC(LOGWARNING, LOGANNOUNCE,
                  "Unable to open video database. Can not load video tag for announcement!");
      return announcement;
    }
    CVideoInfoTag tag = *item->GetVideoInfoTag();
    const std::string path = StringUtils::StartsWith(tag.m_strFileNameAndPath, "removable://")
                                 ? tag.m_strFileNameAndPath
                                 : item->GetPath();
    if (!videodatabase.LoadVideoInfo(path, tag))
      return announcement;

    auto loaded = std::make_shared<CFileItem>(*item);
    *loaded->GetVideoInfoTag() = std::move(tag);
    return WithItem(std::move(announcement), std::move(loaded));
  }

  if (item->HasMusicInfoTag())
  {
    if (item->GetMusicInfoTag()->GetDatabaseId() > 0)
      return announcement;

    CMusicDatabase musicdatabase;
    if (!musicdatabase.Open())
    {
      CLog::LogFC(LOGWARNING, LOGANNOUNCE,
                  "Unable to open music database. Can not load song tag for announcement!");
      return announcement;
    }
    CSong song;
    if (!musicdatabase.GetSongByFileName(item->GetPath(), song, item->GetStartOffset()))
      return announcement;

    auto loaded = std::make_shared<CFileItem>(*item);
    loaded->GetMusicInfoTag()->SetSong(song);
    return WithItem(std::move(announcement), std::move(loaded));
  }

  return announcement;
}

Announcement WithItem(Announcement announcement, std::shared_ptr<const CFileItem> item)
{
  std::visit(
      [&item](auto& flagEvent)
      {
        using FlagEvent = std::decay_t<decltype(flagEvent)>;
        if constexpr (!std::is_same_v<FlagEvent, OtherEvent>)
          std::visit(
              [&item](auto& event)
              {
                if constexpr (requires { event.item; })
                  event.item = std::move(item);
              },
              static_cast<typename FlagEvent::variant&>(flagEvent));
      },
      static_cast<Announcement::variant&>(announcement));
  return announcement;
}

} // namespace ANNOUNCEMENT
