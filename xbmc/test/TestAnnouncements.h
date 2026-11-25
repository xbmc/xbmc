/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceBroker.h"
#include "interfaces/AnnouncementEvents.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/AnnouncementV14.h"
#include "media/MediaType.h"
#include "utils/Variant.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

//! \brief Never started, so it keeps every announcement made in its queue
class CQueuedAnnouncements : public ANNOUNCEMENT::CAnnouncementManager
{
public:
  long CountRemoved(KODI::MEDIA::TYPE type, int id) const
  {
    return Count<ANNOUNCEMENT::EVENT::LIBRARY::Remove>(type, id);
  }

  long CountUpdated(KODI::MEDIA::TYPE type, int id) const
  {
    return Count<ANNOUNCEMENT::EVENT::LIBRARY::Update>(type, id);
  }

  //! A Files announcement, as its message and the data JSON-RPC and Python are given
  struct FileAnnouncement
  {
    std::string message;
    CVariant data;
  };

  std::vector<FileAnnouncement> Files() const
  {
    std::vector<FileAnnouncement> files;
    for (const ANNOUNCEMENT::Announcement& announcement : m_announcementQueue)
    {
      // Temporary: the data as JSON-RPC v14 gives it; removed with JSON-RPC v15.
      if (ANNOUNCEMENT::FlagOf(announcement) == ANNOUNCEMENT::Files)
        files.push_back({ANNOUNCEMENT::MessageOf(announcement),
                         ANNOUNCEMENT::V14::NotificationsOf(announcement).at(0).data});
    }
    return files;
  }

private:
  template<class Event>
  long Count(KODI::MEDIA::TYPE type, int id) const
  {
    return std::ranges::count_if(m_announcementQueue,
                                 [type, id](const ANNOUNCEMENT::Announcement& announcement)
                                 {
                                   const auto* library{
                                       std::get_if<ANNOUNCEMENT::VideoLibraryEvent>(&announcement)};
                                   const auto* event{std::get_if<Event>(library)};
                                   return event && event->type == type && event->id == id;
                                 });
  }
};

/*!
 * \brief Registers an announcement manager for its lifetime, a fresh one unless one is given, and
 * the previous one after it
 */
class CScopedAnnouncementManager
{
public:
  explicit CScopedAnnouncementManager(std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> manager =
                                          std::make_shared<ANNOUNCEMENT::CAnnouncementManager>())
    : m_previous(CServiceBroker::GetAnnouncementManager())
  {
    CServiceBroker::RegisterAnnouncementManager(std::move(manager));
  }
  ~CScopedAnnouncementManager() { CServiceBroker::RegisterAnnouncementManager(m_previous); }

  CScopedAnnouncementManager(const CScopedAnnouncementManager&) = delete;
  CScopedAnnouncementManager& operator=(const CScopedAnnouncementManager&) = delete;

private:
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_previous;
};
