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

#include "ZeroconfAndroid.h"

#include <androidjni/Context.h>
#include "utils/log.h"

#include "threads/SingleLock.h"

CZeroconfAndroid::CZeroconfAndroid()
{
  m_manager = CJNIContext::getSystemService(CJNIContext::NSD_SERVICE);
}

CZeroconfAndroid::~CZeroconfAndroid()
{
  doStop();
}

bool CZeroconfAndroid::doPublishService(const std::string& fcr_identifier, const std::string& fcr_type, const std::string& fcr_name, unsigned int f_port, const std::vector<std::pair<std::string, std::string> >& txt)
{
  CLog::Log(LOGDEBUG, "ZeroconfAndroid: identifier: %s type: %s name:%s port:%i", fcr_identifier.c_str(), fcr_type.c_str(), fcr_name.c_str(), f_port);

  struct tServiceRef newService;
  
  newService.serviceInfo.setServiceName(fcr_name);
  newService.serviceInfo.setServiceType(fcr_type);
  newService.serviceInfo.setHost(CJNIInetAddress::getLocalHost());
  newService.serviceInfo.setPort(f_port);
  
  for (auto it : txt)
  {
//    CLog::Log(LOGDEBUG, "ZeroconfAndroid: key:%s, value:%s", it.first.c_str(),it.second.c_str());
    newService.serviceInfo.setAttribute(it.first, it.second);
  }
  
  m_manager.registerService(newService.serviceInfo, 1 /* PROTOCOL_DNS_SD */, newService.registrationListener);
  
  CSingleLock lock(m_data_guard);
  newService.updateNumber = 0;
  m_services.insert(make_pair(fcr_identifier, newService));

  return true;
}

bool CZeroconfAndroid::doForceReAnnounceService(const std::string& fcr_identifier)
{
  bool ret = false;
  CSingleLock lock(m_data_guard);
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
  CSingleLock lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_ident);
  if(it != m_services.end())
  {
    m_manager.unregisterService(it->second.registrationListener);
    m_services.erase(it);
    CLog::Log(LOGDEBUG, "CZeroconfAndroid: Removed service %s", fcr_ident.c_str());
    return true;
  }
  else
    return false;
}

void CZeroconfAndroid::doStop()
{
  {
    CSingleLock lock(m_data_guard);
    CLog::Log(LOGDEBUG, "ZeroconfAndroid: Shutdown services");
    for(auto it : m_services)
    {
      m_manager.unregisterService(it.second.registrationListener);
      CLog::Log(LOGDEBUG, "CZeroconfAndroid: Removed service %s", it.first.c_str());
    }
    m_services.clear();
  }
}

