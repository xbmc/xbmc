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

#include "WifiInfo.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

std::string CJNIWifiInfo::getSSID() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getSSID", "()Ljava/lang/String;"));
}

std::string CJNIWifiInfo::getBSSID() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getBSSID", "()Ljava/lang/String;"));
}

int CJNIWifiInfo::getRssi() const
{
  return call_method<jint>(m_object,
    "getRssi", "()I");
}

int CJNIWifiInfo::getLinkSpeed() const
{
  return call_method<jint>(m_object,
    "getLinkSpeed", "()I");
}

std::string CJNIWifiInfo::getMacAddress() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getMacAddress", "()Ljava/lang/String;"));
}

int CJNIWifiInfo::getNetworkId() const
{
  return call_method<jint>(m_object,
    "getNetworkId", "()I");
}

int CJNIWifiInfo::getIpAddress() const
{
  return call_method<jint>(m_object,
    "getIpAddress" , "()I");
}

bool CJNIWifiInfo::getHiddenSSID() const
{
  return call_method<jboolean>(m_object,
    "getHiddenSSID" , "()Z");
}

std::string CJNIWifiInfo::toString() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNIWifiInfo::describeContents() const
{
  return call_method<jint>(m_object,
    "describeContents" , "()I");
}

CJNISupplicantState CJNIWifiInfo::getSupplicantState() const
{
  return call_method<jhobject>(m_object,
    "getSupplicantState", "()Landroid/net/wifi/SupplicantState;");
}

CJNINetworkInfoDetailedState CJNIWifiInfo::getDetailedStateOf(const CJNISupplicantState &suppState)
{
  return call_static_method<jhobject>("platform/android/net/wifi/WifiInfo",
    "getDetailedStateOf", "(Landroid/net/wifi/SupplicantState;)Landroid/net/NetworkInfo$DetailedState;",
    suppState.get_raw());
}
