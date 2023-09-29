/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WakeOnAccess.h"

#include "DNSNameCache.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "network/Network.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <limits.h>
#include <mutex>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#include <Platinum/Source/Platinum/Platinum.h>
#endif

#define DEFAULT_NETWORK_INIT_SEC      (20)   // wait 20 sec for network after startup or resume
#define DEFAULT_NETWORK_SETTLE_MS     (500)  // require 500ms of consistent network availability before trusting it

#define DEFAULT_TIMEOUT_SEC (5*60)           // at least 5 minutes between each magic packets
#define DEFAULT_WAIT_FOR_ONLINE_SEC_1 (40)   // wait at 40 seconds after sending magic packet
#define DEFAULT_WAIT_FOR_ONLINE_SEC_2 (40)   // same for extended wait
#define DEFAULT_WAIT_FOR_SERVICES_SEC (5)    // wait 5 seconds after host go online to launch file sharing daemons

using namespace std::chrono_literals;

static CDateTime upnpInitReady;

static int GetTotalSeconds(const CDateTimeSpan& ts)
{
  int hours = ts.GetHours() + ts.GetDays() * 24;
  int minutes = ts.GetMinutes() + hours * 60;
  return ts.GetSeconds() + minutes * 60;
}

static unsigned long HostToIP(const std::string& host)
{
  std::string ip;
  CDNSNameCache::Lookup(host, ip);
  return inet_addr(ip.c_str());
}

#define LOCALIZED(id) g_localizeStrings.Get(id)

static void ShowDiscoveryMessage(const char* function, const char* server_name, bool new_entry)
{
  std::string message;

  if (new_entry)
  {
    CLog::Log(LOGINFO, "{} - Create new entry for host '{}'", function, server_name);
    message = StringUtils::Format(LOCALIZED(13035), server_name);
  }
  else
  {
    CLog::Log(LOGINFO, "{} - Update existing entry for host '{}'", function, server_name);
    message = StringUtils::Format(LOCALIZED(13034), server_name);
  }
  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, LOCALIZED(13033), message, 4000, true, 3000);
}

struct UPnPServer
{
  UPnPServer() : m_nextWake(CDateTime::GetCurrentDateTime()) {}
  bool operator == (const UPnPServer& server) const { return server.m_uuid == m_uuid; }
  bool operator != (const UPnPServer& server) const { return !(*this == server); }
  bool operator == (const std::string& server_uuid) const { return server_uuid == m_uuid; }
  bool operator != (const std::string& server_uuid) const { return !(*this == server_uuid); }
  std::string m_name;
  std::string m_uuid;
  std::string m_mac;
  CDateTime m_nextWake;
};

static UPnPServer* LookupUPnPServer(std::vector<UPnPServer>& list, const std::string& uuid)
{
  auto serverIt = find(list.begin(), list.end(), uuid);

  return serverIt != list.end() ? &(*serverIt) : nullptr;
}

static void AddOrUpdateUPnPServer(std::vector<UPnPServer>& list, const UPnPServer& server)
{
  auto serverIt = find(list.begin(), list.end(), server);

  bool addNewEntry = serverIt == list.end();

  if (addNewEntry)
	  list.push_back(server); // add server
  else
    *serverIt = server; // update existing server

  ShowDiscoveryMessage(__FUNCTION__, server.m_name.c_str(), addNewEntry);
}

static void AddMatchingUPnPServers(std::vector<UPnPServer>& list, const std::string& host, const std::string& mac, const CDateTimeSpan& wakeupDelay)
{
#ifdef HAS_UPNP
  while (CDateTime::GetCurrentDateTime() < upnpInitReady)
    KODI::TIME::Sleep(1s);

  PLT_SyncMediaBrowser* browser = UPNP::CUPnP::GetInstance()->m_MediaBrowser;

  if (browser)
  {
    UPnPServer server;
    server.m_nextWake += wakeupDelay;

    for (NPT_List<PLT_DeviceDataReference>::Iterator device = browser->GetMediaServers().GetFirstItem(); device; ++device)
    {
      if (host == (const char*) (*device)->GetURLBase().GetHost())
      {
        server.m_name = (*device)->GetFriendlyName();
        server.m_uuid = (*device)->GetUUID();
        server.m_mac = mac;

        AddOrUpdateUPnPServer(list, server);
      }
    }
  }
#endif
}

