/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"
#include "XBDateTime.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"
#include "utils/Job.h"

#include <string>
#include <vector>

class CWakeOnAccess : private IJobCallback, public ISettingCallback, public ISettingsHandler
{
public:
  static CWakeOnAccess &GetInstance();

  bool WakeUpHost (const CURL& fileUrl);
  bool WakeUpHost (const std::string& hostName, const std::string& customMessage);

  void QueueMACDiscoveryForAllRemotes();

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingsLoaded() override;

  // struct to keep per host settings
  struct WakeUpEntry
  {
    explicit WakeUpEntry (bool isAwake = false);

    std::string host;
    std::string mac;
    CDateTimeSpan timeout;
    unsigned int wait_online1_sec; // initial wait
    unsigned int wait_online2_sec; // extended wait
    unsigned int wait_services_sec;

    unsigned short ping_port = 0; // where to ping
    unsigned short ping_mode = 0; // how to ping

    CDateTime nextWake;
    std::string upnpUuid; // empty unless upnpmode
    std::string friendlyName;
  };

private:
  CWakeOnAccess();
  std::string GetSettingFile();
  void LoadFromXML();
  void SaveToXML();

  void SetEnabled(bool enabled);
  bool IsEnabled() const { return m_enabled; }

  void QueueMACDiscoveryForHost(const std::string& host);
  void SaveMACDiscoveryResult(const std::string& host, const std::string& mac);

  typedef std::vector<WakeUpEntry> EntriesVector;
  EntriesVector m_entries;
  CCriticalSection m_entrylist_protect;
  bool FindOrTouchHostEntry(const std::string& hostName, bool upnpMode, WakeUpEntry& server);
  void TouchHostEntry(const std::string& hostName, bool upnpMode);

  unsigned int m_netinit_sec, m_netsettle_ms; //time to wait for network connection

  bool m_enabled = false;

  bool WakeUpHost(const std::string& hostName, const std::string& customMessage, bool upnpMode);
  bool WakeUpHost(const WakeUpEntry& server);

  std::vector<struct UPnPServer> m_UPnPServers; // list of wakeable upnp servers
};
