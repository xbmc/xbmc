#pragma once
/*
 *      Copyright (C) 2017 Christian Browet
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

#include "network/ZeroconfBrowser.h"

#include <androidjni/NsdManager.h>
#include <androidjni/NsdServiceInfo.h>

#include "platform/android/activity/JNIXBMCNsdManagerDiscoveryListener.h"
#include "platform/android/activity/JNIXBMCNsdManagerResolveListener.h"
#include "threads/Event.h"

class CZeroconfBrowserAndroid;

class CZeroconfBrowserAndroidDiscover : public jni::CJNIXBMCNsdManagerDiscoveryListener
{
public:
  explicit CZeroconfBrowserAndroidDiscover(CZeroconfBrowserAndroid* browser);
  bool IsActive() { return m_isActive; }
  
  // CJNINsdManagerDiscoveryListener interface
public:
  void onDiscoveryStarted(const std::string& serviceType);
  void onDiscoveryStopped(const std::string& serviceType);
  void onServiceFound(const jni::CJNINsdServiceInfo& serviceInfo);
  void onServiceLost(const jni::CJNINsdServiceInfo& serviceInfo);
  void onStartDiscoveryFailed(const std::string& serviceType, int errorCode);
  void onStopDiscoveryFailed(const std::string& serviceType, int errorCode);
  
protected:
  CZeroconfBrowserAndroid* m_browser;
  bool m_isActive;
};

class CZeroconfBrowserAndroidResolve : public jni::CJNIXBMCNsdManagerResolveListener
{
public:
  CZeroconfBrowserAndroidResolve();
  
  // CJNINsdManagerResolveListener interface
public:
  void onResolveFailed(const jni::CJNINsdServiceInfo& serviceInfo, int errorCode);
  void onServiceResolved(const jni::CJNINsdServiceInfo& serviceInfo);
  
  CEvent m_resolutionDone;
  int m_errorCode;
  jni::CJNINsdServiceInfo m_retServiceInfo;
};

class CZeroconfBrowserAndroid : public CZeroconfBrowser
{
  friend class CZeroconfBrowserAndroidDiscover;
  
public:
  CZeroconfBrowserAndroid();
  virtual ~CZeroconfBrowserAndroid();
  
  // CZeroconfBrowser interface
protected:
  bool doAddServiceType(const std::string& fcr_service_type);
  bool doRemoveServiceType(const std::string& fcr_service_type);
  std::vector<ZeroconfService> doGetFoundServices();
  bool doResolveService(ZeroconfService& fr_service, double f_timeout);
  
  void addDiscoveredService(CZeroconfBrowserAndroidDiscover* browser, const CZeroconfBrowser::ZeroconfService& fcr_service);
  void removeDiscoveredService(CZeroconfBrowserAndroidDiscover* browser, const CZeroconfBrowser::ZeroconfService& fcr_service);

private:
  jni::CJNINsdManager m_manager;

  //shared variables (with guard)
  CCriticalSection m_data_guard;
  typedef std::map<std::string, CZeroconfBrowserAndroidDiscover*> tBrowserMap;
  // tBrowserMap maps service types the corresponding browser  
  tBrowserMap m_service_browsers;
  //tDiscoveredServicesMap maps browsers to their discovered services + a ref-count for each service
  //ref-count is needed, because a service might pop up more than once, if there's more than one network-iface
  typedef std::map<CZeroconfBrowserAndroidDiscover*, std::vector<std::pair<ZeroconfService, unsigned int> > > tDiscoveredServicesMap;
  tDiscoveredServicesMap m_discovered_services;
};
