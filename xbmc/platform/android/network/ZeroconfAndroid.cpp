/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfAndroid.h"

#include "utils/log.h"

#include <mutex>

#include <androidjni/Context.h>

CZeroconfAndroid::CZeroconfAndroid()
  : m_manager(CJNIContext::getSystemService(CJNIContext::NSD_SERVICE))
{
}

CZeroconfAndroid::~CZeroconfAndroid()
{
  doStop();
}

bool CZeroconfAndroid::doPublishService(const std::string& fcr_identifier, const std::string& fcr_type, const std::string& fcr_name, unsigned int f_port, const std::vector<std::pair<std::string, std::string> >& txt)
{
  CLog::Log(LOGDEBUG, "ZeroconfAndroid: identifier: {} type: {} name:{} port:{}", fcr_identifier,
            fcr_type, fcr_name, f_port);

  struct tServiceRef newService;

  newService.serviceInfo.setServiceName(fcr_name);
  newService.serviceInfo.setServiceType(fcr_type);
  newService.serviceInfo.setHost(CJNIInetAddress::getLocalHost());
  newService.serviceInfo.setPort(f_port);

  for (const auto& it : txt)
  {
    //    CLog::Log(LOGDEBUG, "ZeroconfAndroid: key:{}, value:{}", it.first,it.second);
    newService.serviceInfo.setAttribute(it.first, it.second);
  }

  m_manager.registerService(newService.serviceInfo, 1 /* PROTOCOL_DNS_SD */, newService.registrationListener);

  std::unique_lock<CCriticalSection> lock(m_data_guard);
  newService.updateNumber = 0;
  m_services.insert(make_pair(fcr_identifier, newService));

  return true;
}

bool CZeroconfAndroid::doForceReAnnounceService(const std::string& fcr_identifier)
{
  bool ret = false;
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_identifier);
  if(it != m_services.end())
  {
    // for force announcing a service with mdns we need
    // to change a txt record - so we diddle between
    // even and odd dummy records here
    if ( (it->second.updateNumber % 2) == 0)
      it->second.serviceInfo.setAttribute("xbmcdummy", "evendummy");
    else
      it->second.serviceInfo.setAttribute("xbmcdummy", "odddummy");
    it->second.updateNumber++;

    m_manager.unregisterService(it->second.registrationListener);
    it->second.registrationListener = jni::CJNIXBMCNsdManagerRegistrationListener();
    m_manager.registerService(it->second.serviceInfo, 1 /* PROTOCOL_DNS_SD */, it->second.registrationListener);
  }
  return ret;
}

bool CZeroconfAndroid::doRemoveService(const std::string& fcr_ident)
{
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_ident);
  if(it != m_services.end())
  {
    m_manager.unregisterService(it->second.registrationListener);
    m_services.erase(it);
    CLog::Log(LOGDEBUG, "CZeroconfAndroid: Removed service {}", fcr_ident);
    return true;
  }
  else
    return false;
}

void CZeroconfAndroid::doStop()
{
  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
    CLog::Log(LOGDEBUG, "ZeroconfAndroid: Shutdown services");
    for (const auto& it : m_services)
    {
      m_manager.unregisterService(it.second.registrationListener);
      CLog::Log(LOGDEBUG, "CZeroconfAndroid: Removed service {}", it.first);
    }
    m_services.clear();
  }
}

