/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

#include "FileItem.h"
#include "addons/PVRClient.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "utils/JobManager.h"

#include "pvr/PVRTypes.h"

namespace PVR
{
  class CPVRPlayChannelOnStartupJob : public CJob
  {
  public:
    CPVRPlayChannelOnStartupJob() = default;
    ~CPVRPlayChannelOnStartupJob() override = default;
    const char *GetType() const override { return "pvr-play-channel-on-startup"; }

    bool DoWork() override;
  };

  class CPVRChannelEntryTimeoutJob : public CJob, public IJobCallback
  {
  public:
    explicit CPVRChannelEntryTimeoutJob(int timeout);
    ~CPVRChannelEntryTimeoutJob() override = default;
    const char *GetType() const override { return "pvr-channel-entry-timeout-job"; }
    void OnJobComplete(unsigned int iJobID, bool bSuccess, CJob *job) override {}

    bool DoWork() override;
  private:
    XbmcThreads::EndTime m_delayTimer;
  };

  class CPVRChannelInfoTimeoutJob : public CJob, public IJobCallback
  {
  public:
    CPVRChannelInfoTimeoutJob(int iTimeout);
    ~CPVRChannelInfoTimeoutJob() override = default;
    const char *GetType() const override { return "pvr-channel-info-timeout-job"; }
    void OnJobComplete(unsigned int iJobID, bool bSuccess, CJob *job) override {}

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

  class CPVRUpdateAddonsJob : public CJob
  {
  public:
    explicit CPVRUpdateAddonsJob(const std::string& changedAddonId) : m_changedAddonId(changedAddonId) {};
    ~CPVRUpdateAddonsJob() override = default;
    const char *GetType() const override { return "pvr-update-addons"; }

    bool DoWork() override;

  private:
    CPVRUpdateAddonsJob() = delete;

    std::string m_changedAddonId;
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
