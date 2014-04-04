/*
 *      Copyright (C) 2013 Team XBMC
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

#include "URL.h"
#include "XBDateTime.h"
#include "utils/Job.h"
#include "settings/lib/ISettingsHandler.h"

class CWakeOnAccess : private IJobCallback, public ISettingsHandler
{
public:
  static CWakeOnAccess &Get();

  bool WakeUpHost (const CURL& fileUrl);
  bool WakeUpHost (const CStdString& hostName, const std::string& customMessage);

  void QueueMACDiscoveryForAllRemotes();

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  virtual void OnSettingsLoaded();
  virtual void OnSettingsSaved();

  // struct to keep per host settings
  struct WakeUpEntry
  {
    WakeUpEntry (bool isAwake = false);

    std::string host;
    std::string mac;
    CDateTimeSpan timeout;
    unsigned int wait_online1_sec; // initial wait
    unsigned int wait_online2_sec; // extended wait
    unsigned int wait_services_sec;

    unsigned short ping_port; // where to ping
    unsigned short ping_mode; // how to ping

    CDateTime nextWake;
  };

private:
  CWakeOnAccess();
  CStdString GetSettingFile();
  void LoadFromXML();
  void SaveToXML();

  void SetEnabled(bool enabled);
  bool IsEnabled() const { return m_enabled; }

  void QueueMACDiscoveryForHost(const CStdString& host);
  void SaveMACDiscoveryResult(const CStdString& host, const CStdString& mac);

  typedef std::vector<WakeUpEntry> EntriesVector;
  EntriesVector m_entries;
  CCriticalSection m_entrylist_protect;
  bool FindOrTouchHostEntry (const CStdString& hostName, WakeUpEntry& server);
  void TouchHostEntry (const CStdString& hostName);

  unsigned int m_netinit_sec, m_netsettle_ms; //time to wait for network connection

  bool m_enabled;

  bool WakeUpHost(const WakeUpEntry& server);
};
