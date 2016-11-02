/*
 *      Copyright (C) 2016 Christian Browet
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

#include "LinkProperties.h"
#include "RouteInfo.h"
#include "InetAddress.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

std::string CJNILinkProperties::getDomains()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getDomains", "()Ljava/lang/String;"));
}

std::string CJNILinkProperties::getInterfaceName()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getInterfaceName", "()Ljava/lang/String;"));
}

CJNIList<CJNIRouteInfo> CJNILinkProperties::getRoutes()
{
  return call_method<jhobject>(m_object,
                               "getRoutes", "()Ljava/util/List;");
}

CJNIList<CJNILinkAddress> CJNILinkProperties::getLinkAddresses()
{
  return call_method<jhobject>(m_object,
                               "getLinkAddresses", "()Ljava/util/List;");
}

CJNIList<CJNIInetAddress> CJNILinkProperties::getDnsServers()
{
  return call_method<jhobject>(m_object,
                               "getDnsServers", "()Ljava/util/List;");
}


bool CJNILinkProperties::equals(const CJNILinkProperties& other)
{
  return call_method<jboolean>(m_object,
    "equals", "(Ljava/lang/Object;)Z", other.get_raw());
}

std::string CJNILinkProperties::toString() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNILinkProperties::describeContents() const
{
  return call_method<jint>(m_object,
    "describeContents", "()I");
}

