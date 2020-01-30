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
#include "addons/kodi-addon-dev-kit/include/kodi/Network.h"
#include "network/DNSNameCache.h"
#include "network/Network.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_Network::Init(AddonGlobalInterface *addonInterface)
{
  addonInterface->toKodi->kodi_network = static_cast<AddonToKodiFuncTable_kodi_network*>(malloc(sizeof(AddonToKodiFuncTable_kodi_network)));

  addonInterface->toKodi->kodi_network->wake_on_lan = wake_on_lan;
  addonInterface->toKodi->kodi_network->get_ip_address = get_ip_address;
  addonInterface->toKodi->kodi_network->dns_lookup = dns_lookup;
  addonInterface->toKodi->kodi_network->url_encode = url_encode;
}

void Interface_Network::DeInit(AddonGlobalInterface* addonInterface)
{
  if (addonInterface->toKodi && /* <-- needed as long as the old addon way is used */
      addonInterface->toKodi->kodi_network)
  {
    free(addonInterface->toKodi->kodi_network);
    addonInterface->toKodi->kodi_network = nullptr;
  }
}

bool Interface_Network::wake_on_lan(void* kodiBase, const char* mac)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || mac == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Network::{} - invalid data (addon='{}', mac='{}')", __FUNCTION__,
              kodiBase, static_cast<const void*>(mac));
    return false;
  }

  return CServiceBroker::GetNetwork().WakeOnLan(mac);
}

char* Interface_Network::get_ip_address(void* kodiBase)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Network::{} - invalid data (addon='{}')", __FUNCTION__,
              kodiBase);
    return nullptr;
  }

  std::string titleIP;
  CNetworkInterface* iface = CServiceBroker::GetNetwork().GetFirstConnectedInterface();
  if (iface)
    titleIP = iface->GetCurrentIPAddress();
  else
    titleIP = "127.0.0.1";

  char* buffer = nullptr;
  if (!titleIP.empty())
    buffer = strdup(titleIP.c_str());
  return buffer;
}

char* Interface_Network::dns_lookup(void* kodiBase, const char* url, bool* ret)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || url == nullptr || ret == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Network::{} - invalid data (addon='{}', url='{}', ret='{}')",
              __FUNCTION__, kodiBase, static_cast<const void*>(url), static_cast<void*>(ret));
    return nullptr;
  }

  std::string string;
  *ret = CDNSNameCache::Lookup(url, string);
  char* buffer = nullptr;
  if (!string.empty())
    buffer = strdup(string.c_str());
  return buffer;
}

char* Interface_Network::url_encode(void* kodiBase, const char* url)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (addon == nullptr || url == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_Network::{} - invalid data (addon='{}', url='{}')", __FUNCTION__,
              kodiBase, static_cast<const void*>(url));
    return nullptr;
  }

  std::string string = CURL::Encode(url);
  char* buffer = nullptr;
  if (!string.empty())
    buffer = strdup(string.c_str());
  return buffer;
}

} /* namespace ADDON */