static std::string LookupUPnPHost(const std::string& uuid)
{
#ifdef HAS_UPNP
  UPNP::CUPnP* upnp = UPNP::CUPnP::GetInstance();

  if (!upnp->IsClientStarted())
  {
    upnp->StartClient();

    upnpInitReady = CDateTime::GetCurrentDateTime() + CDateTimeSpan(0, 0, 0, 10);
  }

  PLT_SyncMediaBrowser* browser = upnp->m_MediaBrowser;

  PLT_DeviceDataReference device;

  if (browser && NPT_SUCCEEDED(browser->FindServer(uuid.c_str(), device)) && !device.IsNull())
    return (const char*)device->GetURLBase().GetHost();
#endif

  return "";
}

CWakeOnAccess::WakeUpEntry::WakeUpEntry(bool isAwake)
  : timeout(0, 0, 0, DEFAULT_TIMEOUT_SEC),
    wait_online1_sec(DEFAULT_WAIT_FOR_ONLINE_SEC_1),
    wait_online2_sec(DEFAULT_WAIT_FOR_ONLINE_SEC_2),
    wait_services_sec(DEFAULT_WAIT_FOR_SERVICES_SEC),
    nextWake(CDateTime::GetCurrentDateTime())
{
  if (isAwake)
    nextWake += timeout;
}

//**

class CMACDiscoveryJob : public CJob
{
public:
  explicit CMACDiscoveryJob(const std::string& host) : m_host(host) {}

  bool DoWork() override;

  const std::string& GetMAC() const { return m_macAddress; }
  const std::string& GetHost() const { return m_host; }

private:
  std::string m_macAddress;
  std::string m_host;
};

bool CMACDiscoveryJob::DoWork()
{
  unsigned long ipAddress = HostToIP(m_host);

  if (ipAddress == INADDR_NONE)
  {
    CLog::Log(LOGERROR, "{} - can't determine ip of '{}'", __FUNCTION__, m_host);
    return false;
  }

  const std::vector<CNetworkInterface*>& ifaces = CServiceBroker::GetNetwork().GetInterfaceList();
  for (const auto& it : ifaces)
  {
    if (it->GetHostMacAddress(ipAddress, m_macAddress))
      return true;
  }

  return false;
}

//**

class WaitCondition
{
public:
  virtual ~WaitCondition() = default;
  virtual bool SuccessWaiting () const { return false; }
};

//

class NestDetect
{
public:
  NestDetect() : m_gui_thread(CServiceBroker::GetAppMessenger()->IsProcessThread())
  {
    if (m_gui_thread)
      ++m_nest;
  }
  ~NestDetect()
  {
    if (m_gui_thread)
      m_nest--;
  }
  static int Level()
  {
    return m_nest;
  }
  bool IsNested() const
  {
    return m_gui_thread && m_nest > 1;
  }

private:
  static int m_nest;
  const bool m_gui_thread;
};
int NestDetect::m_nest = 0;

//

class ProgressDialogHelper
{
public:
  explicit ProgressDialogHelper(const std::string& heading)
  {
    if (CServiceBroker::GetAppMessenger()->IsProcessThread())
    {
      CGUIComponent *gui = CServiceBroker::GetGUI();
      if (gui)
        m_dialog = gui->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
    }

    if (m_dialog)
    {
      m_dialog->SetHeading(CVariant{heading});
      m_dialog->SetLine(0, CVariant{""});
      m_dialog->SetLine(1, CVariant{""});
      m_dialog->SetLine(2, CVariant{""});
    }
  }
  ~ProgressDialogHelper ()
  {
    if (m_dialog)
      m_dialog->Close();
  }

