/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Network.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/Network.h"
#include "network/DNSNameCache.h"
#include "network/Network.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_Network::Init(AddonGlobalInterface *addonInterface)
{
  addonInterface->toKodi->kodi_network = new AddonToKodiFuncTable_kodi_network();

  addonInterface->toKodi->kodi_network->wake_on_lan = wake_on_lan;
  addonInterface->toKodi->kodi_network->get_ip_address = get_ip_address;
  addonInterface->toKodi->kodi_network->get_hostname = get_hostname;
  addonInterface->toKodi->kodi_network->get_user_agent = get_user_agent;
  addonInterface->toKodi->kodi_network->is_local_host = is_local_host;
  addonInterface->toKodi->kodi_network->is_host_on_lan = is_host_on_lan;
  addonInterface->toKodi->kodi_network->dns_lookup = dns_lookup;
  addonInterface->toKodi->kodi_network->url_encode = url_encode;
}

void Interface_Network::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi) /* <-- needed as long as the old addon way is used */
  {
    delete addonInterface->toKodi->kodi_network;
    addonInterface->toKodi->kodi_network = nullptr;
  }
}

bool Interface_Network::wake_on_lan(void* kodiBase, const char* mac)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !mac)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', mac='{}')", kodiBase,
               static_cast<const void*>(mac));
    return false;
  }

  return CServiceBroker::GetNetwork().WakeOnLan(mac);
}

char* Interface_Network::get_ip_address(void* kodiBase)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}')", kodiBase);
    return nullptr;
  }

  std::string titleIP;
  CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
  if (iface)
    titleIP = iface->GetCurrentIPAddress();
  else
    titleIP = "127.0.0.1";

  if (titleIP.empty())
    return nullptr;

  return strdup(titleIP.c_str());
}

char* Interface_Network::get_hostname(void* kodiBase)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}')", kodiBase);
    return nullptr;
  }

  std::string hostname;
  if (!CServiceBroker::GetNetwork().GetHostName(hostname))
    return nullptr;

  if (hostname.empty())
    return nullptr;

  return strdup(hostname.c_str());
}

char* Interface_Network::get_user_agent(void* kodiBase)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}')", kodiBase);
    return nullptr;
  }

  const std::string string{CSysInfo::GetUserAgent()};
  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

bool Interface_Network::is_local_host(void* kodiBase, const char* hostname)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !hostname)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', hostname='{}')", kodiBase,
               static_cast<const void*>(hostname));
    return false;
  }

  return CServiceBroker::GetNetwork().IsLocalHost(hostname);
}

bool Interface_Network::is_host_on_lan(void* kodiBase, const char* hostname, bool offLineCheck)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !hostname)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', hostname='{}')", kodiBase,
               static_cast<const void*>(hostname));
    return false;
  }

  //! TODO change to use the LanCheckMode enum class on the API signature
  return URIUtils::IsHostOnLAN(hostname, offLineCheck ? LanCheckMode::ANY_PRIVATE_SUBNET
                                                      : LanCheckMode::ONLY_LOCAL_SUBNET);
}

char* Interface_Network::dns_lookup(void* kodiBase, const char* url, bool* ret)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url || !ret)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', url='{}', ret='{}')", kodiBase,
               static_cast<const void*>(url), static_cast<void*>(ret));
    return nullptr;
  }

  std::string string;
  *ret = CServiceBroker::GetDNSNameCache()->Lookup(url, string);
  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

char* Interface_Network::url_encode(void* kodiBase, const char* url)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon || !url)
  {
    CLog::LogF(LOGERROR, "Invalid data (addon='{}', url='{}')", kodiBase,
               static_cast<const void*>(url));
    return nullptr;
  }

  const std::string string{CURL::Encode(url)};
  if (string.empty())
    return nullptr;

  return strdup(string.c_str());
}

} /* namespace ADDON */
