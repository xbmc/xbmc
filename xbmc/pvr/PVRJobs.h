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

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "addons/PVRClient.h"
#include "FileItem.h"
#include "pvr/PVRTypes.h"
#include "utils/JobManager.h"

namespace PVR
{
  class CPVRSetRecordingOnChannelJob : public CJob
  {
  public:
    CPVRSetRecordingOnChannelJob(const CPVRChannelPtr &channel, bool bOnOff)
    : m_channel(channel), m_bOnOff(bOnOff) {}
    ~CPVRSetRecordingOnChannelJob() override = default;
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
    ~CPVRContinueLastChannelJob() override = default;
    const char *GetType() const override { return "pvr-continue-last-channel-job"; }

    bool DoWork() override;
  };

  class CPVRChannelEntryTimeoutJob : public CJob, public IJobCallback
  {
  public:
    CPVRChannelEntryTimeoutJob(int timeout);
    ~CPVRChannelEntryTimeoutJob() override = default;
    const char *GetType() const override { return "pvr-channel-entry-timeout-job"; }
    void OnJobComplete(unsigned int jobID, bool success, CJob *job) override { }

    bool DoWork() override;
  private:
    XbmcThreads::EndTime m_delayTimer;
  };

  class CPVREventlogJob : public CJob
  {
  public:
    CPVREventlogJob() = default;
    CPVREventlogJob(bool bNotifyUser, bool bError, const std::string &label, const std::string &msg, const std::string &icon);
    ~CPVREventlogJob() override = default;
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

  class CPVRStartupJob : public CJob
  {
  public:
    CPVRStartupJob(void) = default;
    ~CPVRStartupJob() override = default;
    const char *GetType() const override { return "pvr-startup"; }

    bool DoWork() override;
  };

  class CPVREpgsCreateJob : public CJob
  {
  public:
    CPVREpgsCreateJob(void) = default;
    ~CPVREpgsCreateJob() override = default;
    const char *GetType() const override { return "pvr-create-epgs"; }

    bool DoWork() override;
  };

  class CPVRRecordingsUpdateJob : public CJob
  {
  public:
    CPVRRecordingsUpdateJob(void) = default;
    ~CPVRRecordingsUpdateJob() override = default;
    const char *GetType() const override { return "pvr-update-recordings"; }

    bool DoWork() override;
  };

  class CPVRTimersUpdateJob : public CJob
  {
  public:
    CPVRTimersUpdateJob(void) = default;
    ~CPVRTimersUpdateJob() override = default;
    const char *GetType() const override { return "pvr-update-timers"; }

    bool DoWork() override;
  };

  class CPVRChannelsUpdateJob : public CJob
  {
  public:
    CPVRChannelsUpdateJob(void) = default;
    ~CPVRChannelsUpdateJob() override = default;
    const char *GetType() const override { return "pvr-update-channels"; }

    bool DoWork() override;
  };

  class CPVRChannelGroupsUpdateJob : public CJob
  {
  public:
    CPVRChannelGroupsUpdateJob(void) = default;
    ~CPVRChannelGroupsUpdateJob() override = default;
    const char *GetType() const override { return "pvr-update-channelgroups"; }

    bool DoWork() override;
  };

  class CPVRSearchMissingChannelIconsJob : public CJob
  {
  public:
    CPVRSearchMissingChannelIconsJob(void) = default;
    ~CPVRSearchMissingChannelIconsJob() override = default;
    const char *GetType() const override { return "pvr-search-missing-channel-icons"; }

    bool DoWork() override;
  };

  class CPVRClientConnectionJob : public CJob
  {
  public:
    CPVRClientConnectionJob(CPVRClient *client, std::string connectString, PVR_CONNECTION_STATE state, std::string message)
    : m_client(client), m_connectString(connectString), m_state(state), m_message(message) {}
    ~CPVRClientConnectionJob() override = default;
    const char *GetType() const override { return "pvr-client-connection"; }

    bool DoWork() override;
  private:
    CPVRClient *m_client;
    std::string m_connectString;
    PVR_CONNECTION_STATE m_state;
    std::string m_message;
  };

} // namespace PVR
