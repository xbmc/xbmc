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

#include "ConnectivityManager.h"
#include "NetworkInfo.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIConnectivityManager::TYPE_MOBILE(0);
int CJNIConnectivityManager::TYPE_WIFI(0);
int CJNIConnectivityManager::TYPE_MOBILE_MMS(0);
int CJNIConnectivityManager::TYPE_MOBILE_SUPL(0);
int CJNIConnectivityManager::TYPE_MOBILE_DUN(0);
int CJNIConnectivityManager::TYPE_MOBILE_HIPRI(0);
int CJNIConnectivityManager::TYPE_WIMAX(0);
int CJNIConnectivityManager::TYPE_BLUETOOTH(0);
int CJNIConnectivityManager::TYPE_DUMMY(0);
int CJNIConnectivityManager::TYPE_ETHERNET(0);
int CJNIConnectivityManager::DEFAULT_NETWORK_PREFERENCE(0);

void CJNIConnectivityManager::PopulateStaticFields()
{
  jhclass clazz = find_class("android/net/ConnectivityManager");
  TYPE_MOBILE       = (get_static_field<int>(clazz, "TYPE_MOBILE"));
  TYPE_WIFI         = (get_static_field<int>(clazz, "TYPE_WIFI"));
  TYPE_MOBILE_MMS   = (get_static_field<int>(clazz, "TYPE_MOBILE_MMS"));
  TYPE_MOBILE_SUPL  = (get_static_field<int>(clazz, "TYPE_MOBILE_SUPL"));
  TYPE_MOBILE_DUN   = (get_static_field<int>(clazz, "TYPE_MOBILE_DUN"));
  TYPE_MOBILE_HIPRI = (get_static_field<int>(clazz, "TYPE_MOBILE_HIPRI"));
  TYPE_WIMAX        = (get_static_field<int>(clazz, "TYPE_WIMAX"));
  TYPE_BLUETOOTH    = (get_static_field<int>(clazz, "TYPE_BLUETOOTH"));
  TYPE_DUMMY        = (get_static_field<int>(clazz, "TYPE_DUMMY"));
  TYPE_ETHERNET     = (get_static_field<int>(clazz, "TYPE_ETHERNET"));
  DEFAULT_NETWORK_PREFERENCE = (get_static_field<int>(clazz, "DEFAULT_NETWORK_PREFERENCE"));
}

bool CJNIConnectivityManager::isNetworkTypeValid(int networkType)
{
  return call_method<jboolean>(m_object,
    "isNetworkTypeValid", "(I)Z",
    networkType);
}

void CJNIConnectivityManager::setNetworkPreference(int preference)
{
  return call_method<void>(m_object,
    "setNetworkPreference", "(I)V",
    preference);
}

int CJNIConnectivityManager::getNetworkPreference()
{
  return call_method<jint>(m_object,
    "getNetworkPreference", "()I");
}

CJNINetworkInfo CJNIConnectivityManager::getActiveNetworkInfo()
{
  return call_method<jhobject>(m_object,
    "getActiveNetworkInfo", "()Landroid/net/NetworkInfo;");
}

CJNINetworkInfo CJNIConnectivityManager::getNetworkInfo(int networkType)
{
  return call_method<jhobject>(m_object,
    "getNetworkInfo", "(I)Landroid/net/NetworkInfo;",
    networkType);
}

std::vector<CJNINetworkInfo> CJNIConnectivityManager::getAllNetworkInfo()
{
  JNIEnv *env = xbmc_jnienv();

  jhobjectArray oNetworks = call_method<jhobjectArray>(m_object,
    "getAllNetworkInfo", "()[Landroid/net/NetworkInfo;");
  jsize size = env->GetArrayLength(oNetworks.get());
  std::vector<CJNINetworkInfo> networks;
  networks.reserve(size);
  for(int i = 0; i < size; i++)
    networks.push_back(CJNINetworkInfo(jhobject(env->GetObjectArrayElement(oNetworks.get(), i))));

  return networks;
}

int CJNIConnectivityManager::startUsingNetworkFeature(int networkType, std::string feature)
{
  return call_method<jint>(m_object,
    "startUsingNetworkFeature", "(ILjava/lang/String;)I",
    networkType, jcast<jhstring>(feature));
}

int CJNIConnectivityManager::stopUsingNetworkFeature(int networkType, std::string feature)
{
  return call_method<jint>(m_object,
    "stopUsingNetworkFeature", "(ILjava/lang/String;)I",
    networkType, jcast<jhstring>(feature));
}

bool CJNIConnectivityManager::requestRouteToHost(int networkType, int hostAddress)
{
  return call_method<jboolean>(m_object,
    "requestRouteToHost", "(II)Z",
    networkType, hostAddress);
}

bool CJNIConnectivityManager::getBackgroundDataSetting()
{
  return call_method<jboolean>(m_object,
    "getBackgroundDataSetting", "()Z");
}