  bool HasDialog() const { return m_dialog != 0; }

  enum wait_result { TimedOut, Canceled, Success };

  wait_result ShowAndWait (const WaitCondition& waitObj, unsigned timeOutSec, const std::string& line1)
  {
    auto timeOutMs = std::chrono::milliseconds(timeOutSec * 1000);

    if (m_dialog)
    {
      m_dialog->SetLine(0, CVariant{line1});

      m_dialog->SetPercentage(1); // avoid flickering by starting at 1% ..
    }

    XbmcThreads::EndTime<> end_time(timeOutMs);

    while (!end_time.IsTimePast())
    {
      if (waitObj.SuccessWaiting())
        return Success;

      if (m_dialog)
      {
        if (!m_dialog->IsActive())
          m_dialog->Open();

        if (m_dialog->IsCanceled())
          return Canceled;

        m_dialog->Progress();

        auto ms_passed = timeOutMs - end_time.GetTimeLeft();

        int percentage = (ms_passed.count() * 100) / timeOutMs.count();
        m_dialog->SetPercentage(std::max(percentage, 1)); // avoid flickering , keep minimum 1%
      }

      KODI::TIME::Sleep(m_dialog ? 20ms : 200ms);
    }

    return TimedOut;
  }

private:
  CGUIDialogProgress* m_dialog = 0;
};

class NetworkStartWaiter : public WaitCondition
{
public:
  NetworkStartWaiter (unsigned settle_time_ms, const std::string& host) : m_settle_time_ms (settle_time_ms), m_host(host)
  {
  }
  bool SuccessWaiting () const override
  {
    unsigned long address = ntohl(HostToIP(m_host));
    bool online = CServiceBroker::GetNetwork().HasInterfaceForIP(address);

    if (!online) // setup endtime so we dont return true until network is consistently connected
      m_end.Set(std::chrono::milliseconds(m_settle_time_ms));

    return online && m_end.IsTimePast();
  }
private:
  mutable XbmcThreads::EndTime<> m_end;
  unsigned m_settle_time_ms;
  const std::string m_host;
};

class PingResponseWaiter : public WaitCondition, private IJobCallback
{
public:
  PingResponseWaiter(bool async, const CWakeOnAccess::WakeUpEntry& server) : m_server(server)
  {
    if (async)
    {
      CJob* job = new CHostProberJob(server);
      m_jobId = CServiceBroker::GetJobManager()->AddJob(job, this);
    }
  }
  ~PingResponseWaiter() override { CServiceBroker::GetJobManager()->CancelJob(m_jobId); }
  bool SuccessWaiting () const override
  {
    return m_jobId ? m_hostOnline : Ping(m_server);
  }

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override
  {
    m_hostOnline = success;
  }

  static bool Ping(const CWakeOnAccess::WakeUpEntry& server, unsigned timeOutMs = 2000)
  {
    if (server.upnpUuid.empty())
    {
      unsigned long dst_ip = HostToIP(server.host);

      return CServiceBroker::GetNetwork().PingHost(dst_ip, server.ping_port, timeOutMs, server.ping_mode & 1);
    }
    else // upnp mode
    {
      std::string host = LookupUPnPHost(server.upnpUuid);

      if (host.empty())
      {
        KODI::TIME::Sleep(std::chrono::milliseconds(timeOutMs));

        host = LookupUPnPHost(server.upnpUuid);
      }

      return !host.empty();
    }
  }

private:
  class CHostProberJob : public CJob
  {
    public:
      explicit CHostProberJob(const CWakeOnAccess::WakeUpEntry& server) : m_server (server) {}

