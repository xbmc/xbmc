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

#include "WifiConfiguration.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIWifiConfiguration::CJNIWifiConfiguration(const jhobject &object) : CJNIBase(object)
  ,networkId(             get_field<jint>(m_object, "networkId"))
  ,status(                get_field<jint>(m_object, "status"))
  ,SSID(                  jcast<std::string>(get_field<jhstring>(m_object, "SSID")))
  ,BSSID(                 jcast<std::string>(get_field<jhstring>(m_object, "BSSID")))
  ,preSharedKey(          jcast<std::string>(get_field<jhstring>(m_object, "preSharedKey")))
  ,wepTxKeyIndex(         get_field<jint>(m_object, "wepTxKeyIndex"))
  ,priority(              get_field<jint>(m_object, "priority"))
  ,hiddenSSID(            get_field<jboolean>(m_object, "hiddenSSID"))
  ,allowedKeyManagement(  get_field<jhobject>(m_object, "allowedKeyManagement", "Ljava/util/BitSet;"))
  ,allowedProtocols(      get_field<jhobject>(m_object, "allowedProtocols", "Ljava/util/BitSet;"))
  ,allowedPairwiseCiphers(get_field<jhobject>(m_object, "allowedPairwiseCiphers", "Ljava/util/BitSet;"))
  ,allowedGroupCiphers(   get_field<jhobject>(m_object, "allowedGroupCiphers", "Ljava/util/BitSet;"))
{
}

std::string CJNIWifiConfiguration::toString()
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "toString", "()Ljava/lang/String;"));
}

int CJNIWifiConfiguration::describeContents()
{
  return call_method<jint>(m_object,
    "describeContents", "()I");
}
