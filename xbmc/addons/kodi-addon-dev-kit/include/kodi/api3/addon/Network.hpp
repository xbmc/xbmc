#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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

#include <string>

#include "../definitions.hpp"
#include "../.internal/AddonLib_internal.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace AddOn
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_Network Network
  /// \ingroup CPP_KodiAPI_AddOn
  /// @{
  /// @brief <b>Network functions</b>
  ///
  /// The network module offers functions that allow you to control it.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref Network.hpp "#include <kodi/api3/addon/Network.hpp>" be included
  /// to enjoy it.
  ///
  namespace Network
  {
    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_Network
    /// @brief Send WakeOnLan magic packet.
    ///
    /// @param[in] mac Network address of the host to wake.
    /// @return True if the magic packet was successfully sent, false otherwise.
    ///
    bool WakeOnLan(const std::string& mac);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_Network
    /// @brief To the current own ip address as a string.
    ///
    /// @return Own system ip.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/Network.hpp>
    /// ...
    /// std::string strIpAddress = CPP_KodiAPI_AddOn_Network::GetIPAddress();
    /// fprintf(stderr, "My IP is '%s'\n", strIpAddress.c_str());
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string GetIPAddress();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_Network
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
    /// #include <kodi/api3/addon/Network.hpp>
    /// ...
    /// std::string encodedUrl = KodiAPI::AddOn::Network::URLEncode("François");
    /// fprintf(stderr, "Encoded URL is '%s'\n", encodedUrl.c_str());
    /// ...
    /// ~~~~~~~~~~~~~
    /// For example, the string: François ,would be encoded as: Fran%C3%A7ois
    ///
    std::string URLEncode(const std::string& url);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_Network
    /// @brief Lookup URL in DNS cache
    ///
    /// This test will get DNS record for a domain. The DNS lookup is done directly
    /// against the domain's authoritative name server, so changes to DNS Records
    /// should show up instantly. By default, the DNS lookup tool will return an
    /// IP address if you give it a name (e.g. www.example.com)
    ///
    /// @param[in] strHostName  The code of the message to get.
    /// @param[in] strIpAddress Returned address
    /// @return true if successfull
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/Network.hpp>
    /// ...
    /// std::string strIpAddress;
    /// bool ret = KodiAPI::AddOn::Network::DNSLookup("www.google.com", strIpAddress);
    /// fprintf(stderr, "DNSLookup returned for www.google.com the IP '%s', call was %s\n", strIpAddress.c_str(), ret ? "ok" : "failed");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool DNSLookup(const std::string& strHostName, std::string& strIpAddress);
    //----------------------------------------------------------------------------
  }; /* namespace Network */
  //@}

} /* namespace AddOn */
} /* namespace KodiAPI */

END_NAMESPACE()