      bool DoWork() override
      {
        while (!ShouldCancel(0,0))
        {
          if (PingResponseWaiter::Ping(m_server))
            return true;
        }
        return false;
      }

    private:
      const CWakeOnAccess::WakeUpEntry& m_server;
  };

  const CWakeOnAccess::WakeUpEntry& m_server;
  unsigned int m_jobId = 0;
  bool m_hostOnline = false;
};

//

CWakeOnAccess::CWakeOnAccess()
  : m_netinit_sec(DEFAULT_NETWORK_INIT_SEC)    // wait for network to connect
  , m_netsettle_ms(DEFAULT_NETWORK_SETTLE_MS)  // wait for network to settle
{
}

CWakeOnAccess &CWakeOnAccess::GetInstance()
{
  static CWakeOnAccess sWakeOnAccess;
  return sWakeOnAccess;
}

bool CWakeOnAccess::WakeUpHost(const CURL& url)
{
  const std::string& hostName = url.GetHostName();

  if (!hostName.empty())
    return WakeUpHost(hostName, url.Get(), url.IsProtocol("upnp"));

  return true;
}

bool CWakeOnAccess::WakeUpHost(const std::string& hostName, const std::string& customMessage)
{
  return WakeUpHost(hostName, customMessage, false);
}

bool CWakeOnAccess::WakeUpHost(const std::string& hostName, const std::string& customMessage, bool upnpMode)
{
  if (!IsEnabled())
    return true; // bail if feature is turned off

  WakeUpEntry server;

  if (FindOrTouchHostEntry(hostName, upnpMode, server))
  {
    CLog::Log(LOGINFO, "WakeOnAccess [{}] triggered by accessing : {}", server.friendlyName,
              customMessage);

    NestDetect nesting ; // detect recursive calls on gui thread..

    if (nesting.IsNested()) // we might get in trouble if it gets called back in loop
      CLog::Log(LOGWARNING, "WakeOnAccess recursively called on gui-thread [{}]",
                NestDetect::Level());

    bool ret = WakeUpHost(server);

    if (!ret) // extra log if we fail for some reason
      CLog::Log(LOGWARNING, "WakeOnAccess failed to bring up [{}] - there may be trouble ahead !",
                server.friendlyName);

    TouchHostEntry(hostName, upnpMode);

    return ret;
  }
  return true;
}

bool CWakeOnAccess::WakeUpHost(const WakeUpEntry& server)
{
  std::string heading = StringUtils::Format(LOCALIZED(13027), server.friendlyName);

  ProgressDialogHelper dlg (heading);

  {
    NetworkStartWaiter waitObj (m_netsettle_ms, server.host); // wait until network connected before sending wake-on-lan

    if (dlg.ShowAndWait (waitObj, m_netinit_sec, LOCALIZED(13028)) != ProgressDialogHelper::Success)
    {
      if (CServiceBroker::GetNetwork().IsConnected() && HostToIP(server.host) == INADDR_NONE)
      {
        // network connected (at least one interface) but dns-lookup failed (host by name, not ip-address), so dont abort yet
        CLog::Log(LOGWARNING, "WakeOnAccess timeout/cancel while waiting for network (proceeding anyway)");
      }
      else
      {
        CLog::Log(LOGINFO, "WakeOnAccess timeout/cancel while waiting for network");
        return false; // timedout or canceled ; give up
      }
    }
  }

  if (PingResponseWaiter::Ping(server, 500)) // quick ping with short timeout to not block too long
  {
    CLog::Log(LOGINFO, "WakeOnAccess success exit, server already running");
    return true;
  }

  if (!CServiceBroker::GetNetwork().WakeOnLan(server.mac.c_str()))
  {
    CLog::Log(LOGERROR,"WakeOnAccess failed to send. (Is it blocked by firewall?)");

    const auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (CServiceBroker::GetAppMessenger()->IsProcessThread() || !appPlayer->IsPlaying())
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, heading, LOCALIZED(13029));
    return false;
  }

  {
    PingResponseWaiter waitObj (dlg.HasDialog(), server); // wait for ping response ..

    ProgressDialogHelper::wait_result
      result = dlg.ShowAndWait (waitObj, server.wait_online1_sec, LOCALIZED(13030));

    if (result == ProgressDialogHelper::TimedOut)
      result = dlg.ShowAndWait (waitObj, server.wait_online2_sec, LOCALIZED(13031));

    if (result != ProgressDialogHelper::Success)
    {
      CLog::Log(LOGINFO, "WakeOnAccess timeout/cancel while waiting for response");
      return false; // timedout or canceled
    }
  }

  // we have ping response ; just add extra wait-for-services before returning if requested

  {
    WaitCondition waitObj ; // wait uninterruptible fixed time for services ..

    dlg.ShowAndWait (waitObj, server.wait_services_sec, LOCALIZED(13032));

    CLog::Log(LOGINFO, "WakeOnAccess sequence completed, server started");
  }
  return true;
}

