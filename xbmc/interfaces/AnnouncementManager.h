/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAnnouncer.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/Variant.h"

#include <list>
#include <memory>
#include <vector>

class CFileItem;
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

    void Announce(AnnouncementFlag flag, const std::string& message);
    void Announce(AnnouncementFlag flag, const std::string& message, const CVariant& data);
    void Announce(AnnouncementFlag flag,
                  const std::string& message,
                  const std::shared_ptr<const CFileItem>& item);
    void Announce(AnnouncementFlag flag,
                  const std::string& message,
                  const std::shared_ptr<const CFileItem>& item,
                  const CVariant& data);

    void Announce(AnnouncementFlag flag, const std::string& sender, const std::string& message);
    void Announce(AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const CVariant& data);
    void Announce(AnnouncementFlag flag,
                  const std::string& sender,
                  const std::string& message,
                  const std::shared_ptr<const CFileItem>& item,
                  const CVariant& data);

    // The sender is not related to the application name.
    // Also it's part of Kodi's API - changing it will break
    // a big number of python addons and third party json consumers.
    static const std::string ANNOUNCEMENT_SENDER;

  protected:
    void Process() override;
    void DoAnnounce(AnnouncementFlag flag,
                    const std::string& sender,
                    const std::string& message,
                    const std::shared_ptr<CFileItem>& item,
                    const CVariant& data);
    void DoAnnounce(AnnouncementFlag flag,
                    const std::string& sender,
                    const std::string& message,
                    const CVariant& data);

    struct CAnnounceData
    {
      AnnouncementFlag flag;
      std::string sender;
      std::string message;
      std::shared_ptr<CFileItem> item;
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
