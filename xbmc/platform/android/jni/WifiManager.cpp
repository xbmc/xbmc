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

#include "WifiManager.h"
#include "DhcpInfo.h"
#include "List.h"
#include "WifiInfo.h"
#include "WifiConfiguration.h"
#include "ScanResult.h"
#include "WifiManagerMulticastLock.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIList<CJNIWifiConfiguration> CJNIWifiManager::getConfiguredNetworks()
{
  return call_method<jhobject>(m_object,
    "getConfiguredNetworks" , "()Ljava/util/List;");
}

int CJNIWifiManager::addNetwork(const CJNIWifiConfiguration &config)
{
    return call_method<int>(m_object,
    "addNetwork" , "(Landroid/net/wifi/WifiConfiguration;)I", config.get_raw());
}

int CJNIWifiManager::updateNetwork(const CJNIWifiConfiguration &config)
{
    return call_method<int>(m_object,
    "updateNetwork" , "(Landroid/net/wifi/WifiConfiguration;)I", config.get_raw());
}

CJNIList<CJNIScanResult> CJNIWifiManager::getScanResults()
{
  return call_method<jhobject>(m_object,
    "getScanResults" , "()Ljava/util/List;");
}

bool CJNIWifiManager::removeNetwork(int netId)
{
  return call_method<jboolean>(m_object,
    "removeNetwork", "(I)Z", netId);
}

bool CJNIWifiManager::enableNetwork(int netID, bool disableOthers)
{
  return call_method<jboolean>(m_object,
    "enableNetwork", "(IZ)Z", netID, disableOthers);
}

bool CJNIWifiManager::disableNetwork(int netID)
{
  return call_method<jboolean>(m_object,
    "disableNetwork", "(I)Z", netID);
}

bool CJNIWifiManager::disconnect()
{
  return call_method<jboolean>(m_object,
    "disconnect", "()Z");
}

bool CJNIWifiManager::reconnect()
{
  return call_method<jboolean>(m_object,
    "reconnect", "()Z");
}

bool CJNIWifiManager::reassociate()
{
  return call_method<jboolean>(m_object,
    "rassociate", "()Z");
}

bool CJNIWifiManager::pingSupplicant()
{
  return call_method<jboolean>(m_object,
    "pingSupplicant", "()Z");
}

bool CJNIWifiManager::startScan()
{
  return call_method<jboolean>(m_object,
    "startScan", "()Z");
}

CJNIWifiInfo CJNIWifiManager::getConnectionInfo()
{
  return call_method<jhobject>(m_object,
    "getConnectionInfo", "()Landroid/net/wifi/WifiInfo;");
}


bool CJNIWifiManager::saveConfiguration()
{
  return call_method<jboolean>(m_object,
    "saveConfiguration", "()Z");
}

CJNIDhcpInfo CJNIWifiManager::getDhcpInfo()
{
  return call_method<jhobject>(m_object,
    "getDhcpInfo", "()Landroid/net/DhcpInfo;");
}

bool CJNIWifiManager::setWifiEnabled(bool enabled)
{
  return call_method<jboolean>(m_object,
    "setWifiEnabled", "(Z)Z", enabled);
}

int CJNIWifiManager::getWifiState()
{
  return call_method<jint>(m_object,
    "getWifiState", "()I");
}

bool CJNIWifiManager::isWifiEnabled()
{
  return call_method<jboolean>(m_object,
    "isWifiEnabled", "()Z");
}

int CJNIWifiManager::calculateSignalLevel(int rssi, int numLevels)
{
  return call_static_method<jint>("platform/android/net/wifi/WifiManager",
    "calculateSignalLevel", "(II)I", rssi, numLevels);
}

int CJNIWifiManager::compareSignalLevel(int rssiA, int rssiB)
{
  return call_static_method<jint>("platform/android/net/wifi/WifiManager",
    "compareSignalLevel", "(II)I", rssiA, rssiB);
}

CJNIWifiManagerMulticastLock CJNIWifiManager::createMulticastLock(const std::string &tag)
{
  return (CJNIWifiManagerMulticastLock)call_method<jhobject>(m_object, "createMulticastLock", "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;", jcast<jhstring>(tag));
}