bool CWakeOnAccess::FindOrTouchHostEntry(const std::string& hostName, bool upnpMode, WakeUpEntry& result)
{
  std::unique_lock<CCriticalSection> lock(m_entrylist_protect);

  bool need_wakeup = false;

  UPnPServer* upnp = upnpMode ? LookupUPnPServer(m_UPnPServers, hostName) : nullptr;

  for (auto& server : m_entries)
  {
    if (upnp ? StringUtils::EqualsNoCase(upnp->m_mac, server.mac) : StringUtils::EqualsNoCase(hostName, server.host))
    {
      CDateTime now = CDateTime::GetCurrentDateTime();

      if (now >= (upnp ? upnp->m_nextWake : server.nextWake))
      {
        result = server;

        result.friendlyName = upnp ? upnp->m_name : server.host;

        if (upnp)
          result.upnpUuid = upnp->m_uuid;

        need_wakeup = true;
      }
      else // 'touch' next wakeup time
      {
        server.nextWake = now + server.timeout;

        if (upnp)
          upnp->m_nextWake = server.nextWake;
      }

      break;
    }
  }

  return need_wakeup;
}

void CWakeOnAccess::TouchHostEntry(const std::string& hostName, bool upnpMode)
{
  std::unique_lock<CCriticalSection> lock(m_entrylist_protect);

  UPnPServer* upnp = upnpMode ? LookupUPnPServer(m_UPnPServers, hostName) : nullptr;

  for (auto& server : m_entries)
  {
    if (upnp ? StringUtils::EqualsNoCase(upnp->m_mac, server.mac) : StringUtils::EqualsNoCase(hostName, server.host))
    {
      server.nextWake = CDateTime::GetCurrentDateTime() + server.timeout;

      if (upnp)
        upnp->m_nextWake = server.nextWake;

      return;
    }
  }
}

static void AddHost (const std::string& host, std::vector<std::string>& hosts)
{
  for (const auto& it : hosts)
    if (StringUtils::EqualsNoCase(host, it))
      return; // already there ..

  if (!host.empty())
    hosts.push_back(host);
}

static void AddHostFromDatabase(const DatabaseSettings& setting, std::vector<std::string>& hosts)
{
  if (StringUtils::EqualsNoCase(setting.type, "mysql"))
    AddHost(setting.host, hosts);
}

void CWakeOnAccess::QueueMACDiscoveryForHost(const std::string& host)
{
  if (IsEnabled())
  {
    if (URIUtils::IsHostOnLAN(host, LanCheckMode::ANY_PRIVATE_SUBNET))
      CServiceBroker::GetJobManager()->AddJob(new CMACDiscoveryJob(host), this);
    else
      CLog::Log(LOGINFO, "{} - skip Mac discovery for non-local host '{}'", __FUNCTION__, host);
  }
}

