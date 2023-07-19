/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfBrowserAndroid.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "network/DNSNameCache.h"
#include "utils/log.h"

#include <mutex>

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

CZeroconfBrowserAndroid::CZeroconfBrowserAndroid()
  : m_manager(CJNIContext::getSystemService(CJNIContext::NSD_SERVICE))
{
}

CZeroconfBrowserAndroid::~CZeroconfBrowserAndroid()
{
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  //make sure there are no browsers anymore
  for (const auto& it : m_service_browsers)
    doRemoveServiceType(it.first);
}

bool CZeroconfBrowserAndroid::doAddServiceType(const std::string& fcr_service_type)
{
  CZeroconfBrowserAndroidDiscover* discover = new CZeroconfBrowserAndroidDiscover(this);

  // Remove trailing dot
//  std::string nsdType = fcr_service_type;
//  if (nsdType.compare(nsdType.size() - 1, 1, ".") == 0)
//    nsdType = nsdType.substr(0, nsdType.size() - 1);

  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroid::doAddServiceType: {}", fcr_service_type);
  m_manager.discoverServices(fcr_service_type,  1 /* PROTOCOL_DNS_SD */, *discover);

  //store the browser
  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
    m_service_browsers.insert(std::make_pair(fcr_service_type, discover));
  }
  return true;
}

bool CZeroconfBrowserAndroid::doRemoveServiceType(const std::string& fcr_service_type)
{
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroid::doRemoveServiceType: {}", fcr_service_type);

  CZeroconfBrowserAndroidDiscover* discover;
  //search for this browser and remove it from the map
  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
    tBrowserMap::iterator it = m_service_browsers.find(fcr_service_type);
    if(it == m_service_browsers.end())
    {
      return false;
    }
    discover = it->second;
    if (discover->IsActive())
      m_manager.stopServiceDiscovery(*discover);
    // Be extra careful: Discover listener is gone as of now
    m_service_browsers.erase(it);
  }

  //remove the services of this browser
  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
    tDiscoveredServicesMap::iterator it = m_discovered_services.find(discover);
    if(it != m_discovered_services.end())
      m_discovered_services.erase(it);
  }
  delete discover;

  return true;
}

std::vector<CZeroconfBrowser::ZeroconfService> CZeroconfBrowserAndroid::doGetFoundServices()
{
  std::vector<CZeroconfBrowser::ZeroconfService> ret;
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  for (const auto& it : m_discovered_services)
  {
    const auto& services = it.second;
    for (const auto& service : services)
    {
      ret.push_back(service.first);
    }
  }
  return ret;
}

bool CZeroconfBrowserAndroid::doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout)
{
  jni::CJNINsdServiceInfo service;
  service.setServiceName(fr_service.GetName());
  service.setServiceType(fr_service.GetType());

  CZeroconfBrowserAndroidResolve resolver;
  m_manager.resolveService(service, resolver);

  if (!resolver.m_resolutionDone.Wait(std::chrono::duration<double, std::milli>(f_timeout * 1000)))
  {
    CLog::Log(LOGERROR, "ZeroconfBrowserAndroid: DNSServiceResolve Timeout error");
    return false;
  }

  if (resolver.m_errorCode != -1)
  {
    CLog::Log(LOGERROR, "ZeroconfBrowserAndroid: DNSServiceResolve returned (error = {})",
              resolver.m_errorCode);
    return false;
  }

  fr_service.SetHostname(resolver.m_retServiceInfo.getHost().getHostName());
  fr_service.SetIP(resolver.m_retServiceInfo.getHost().getHostAddress());
  fr_service.SetPort(resolver.m_retServiceInfo.getPort());

  CZeroconfBrowser::ZeroconfService::tTxtRecordMap recordMap;
  jni::CJNISet<jni::jhstring> txtKey = resolver.m_retServiceInfo.getAttributes().keySet();
  jni::CJNIIterator<jni::jhstring> it = txtKey.iterator();
  while (it.hasNext())
  {
    jni::jhstring k = it.next();
    jni::jhbyteArray v = resolver.m_retServiceInfo.getAttributes().get(k);

    std::string key = jni::jcast<std::string>(k);
    std::vector<char> vv = jni::jcast<std::vector<char>>(v);
    std::string value = std::string(vv.begin(), vv.end());

    CLog::Log(LOGDEBUG, "ZeroconfBrowserAndroid: TXT record {} = {} ({})", key, value, vv.size());

    recordMap.insert(std::make_pair(key, value));
  }
  fr_service.SetTxtRecords(recordMap);
  return (!fr_service.GetIP().empty());
}

/// adds the service to list of found services
void CZeroconfBrowserAndroid::addDiscoveredService(CZeroconfBrowserAndroidDiscover* browser, CZeroconfBrowser::ZeroconfService const& fcr_service)
{
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  tDiscoveredServicesMap::iterator browserIt = m_discovered_services.find(browser);
  if(browserIt == m_discovered_services.end())
  {
     //first service by this browser
     browserIt = m_discovered_services.insert(make_pair(browser, std::vector<std::pair<ZeroconfService, unsigned int> >())).first;
  }
  //search this service
  std::vector<std::pair<ZeroconfService, unsigned int> >& services = browserIt->second;
  std::vector<std::pair<ZeroconfService, unsigned int> >::iterator serviceIt = services.begin();
  for( ; serviceIt != services.end(); ++serviceIt)
  {
    if(serviceIt->first == fcr_service)
      break;
  }
  if(serviceIt == services.end())
    services.emplace_back(fcr_service, 1);
  else
    ++serviceIt->second;
}

