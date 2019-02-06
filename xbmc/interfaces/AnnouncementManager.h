/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>
#include <list>

#include "IAnnouncer.h"
#include "FileItem.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "threads/Event.h"
#include "utils/Variant.h"

class CVariant;

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
    void RemoveAnnouncer(IAnnouncer *listener);

    void Announce(AnnouncementFlag flag, const char *sender, const char *message);
    void Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
    void Announce(AnnouncementFlag flag, const char *sender, const char *message,
        const std::shared_ptr<const CFileItem>& item);
    void Announce(AnnouncementFlag flag, const char *sender, const char *message,
        const std::shared_ptr<const CFileItem>& item, const CVariant &data);

  protected:
    void Process() override;
    void DoAnnounce(AnnouncementFlag flag, const char *sender, const char *message, CFileItemPtr item, const CVariant &data);
    void DoAnnounce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

    struct CAnnounceData
    {
      AnnouncementFlag flag;
      std::string sender;
      std::string message;
      CFileItemPtr item;
      CVariant data;
    };
    std::list<CAnnounceData> m_announcementQueue;
    CEvent m_queueEvent;

  private:
    CAnnouncementManager(const CAnnouncementManager&) = delete;
    CAnnouncementManager const& operator=(CAnnouncementManager const&) = delete;

    CCriticalSection m_announcersCritSection;
    CCriticalSection m_queueCritSection;
    std::vector<IAnnouncer *> m_announcers;
  };
}