static void AddHostsFromMediaSource(const CMediaSource& source, std::vector<std::string>& hosts)
{
  for (const auto& it : source.vecPaths)
  {
    CURL url(it);

    std::string host_name = url.GetHostName();

    if (url.IsProtocol("upnp"))
      host_name = LookupUPnPHost(host_name);

    AddHost(host_name, hosts);
  }
}

static void AddHostsFromVecSource(const VECSOURCES& sources, std::vector<std::string>& hosts)
{
  for (const auto& it : sources)
    AddHostsFromMediaSource(it, hosts);
}

static void AddHostsFromVecSource(const VECSOURCES* sources, std::vector<std::string>& hosts)
{
  if (sources)
    AddHostsFromVecSource(*sources, hosts);
}

void CWakeOnAccess::QueueMACDiscoveryForAllRemotes()
{
  std::vector<std::string> hosts;

  // add media sources
  CMediaSourceSettings& ms = CMediaSourceSettings::GetInstance();

  AddHostsFromVecSource(ms.GetSources("video"), hosts);
  AddHostsFromVecSource(ms.GetSources("music"), hosts);
  AddHostsFromVecSource(ms.GetSources("files"), hosts);
  AddHostsFromVecSource(ms.GetSources("pictures"), hosts);
  AddHostsFromVecSource(ms.GetSources("programs"), hosts);

  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  // add mysql servers
  AddHostFromDatabase(advancedSettings->m_databaseVideo, hosts);
  AddHostFromDatabase(advancedSettings->m_databaseMusic, hosts);
  AddHostFromDatabase(advancedSettings->m_databaseEpg, hosts);
  AddHostFromDatabase(advancedSettings->m_databaseTV, hosts);

  // add from path substitutions ..
  for (const auto& pathPair : advancedSettings->m_pathSubstitutions)
  {
    CURL url(pathPair.second);
    AddHost (url.GetHostName(), hosts);
  }

  for (const std::string& host : hosts)
    QueueMACDiscoveryForHost(host);
}

void CWakeOnAccess::SaveMACDiscoveryResult(const std::string& host, const std::string& mac)
{
  CLog::Log(LOGINFO, "{} - Mac discovered for host '{}' -> '{}'", __FUNCTION__, host, mac);

  for (auto& i : m_entries)
  {
    if (StringUtils::EqualsNoCase(host, i.host))
    {
      i.mac = mac;
      ShowDiscoveryMessage(__FUNCTION__, host.c_str(), false);

      AddMatchingUPnPServers(m_UPnPServers, host, mac, i.timeout);
      SaveToXML();
      return;
    }
  }

  // not found entry to update - create using default values
  WakeUpEntry entry (true);
  entry.host = host;
  entry.mac  = mac;
  m_entries.push_back(entry);
  ShowDiscoveryMessage(__FUNCTION__, host.c_str(), true);

  AddMatchingUPnPServers(m_UPnPServers, host, mac, entry.timeout);
  SaveToXML();
}

void CWakeOnAccess::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CMACDiscoveryJob* discoverJob = static_cast<CMACDiscoveryJob*>(job);

  const std::string& host = discoverJob->GetHost();
  const std::string& mac = discoverJob->GetMAC();

  if (success)
  {
    std::unique_lock<CCriticalSection> lock(m_entrylist_protect);

    SaveMACDiscoveryResult(host, mac);
  }
  else
  {
    CLog::Log(LOGERROR, "{} - Mac discovery failed for host '{}'", __FUNCTION__, host);

    if (IsEnabled())
    {
      const std::string& heading = LOCALIZED(13033);
      std::string message = StringUtils::Format(LOCALIZED(13036), host);
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, heading, message, 4000, true, 3000);
    }
  }
}

void CWakeOnAccess::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS)
  {
    bool enabled = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();

    SetEnabled(enabled);

    if (enabled)
      QueueMACDiscoveryForAllRemotes();
  }
}

