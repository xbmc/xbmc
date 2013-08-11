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

CJNIWifiConfiguration::CJNIWifiConfiguration() : CJNIBase("android/net/wifi/WifiConfiguration")
{
  m_object = new_object(GetClassName());
  m_object.setGlobal();
}

int CJNIWifiConfiguration::getnetworkId() const
{
  return get_field<jint>(m_object, "networkId");
}

int CJNIWifiConfiguration::getstatus() const
{
  return get_field<jint>(m_object, "status");
}

std::string CJNIWifiConfiguration::getSSID() const
{
  return jcast<std::string>(get_field<jhstring>(m_object, "SSID"));
}

std::string CJNIWifiConfiguration::getBSSID() const
{
  return jcast<std::string>(get_field<jhstring>(m_object, "BSSID"));
}

std::string CJNIWifiConfiguration::getpreSharedKey() const
{
  return jcast<std::string>(get_field<jhstring>(m_object, "preSharedKey"));
}

std::vector<std::string> CJNIWifiConfiguration::getwepKeys() const
{
  return jcast<std::vector<std::string>>(get_field<jhobjectArray>(m_object,"wepKeys", "[Ljava/lang/String;"));
}

int CJNIWifiConfiguration::getwepTxKeyIndex() const
{
  return get_field<jint>(m_object, "wepTxKeyIndex");
}

int CJNIWifiConfiguration::getpriority() const
{
  return get_field<jint>(m_object, "priority");
}

bool CJNIWifiConfiguration::gethiddenSSID() const
{
  return get_field<jboolean>(m_object, "hiddenSSID");
}

CJNIBitSet CJNIWifiConfiguration::getallowedKeyManagement() const
{
  return get_field<jhobject>(m_object, "allowedKeyManagement", "Ljava/util/BitSet;");
}

CJNIBitSet CJNIWifiConfiguration::getallowedProtocols() const
{
  return get_field<jhobject>(m_object, "allowedProtocols", "Ljava/util/BitSet;");
}

CJNIBitSet CJNIWifiConfiguration::getallowedPairwiseCiphers() const
{
  return get_field<jhobject>(m_object, "allowedPairwiseCiphers", "Ljava/util/BitSet;");
}

CJNIBitSet CJNIWifiConfiguration::getallowedGroupCiphers() const
{
  return get_field<jhobject>(m_object, "allowedGroupCiphers", "Ljava/util/BitSet;");
}

void CJNIWifiConfiguration::setnetworkId(int networkId)
{
  set_field(m_object,"networkId", networkId);
}


void CJNIWifiConfiguration::setstatus(int status)
{
  set_field(m_object,"networkId", status);
}

void CJNIWifiConfiguration::setSSID(const std::string &SSID)
{
  set_field(m_object, "SSID", jcast<jhstring>(SSID));
}

void CJNIWifiConfiguration::setBSSID(const std::string &BSSID)
{
  set_field(m_object, "BSSID", jcast<jhstring>(BSSID));
}

void CJNIWifiConfiguration::setpreSharedKey(const std::string &preSharedKey)
{
  set_field(m_object,"preSharedKey", jcast<jhstring>(preSharedKey));
}

void CJNIWifiConfiguration::setwepKeys(const std::vector<std::string> &wepKeys)
{
  set_field(m_object, "wepKeys", "[Ljava/lang/String;", jcast<jhobjectArray>(wepKeys));
}

void CJNIWifiConfiguration::setwepTxKeyIndex(int wepTxKeyIndex)
{
  set_field(m_object, "wepTxKeyIndex", wepTxKeyIndex);
}

void CJNIWifiConfiguration::setpriority(int priority)
{
  set_field(m_object, "priority", priority);
}

void CJNIWifiConfiguration::sethiddenSSID(bool hiddenSSID)
{
  set_field(m_object, "hiddenSSID", (jboolean)hiddenSSID);
}

void CJNIWifiConfiguration::setallowedKeyManagement(const CJNIBitSet& allowedKeyManagement)
{
  set_field(m_object, "allowedKeyManagement", "Ljava/util/BitSet;", allowedKeyManagement.get_raw());
}

void CJNIWifiConfiguration::setallowedProtocols(const CJNIBitSet& allowedProtocols)
{
  set_field(m_object, "allowedProtocols", "Ljava/util/BitSet;", allowedProtocols.get_raw());
}

void CJNIWifiConfiguration::setallowedAuthAlgorithms(const CJNIBitSet& allowedAuthAlgorithms)
{
  set_field(m_object, "allowedAuthAlgorithms", "Ljava/util/BitSet;", allowedAuthAlgorithms.get_raw());
}

void CJNIWifiConfiguration::setallowedPairwiseCiphers(const CJNIBitSet& allowedPairwiseCiphers)
{
  set_field(m_object, "allowedPairwiseCiphers", "Ljava/util/BitSet;", allowedPairwiseCiphers.get_raw());
}

void CJNIWifiConfiguration::setallowedGroupCiphers(const CJNIBitSet& allowedGroupCiphers)
{
  set_field(m_object, "allowedGroupCiphers", "Ljava/util/BitSet;", allowedGroupCiphers.get_raw());
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
