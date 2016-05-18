/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_Network.h"

#include "Application.h"
#include "PasswordManager.h"
#include "URL.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "network/DNSNameCache.h"
#include "network/Network.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnNetwork::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->Network.wake_on_lan     = V2::KodiAPI::AddOn::CAddOnNetwork::wake_on_lan;
  interfaces->Network.get_ip_address  = V2::KodiAPI::AddOn::CAddOnNetwork::get_ip_address;
  interfaces->Network.dns_lookup      = V2::KodiAPI::AddOn::CAddOnNetwork::dns_lookup;
  interfaces->Network.url_encode      = V2::KodiAPI::AddOn::CAddOnNetwork::url_encode;
}

bool CAddOnNetwork::wake_on_lan(
        void*                     hdl,
        const char*               mac)
{
  try
  {
    if (!hdl || !mac)
      throw ADDON::WrongValueException("CAddOnNetwork - %s - invalid data (handle='%p', mac='%p')", __FUNCTION__, hdl, mac);

    return g_application.getNetwork().WakeOnLan(mac);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnNetwork::get_ip_address(
        void*                     hdl,
        char&                     ip,
        unsigned int&             iMaxStringSize)
{
  try
  {
    if (!hdl)
      throw ADDON::WrongValueException("CAddOnNetwork - %s - invalid data (handle='%p')", __FUNCTION__, hdl);

    std::string titleIP;
    CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
    if (iface)
      titleIP = iface->GetCurrentIPAddress();
    else
      titleIP = "127.0.0.1";

    strncpy(&ip, titleIP.c_str(), iMaxStringSize);
  }
  HANDLE_ADDON_EXCEPTION
}

char* CAddOnNetwork::dns_lookup(
        void*                     hdl,
        const char*               url,
        bool&                     ret)
{
  try
  {
    if (!hdl || !url)
      throw ADDON::WrongValueException("CAddOnNetwork - %s - invalid data (handle='%p', url='%p')", __FUNCTION__, hdl, url);

    std::string string;
    ret = CDNSNameCache::Lookup(url, string);
    char* buffer = nullptr;
    if (!string.empty())
      buffer = strdup(string.c_str());
    return buffer;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

char* CAddOnNetwork::url_encode(
        void*                     hdl,
        const char*               url)
{
  try
  {
    if (!hdl || !url)
      throw ADDON::WrongValueException("CAddOnNetwork - %s - invalid data (handle='%p', url='%p')", __FUNCTION__, hdl, url);

    std::string string = CURL::Encode(url);
    char* buffer = nullptr;
    if (!string.empty())
      buffer = strdup(string.c_str());
    return buffer;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V2 */
