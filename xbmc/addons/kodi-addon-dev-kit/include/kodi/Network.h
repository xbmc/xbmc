/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonBase.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /*
   * For interface between add-on and kodi.
   *
   * This structure defines the addresses of functions stored inside Kodi which
   * are then available for the add-on to call
   *
   * All function pointers there are used by the C++ interface functions below.
   * You find the set of them on xbmc/addons/interfaces/General.cpp
   *
   * Note: For add-on development itself this is not needed
   */
  typedef struct AddonToKodiFuncTable_kodi_network
  {
    bool (*wake_on_lan)(KODI_HANDLE kodiBase, const char* mac);
    char* (*get_ip_address)(KODI_HANDLE kodiBase);
    char* (*dns_lookup)(KODI_HANDLE kodiBase, const char* url, bool* ret);
    char* (*url_encode)(KODI_HANDLE kodiBase, const char* url);
  } AddonToKodiFuncTable_kodi_network;

#ifdef __cplusplus
} /* extern "C" */

//==============================================================================
///
/// \defgroup cpp_kodi_network  Interface - kodi::network
/// \ingroup cpp
/// @brief **Network functions**
///
/// The network module offers functions that allow you to control it.
///
/// It has the header \ref Network.h "#include <kodi/Network.h>" be included
/// to enjoy it.
///
//------------------------------------------------------------------------------

namespace kodi
{
namespace network
{

//============================================================================
///
/// \ingroup cpp_kodi_network
/// @brief Send WakeOnLan magic packet.
///
/// @param[in] mac Network address of the host to wake.
/// @return True if the magic packet was successfully sent, false otherwise.
///
inline bool WakeOnLan(const std::string& mac)
{
  using namespace ::kodi::addon;

  return CAddonBase::m_interface->toKodi->kodi_network->wake_on_lan(
      CAddonBase::m_interface->toKodi->kodiBase, mac.c_str());
}
//----------------------------------------------------------------------------

//============================================================================
///
/// \ingroup cpp_kodi_network
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
inline std::string GetIPAddress()
{
  using namespace ::kodi::addon;

  std::string ip;
  char* string = CAddonBase::m_interface->toKodi->kodi_network->get_ip_address(
      CAddonBase::m_interface->toKodi->kodiBase);
  if (string != nullptr)
  {
    ip = string;
    CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, string);
  }
  return ip;
}
//----------------------------------------------------------------------------

//============================================================================
///
/// \ingroup cpp_kodi_network
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
inline std::string URLEncode(const std::string& url)
{
  using namespace ::kodi::addon;

  std::string retString;
  char* string = CAddonBase::m_interface->toKodi->kodi_network->url_encode(
      CAddonBase::m_interface->toKodi->kodiBase, url.c_str());
  if (string != nullptr)
  {
    retString = string;
    CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, string);
  }
  return retString;
}
//----------------------------------------------------------------------------

//============================================================================
///
/// \ingroup cpp_kodi_network
/// @brief Lookup URL in DNS cache
///
/// This test will get DNS record for a domain. The DNS lookup is done directly
/// against the domain's authoritative name server, so changes to DNS Records
/// should show up instantly. By default, the DNS lookup tool will return an
/// IP address if you give it a name (e.g. www.example.com)
///
/// @param[in] hostName   The code of the message to get.
/// @param[out] ipAddress Returned address
/// @return true if successfull
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
inline bool DNSLookup(const std::string& hostName, std::string& ipAddress)
{
  using namespace ::kodi::addon;

  bool ret = false;
  char* string = CAddonBase::m_interface->toKodi->kodi_network->dns_lookup(
      CAddonBase::m_interface->toKodi->kodiBase, hostName.c_str(), &ret);
  if (string != nullptr)
  {
    ipAddress = string;
    CAddonBase::m_interface->toKodi->free_string(CAddonBase::m_interface->toKodi->kodiBase, string);
  }
  return ret;
}
//----------------------------------------------------------------------------

} /* namespace network */
} /* namespace kodi */

#endif /* __cplusplus */
