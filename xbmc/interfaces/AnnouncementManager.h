/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAnnouncer.h"
#include "interfaces/AnnouncementEvents.h"
#include "threads/Condition.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <list>
#include <string>
#include <unordered_map>

namespace ANNOUNCEMENT
{
  class CAnnouncementManager : public CThread
  {
  public:
    CAnnouncementManager();
    ~CAnnouncementManager() override;

    void Start();
    void Deinitialize();

    void AddAnnouncer(IAnnouncer *listener);
    void AddAnnouncer(IAnnouncer* listener, int flagMask);
    void RemoveAnnouncer(IAnnouncer *listener);

    /*!
     * \brief Announce an event. Its item is copied now, and listeners receive the copy.
     */
    void Announce(const Announcement& announcement);

    // The sender is not related to the application name.
    // Also it's part of Kodi's API - changing it will break
    // a big number of python addons and third party json consumers.
    static const std::string ANNOUNCEMENT_SENDER;

  protected:
    void Process() override;
    void DoAnnounce(const Announcement& announcement);

    std::list<Announcement> m_announcementQueue;
    CEvent m_queueEvent;

  private:
    CAnnouncementManager(const CAnnouncementManager&) = delete;
    CAnnouncementManager const& operator=(CAnnouncementManager const&) = delete;

    CCriticalSection m_announcersCritSection;
    CCriticalSection m_queueCritSection;
    std::unordered_map<IAnnouncer*, int> m_announcers;

    //! The announcer being called by DoAnnounce(), which runs without m_announcersCritSection.
    IAnnouncer* m_announcing{nullptr};
    XbmcThreads::ConditionVariable m_announced;
  };
}
