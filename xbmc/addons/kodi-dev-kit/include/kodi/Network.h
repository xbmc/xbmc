/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonBase.h"
#include "c-api/network.h"

#ifdef __cplusplus

//==============================================================================
/// @defgroup cpp_kodi_network  Interface - kodi::network
/// @ingroup cpp
/// @brief **Network functions**\n
/// The network module offers functions that allow you to control it.
///
/// It has the header @ref Network.h "#include <kodi/Network.h>" be included
/// to enjoy it.
///
//------------------------------------------------------------------------------

namespace kodi
{
namespace network
{

//============================================================================
/// @ingroup cpp_kodi_network
/// @brief Send WakeOnLan magic packet.
///
/// @param[in] mac Network address of the host to wake.
/// @return True if the magic packet was successfully sent, false otherwise.
///
inline bool ATTR_DLL_LOCAL WakeOnLan(const std::string& mac)
{
  using namespace ::kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_network->wake_on_lan(
      CPrivateBase::m_interface->toKodi->kodiBase, mac.c_str());
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_network
/// @brief To the current own ip address as a string.
///
/// @return Own system ip.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Network.h>
/// ...
/// std::string ipAddress = kodi::network::GetIPAddress();
/// fprintf(stderr, "My IP is '%s'\n", ipAddress.c_str());
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL GetIPAddress()
{
  using namespace ::kodi::addon;

  std::string ip;
  char* string = CPrivateBase::m_interface->toKodi->kodi_network->get_ip_address(
      CPrivateBase::m_interface->toKodi->kodiBase);
  if (string != nullptr)
  {
    ip = string;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   string);
  }
  return ip;
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_network
/// @brief Return our hostname.
///
/// @return String about hostname, empty in case of error
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Network.h>
/// ...
/// std::string hostname = kodi::network::GetHostname();
/// fprintf(stderr, "My hostname is '%s'\n", hostname.c_str());
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL GetHostname()
{
  using namespace ::kodi::addon;

  std::string ip;
  char* string = CPrivateBase::m_interface->toKodi->kodi_network->get_hostname(
      CPrivateBase::m_interface->toKodi->kodiBase);
  if (string != nullptr)
  {
    ip = string;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   string);
  }
  return ip;
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_network
/// @brief Returns Kodi's HTTP UserAgent string.
///
/// @return HTTP user agent
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.py}
/// ..
/// std::string agent = kodi::network::GetUserAgent()
/// ..
/// ~~~~~~~~~~~~~
///
/// example output:
///   Kodi/19.0-ALPHA1 (X11; Linux x86_64) Ubuntu/20.04 App_Bitness/64 Version/19.0-ALPHA1-Git:20200522-0076d136d3-dirty
///
inline std::string ATTR_DLL_LOCAL GetUserAgent()
{
  using namespace ::kodi::addon;

  std::string agent;
  char* string = CPrivateBase::m_interface->toKodi->kodi_network->get_user_agent(
      CPrivateBase::m_interface->toKodi->kodiBase);
  if (string != nullptr)
  {
    agent = string;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   string);
  }
  return agent;
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_network
/// @brief Check given name or ip address corresponds to localhost.
///
/// @param[in] hostname Hostname to check
/// @return Return true if given name or ip address corresponds to localhost
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Network.h>
/// ...
/// if (kodi::network::IsLocalHost("127.0.0.1"))
///   fprintf(stderr, "Is localhost\n");
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL IsLocalHost(const std::string& hostname)
{
  using namespace ::kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_network->is_local_host(
      CPrivateBase::m_interface->toKodi->kodiBase, hostname.c_str());
}
//----------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_network
/// @brief Checks whether the specified path refers to a local network.
///
/// @param[in] hostname Hostname to check
/// @param[in] offLineCheck Check if in private range, see https://en.wikipedia.org/wiki/Private_network
/// @return True if host is on a LAN, false otherwise
///
inline bool ATTR_DLL_LOCAL IsHostOnLAN(const std::string& hostname, bool offLineCheck = false)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_network->is_host_on_lan(
      CPrivateBase::m_interface->toKodi->kodiBase, hostname.c_str(), offLineCheck);
}
//------------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_network
/// @brief URL encodes the given string
///
/// This function converts the given input string to a URL encoded string and
/// returns that as a new allocated string. All input characters that are
/// not a-z, A-Z, 0-9, '-', '.', '_' or '~' are converted to their "URL escaped"
/// version (%NN where NN is a two-digit hexadecimal number).
///
/// @param[in] url The code of the message to get.
/// @return Encoded URL string
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Network.h>
/// ...
/// std::string encodedUrl = kodi::network::URLEncode("François");
/// fprintf(stderr, "Encoded URL is '%s'\n", encodedUrl.c_str());
/// ...
/// ~~~~~~~~~~~~~
/// For example, the string: François ,would be encoded as: Fran%C3%A7ois
///
inline std::string ATTR_DLL_LOCAL URLEncode(const std::string& url)
{
  using namespace ::kodi::addon;

  std::string retString;
  char* string = CPrivateBase::m_interface->toKodi->kodi_network->url_encode(
      CPrivateBase::m_interface->toKodi->kodiBase, url.c_str());
  if (string != nullptr)
  {
    retString = string;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   string);
  }
  return retString;
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_network
/// @brief Lookup URL in DNS cache
///
/// This test will get DNS record for a domain. The DNS lookup is done directly
/// against the domain's authoritative name server, so changes to DNS Records
/// should show up instantly. By default, the DNS lookup tool will return an
/// IP address if you give it a name (e.g. www.example.com)
///
/// @param[in] hostName   The code of the message to get.
/// @param[out] ipAddress Returned address
/// @return true if successful
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Network.h>
/// ...
/// std::string ipAddress;
/// bool ret = kodi::network::DNSLookup("www.google.com", ipAddress);
/// fprintf(stderr, "DNSLookup returned for www.google.com the IP '%s', call was %s\n", ipAddress.c_str(), ret ? "ok" : "failed");
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL DNSLookup(const std::string& hostName, std::string& ipAddress)
{
  using namespace ::kodi::addon;

  bool ret = false;
  char* string = CPrivateBase::m_interface->toKodi->kodi_network->dns_lookup(
      CPrivateBase::m_interface->toKodi->kodiBase, hostName.c_str(), &ret);
  if (string != nullptr)
  {
    ipAddress = string;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   string);
  }
  return ret;
}
//----------------------------------------------------------------------------

} /* namespace network */
} /* namespace kodi */

#endif /* __cplusplus */