void CZeroconfBrowserAndroid::removeDiscoveredService(CZeroconfBrowserAndroidDiscover* browser, CZeroconfBrowser::ZeroconfService const& fcr_service)
{
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  tDiscoveredServicesMap::iterator browserIt = m_discovered_services.find(browser);
  //search this service
  std::vector<std::pair<ZeroconfService, unsigned int> >& services = browserIt->second;
  std::vector<std::pair<ZeroconfService, unsigned int> >::iterator serviceIt = services.begin();
  for( ; serviceIt != services.end(); ++serviceIt)
    if(serviceIt->first == fcr_service)
      break;
  if(serviceIt != services.end())
  {
    //decrease refCount
    --serviceIt->second;
    if(!serviceIt->second)
    {
      //eventually remove the service
      services.erase(serviceIt);
    }
  } else
  {
    //looks like we missed the announce, no problem though..
  }
}


/******************************************************************/

CZeroconfBrowserAndroidDiscover::CZeroconfBrowserAndroidDiscover(CZeroconfBrowserAndroid* browser)
  : CJNIXBMCNsdManagerDiscoveryListener(), m_browser(browser)
{
}

void CZeroconfBrowserAndroidDiscover::onDiscoveryStarted(const std::string& serviceType)
{
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroidDiscover::onDiscoveryStarted type: {}", serviceType);
  m_isActive = true;
}

void CZeroconfBrowserAndroidDiscover::onDiscoveryStopped(const std::string& serviceType)
{
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroidDiscover::onDiscoveryStopped type: {}", serviceType);
  m_isActive = false;
}

void CZeroconfBrowserAndroidDiscover::onServiceFound(const jni::CJNINsdServiceInfo& serviceInfo)
{
  //store the service
  CZeroconfBrowser::ZeroconfService s(serviceInfo.getServiceName(), serviceInfo.getServiceType(), "local");
  jni::CJNISet<jni::jhstring> txtKey = serviceInfo.getAttributes().keySet();
  jni::CJNIIterator<jni::jhstring> it = txtKey.iterator();
  while (it.hasNext())
  {
    jni::jhstring k = it.next();
    jni::jhbyteArray v = serviceInfo.getAttributes().get(k);

    std::string key = jni::jcast<std::string>(k);
    std::vector<char> vv = jni::jcast<std::vector<char>>(v);
    std::string value = std::string(vv.begin(), vv.end());

    CLog::Log(LOGDEBUG, "ZeroconfBrowserAndroid::onServiceFound: TXT record {} = {} ({})", key,
              value, vv.size());
  }

  CLog::Log(LOGDEBUG,
            "CZeroconfBrowserAndroidDiscover::onServiceFound found service named: {}, type: {}, "
            "domain: {}",
            s.GetName(), s.GetType(), s.GetDomain());
  m_browser->addDiscoveredService(this, s);

  CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
  message.SetStringParam("zeroconf://");
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroidDiscover::onServiceFound sent gui update for path zeroconf://");
}

void CZeroconfBrowserAndroidDiscover::onServiceLost(const jni::CJNINsdServiceInfo& serviceInfo)
{
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroidDiscover::onServiceLost name: {}, type: {}",
            serviceInfo.getServiceName(), serviceInfo.getServiceType());
}

void CZeroconfBrowserAndroidDiscover::onStartDiscoveryFailed(const std::string& serviceType, int errorCode)
{
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroidDiscover::onStartDiscoveryFailed type: {}, error: {}",
            serviceType, errorCode);
  m_isActive = false;
}

void CZeroconfBrowserAndroidDiscover::onStopDiscoveryFailed(const std::string& serviceType, int errorCode)
{
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroidDiscover::onStopDiscoveryFailed type: {}, error: {}",
            serviceType, errorCode);
  m_isActive = false;
}

/***************************************/

CZeroconfBrowserAndroidResolve::CZeroconfBrowserAndroidResolve()
  : CJNIXBMCNsdManagerResolveListener()
{
}

void CZeroconfBrowserAndroidResolve::onResolveFailed(const jni::CJNINsdServiceInfo& serviceInfo, int errorCode)
{
  CLog::Log(LOGDEBUG,
            "CZeroconfBrowserAndroidResolve::onResolveFailed name: {}, type: {}, error: {}",
            serviceInfo.getServiceName(), serviceInfo.getServiceType(), errorCode);
  m_errorCode = errorCode;
  m_resolutionDone.Set();
}

void CZeroconfBrowserAndroidResolve::onServiceResolved(const jni::CJNINsdServiceInfo& serviceInfo)
{
  CLog::Log(LOGDEBUG, "CZeroconfBrowserAndroidResolve::onServiceResolved name: {}, type: {}",
            serviceInfo.getServiceName(), serviceInfo.getServiceType());
  m_errorCode = -1;
  m_retServiceInfo = serviceInfo;
  m_resolutionDone.Set();
}