std::string CWakeOnAccess::GetSettingFile()
{
  return CSpecialProtocol::TranslatePath("special://profile/wakeonlan.xml");
}

void CWakeOnAccess::OnSettingsLoaded()
{
  std::unique_lock<CCriticalSection> lock(m_entrylist_protect);

  LoadFromXML();
}

void CWakeOnAccess::SetEnabled(bool enabled)
{
  m_enabled = enabled;

  CLog::Log(LOGINFO, "WakeOnAccess - Enabled:{}", m_enabled ? "TRUE" : "FALSE");
}

void CWakeOnAccess::LoadFromXML()
{
  bool enabled = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_POWERMANAGEMENT_WAKEONACCESS);

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(GetSettingFile()))
  {
    if (enabled)
      CLog::LogF(LOGINFO, "unable to load:{}", GetSettingFile());
    return;
  }

  auto* rootElement = xmlDoc.RootElement();
  if (StringUtils::CompareNoCase(rootElement->Value(), "onaccesswakeup"))
  {
    CLog::LogF(LOGERROR, "XML file {} doesn't contain <onaccesswakeup>", GetSettingFile());
    return;
  }

  m_entries.clear();

  CLog::Log(LOGINFO, "WakeOnAccess - Load settings :");

  SetEnabled(enabled);

  int tmp;
  if (XMLUtils::GetInt(rootElement, "netinittimeout", tmp, 0, 5 * 60))
    m_netinit_sec = tmp;
  CLog::Log(LOGINFO, "  -Network init timeout : [{}] sec", m_netinit_sec);

  if (XMLUtils::GetInt(rootElement, "netsettletime", tmp, 0, 5 * 1000))
    m_netsettle_ms = tmp;
  CLog::Log(LOGINFO, "  -Network settle time  : [{}] ms", m_netsettle_ms);

  const auto* pWakeUp = rootElement->FirstChildElement("wakeup");
  while (pWakeUp != nullptr)
  {
    WakeUpEntry entry;

    std::string strtmp;
    if (XMLUtils::GetString(pWakeUp, "host", strtmp))
      entry.host = strtmp;

    if (XMLUtils::GetString(pWakeUp, "mac", strtmp))
      entry.mac = strtmp;

    if (entry.host.empty())
      CLog::LogF(LOGERROR, "Missing <host> tag or it's empty");
    else if (entry.mac.empty())
      CLog::Log(LOGERROR, "Missing <mac> tag or it's empty");
    else
    {
      if (XMLUtils::GetInt(pWakeUp, "pingport", tmp, 0, USHRT_MAX))
        entry.ping_port = (unsigned short) tmp;

      if (XMLUtils::GetInt(pWakeUp, "pingmode", tmp, 0, USHRT_MAX))
        entry.ping_mode = (unsigned short) tmp;

      if (XMLUtils::GetInt(pWakeUp, "timeout", tmp, 10, 12 * 60 * 60))
        entry.timeout.SetDateTimeSpan (0, 0, 0, tmp);

      if (XMLUtils::GetInt(pWakeUp, "waitonline", tmp, 0, 10 * 60)) // max 10 minutes
        entry.wait_online1_sec = tmp;

      if (XMLUtils::GetInt(pWakeUp, "waitonline2", tmp, 0, 10 * 60)) // max 10 minutes
        entry.wait_online2_sec = tmp;

      if (XMLUtils::GetInt(pWakeUp, "waitservices", tmp, 0, 5 * 60)) // max 5 minutes
        entry.wait_services_sec = tmp;

      CLog::Log(LOGINFO, "  Registering wakeup entry:");
      CLog::Log(LOGINFO, "    HostName        : {}", entry.host);
      CLog::Log(LOGINFO, "    MacAddress      : {}", entry.mac);
      CLog::Log(LOGINFO, "    PingPort        : {}", entry.ping_port);
      CLog::Log(LOGINFO, "    PingMode        : {}", entry.ping_mode);
      CLog::Log(LOGINFO, "    Timeout         : {} (sec)", GetTotalSeconds(entry.timeout));
      CLog::Log(LOGINFO, "    WaitForOnline   : {} (sec)", entry.wait_online1_sec);
      CLog::Log(LOGINFO, "    WaitForOnlineEx : {} (sec)", entry.wait_online2_sec);
      CLog::Log(LOGINFO, "    WaitForServices : {} (sec)", entry.wait_services_sec);

      m_entries.push_back(entry);
    }

    pWakeUp = pWakeUp->NextSiblingElement("wakeup"); // get next one
  }

  // load upnp server map
  m_UPnPServers.clear();

  const auto* pUPnPNode = rootElement->FirstChildElement("upnp_map");
  while (pUPnPNode != nullptr)
  {
    UPnPServer server;

    XMLUtils::GetString(pUPnPNode, "name", server.m_name);
    XMLUtils::GetString(pUPnPNode, "uuid", server.m_uuid);
    XMLUtils::GetString(pUPnPNode, "mac", server.m_mac);

    if (server.m_name.empty())
      server.m_name = server.m_uuid;

    if (server.m_uuid.empty() || server.m_mac.empty())
      CLog::LogF(LOGERROR, "Missing or empty <upnp_map> entry");
    else
    {
      CLog::Log(LOGINFO, "  Registering upnp_map entry [{} : {}] -> [{}]", server.m_name,
                server.m_uuid, server.m_mac);

      m_UPnPServers.push_back(server);
    }

    pUPnPNode = pUPnPNode->NextSiblingElement("upnp_map"); // get next one
  }
}

