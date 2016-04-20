/*
 *      Copyright (C) 2016 Team KODI
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

#include "InterProcess.h"

#include <string>
#include <stdarg.h>

API_NAMESPACE

namespace KodiAPI
{
namespace AddOn
{
namespace Network
{

  bool WakeOnLan(const std::string& mac)
  {
    return g_interProcess.m_Callbacks->Network.wake_on_lan(g_interProcess.m_Handle, mac.c_str());
  }

  std::string GetIPAddress()
  {
    std::string ip;
    ip.resize(32);
    unsigned int size = (unsigned int)ip.capacity();
    g_interProcess.m_Callbacks->Network.get_ip_address(g_interProcess.m_Handle, ip[0], size);
    ip.resize(size);
    ip.shrink_to_fit();
    return ip;
  }

  std::string URLEncode(const std::string& url)
  {
    std::string retString;
    char* string = g_interProcess.m_Callbacks->Network.url_encode(g_interProcess.m_Handle, url.c_str());
    if (string != nullptr)
    {
      retString = string;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, string);
    }
    return retString;
  }

  bool DNSLookup(const std::string& strHostName, std::string& strIpAddress)
  {
    bool ret = false;
    char* ipAddress = g_interProcess.m_Callbacks->Network.dns_lookup(g_interProcess.m_Handle, strHostName.c_str(), ret);
    if (ipAddress != nullptr)
    {
      strIpAddress = ipAddress;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, ipAddress);
    }
    return ret;
  }

} /* namespace KodiAPI_Codec */
} /* namespace AddOn */
} /* namespace KodiAPI */

END_NAMESPACE()
