#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

#include "utils/JobManager.h"

#include "pvr/PVRTypes.h"

namespace PVR
{
  class CPVRSetRecordingOnChannelJob : public CJob
  {
  public:
    CPVRSetRecordingOnChannelJob(const CPVRChannelPtr &channel, bool bOnOff)
    : m_channel(channel), m_bOnOff(bOnOff) {}
    virtual ~CPVRSetRecordingOnChannelJob() = default;
    const char *GetType() const override { return "pvr-set-recording-on-channel"; }

    bool DoWork() override;
  private:
    CPVRChannelPtr m_channel;
    bool m_bOnOff;
  };

  class CPVRContinueLastChannelJob : public CJob
  {
  public:
    CPVRContinueLastChannelJob() = default;
    virtual ~CPVRContinueLastChannelJob() = default;
    const char *GetType() const override { return "pvr-continue-last-channel-job"; }

    bool DoWork() override;
  };

  class CPVREventlogJob : public CJob
  {
  public:
    CPVREventlogJob() = default;
    CPVREventlogJob(bool bNotifyUser, bool bError, const std::string &label, const std::string &msg, const std::string &icon);
    virtual ~CPVREventlogJob() = default;
    const char *GetType() const override { return "pvr-eventlog-job"; }

    void AddEvent(bool bNotifyUser, bool bError, const std::string &label, const std::string &msg, const std::string &icon);

    bool DoWork() override;
  private:
    struct Event
    {
      bool m_bNotifyUser;
      bool m_bError;
      std::string m_label;
      std::string m_msg;
      std::string m_icon;

      Event(bool bNotifyUser, bool bError, const std::string &label, const std::string &msg, const std::string &icon)
      : m_bNotifyUser(bNotifyUser), m_bError(bError), m_label(label), m_msg(msg), m_icon(icon) {}
    };

    std::vector<Event> m_events;
  };

} // namespace PVR