void CWakeOnAccess::SaveToXML()
{
  CXBMCTinyXML2 xmlDoc;
  auto xmlRootElement = xmlDoc.NewElement("onaccesswakeup");
  if (xmlRootElement == nullptr)
    return;

  auto* root = xmlDoc.InsertEndChild(xmlRootElement);
  if (root == nullptr)
    return;

  XMLUtils::SetInt(root, "netinittimeout", m_netinit_sec);
  XMLUtils::SetInt(root, "netsettletime", m_netsettle_ms);

  for (const auto& i : m_entries)
  {
    auto* xmlSetting = xmlDoc.NewElement("wakeup");
    if (xmlSetting == nullptr)
      continue;
    auto* pWakeUpNode = root->InsertEndChild(xmlSetting);
    if (pWakeUpNode != nullptr)
    {
      XMLUtils::SetString(pWakeUpNode, "host", i.host);
      XMLUtils::SetString(pWakeUpNode, "mac", i.mac);
      XMLUtils::SetInt(pWakeUpNode, "pingport", i.ping_port);
      XMLUtils::SetInt(pWakeUpNode, "pingmode", i.ping_mode);
      XMLUtils::SetInt(pWakeUpNode, "timeout", GetTotalSeconds(i.timeout));
      XMLUtils::SetInt(pWakeUpNode, "waitonline", i.wait_online1_sec);
      XMLUtils::SetInt(pWakeUpNode, "waitonline2", i.wait_online2_sec);
      XMLUtils::SetInt(pWakeUpNode, "waitservices", i.wait_services_sec);
    }
  }

  for (const auto& upnp : m_UPnPServers)
  {
    auto* xmlSetting = xmlDoc.NewElement("upnp_map");
    if (xmlSetting == nullptr)
      continue;
    auto* pUPnPNode = root->InsertEndChild(xmlSetting);
    if (pUPnPNode != nullptr)
    {
      XMLUtils::SetString(pUPnPNode, "name", upnp.m_name);
      XMLUtils::SetString(pUPnPNode, "uuid", upnp.m_uuid);
      XMLUtils::SetString(pUPnPNode, "mac", upnp.m_mac);
    }
  }

  xmlDoc.SaveFile(GetSettingFile());
}
