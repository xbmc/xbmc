/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "events/EventLog.h"
#include "utils/Job.h"

#include <string>
#include <vector>

namespace PVR
{
class CPVREventLogJob : public CJob
{
public:
  CPVREventLogJob() = default;

  CPVREventLogJob(bool bNotifyUser,
                  EventLevel eLevel,
                  const std::string& label,
                  const std::string& msg,
                  const std::string& icon);

  ~CPVREventLogJob() override = default;
  const char* GetType() const override { return "pvr-eventlog-job"; }

  void AddEvent(bool bNotifyUser,
                EventLevel eLevel,
                const std::string& label,
                const std::string& msg,
                const std::string& icon);

  bool DoWork() override;

private:
  struct Event
  {
    bool m_bNotifyUser;
    EventLevel m_eLevel{EventLevel::Information};
    std::string m_label;
    std::string m_msg;
    std::string m_icon;

    Event(bool bNotifyUser,
          EventLevel elevel,
          const std::string& label,
          const std::string& msg,
          const std::string& icon)
      : m_bNotifyUser(bNotifyUser), m_eLevel(elevel), m_label(label), m_msg(msg), m_icon(icon)
    {
    }
  };

  std::vector<Event> m_events;
};
} // namespace PVR
