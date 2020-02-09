/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfBrowserMDNS.h"

#include <netinet/in.h>
#include <arpa/inet.h>

#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "network/DNSNameCache.h"
#include "ServiceBroker.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#if defined(TARGET_WINDOWS)
#include "platform/win32/WIN32Util.h"
#endif //TARGET_WINDOWS

extern HWND g_hWnd;


CZeroconfBrowserMDNS::CZeroconfBrowserMDNS()
{
  m_browser = NULL;
}

CZeroconfBrowserMDNS::~CZeroconfBrowserMDNS()
{
  CSingleLock lock(m_data_guard);
  //make sure there are no browsers anymore
  for (const auto& it : m_service_browsers)
    doRemoveServiceType(it.first);

#if defined(TARGET_WINDOWS_DESKTOP)
  WSAAsyncSelect( (SOCKET) DNSServiceRefSockFD( m_browser ), g_hWnd, BONJOUR_BROWSER_EVENT, 0 );
#elif  defined(TARGET_WINDOWS_STORE)
  // need to modify this code to use WSAEventSelect since WSAAsyncSelect is not supported
  CLog::Log(LOGDEBUG, "%s is not implemented for TARGET_WINDOWS_STORE", __FUNCTION__);
#endif //TARGET_WINDOWS

  if (m_browser)
    DNSServiceRefDeallocate(m_browser);
  m_browser = NULL;
}

void DNSSD_API CZeroconfBrowserMDNS::BrowserCallback(DNSServiceRef browser,
                                                    DNSServiceFlags flags,
                                                    uint32_t interfaceIndex,
                                                    DNSServiceErrorType errorCode,
                                                    const char *serviceName,
                                                    const char *regtype,
                                                    const char *replyDomain,
                                                    void *context)
{

  if (errorCode == kDNSServiceErr_NoError)
  {
    //get our instance
    CZeroconfBrowserMDNS* p_this = reinterpret_cast<CZeroconfBrowserMDNS*>(context);
    //store the service
    ZeroconfService s(serviceName, regtype, replyDomain);

    if (flags & kDNSServiceFlagsAdd)
    {
      CLog::Log(LOGDEBUG, "ZeroconfBrowserMDNS::BrowserCallback found service named: %s, type: %s, domain: %s", s.GetName().c_str(), s.GetType().c_str(), s.GetDomain().c_str());
      p_this->addDiscoveredService(browser, s);
    }
    else
    {
      CLog::Log(LOGDEBUG, "ZeroconfBrowserMDNS::BrowserCallback service named: %s, type: %s, domain: %s disappeared", s.GetName().c_str(), s.GetType().c_str(), s.GetDomain().c_str());
      p_this->removeDiscoveredService(browser, s);
    }
    if(! (flags & kDNSServiceFlagsMoreComing) )
    {
      CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
      message.SetStringParam("zeroconf://");
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
      CLog::Log(LOGDEBUG, "ZeroconfBrowserMDNS::BrowserCallback sent gui update for path zeroconf://");
    }
  }
  else
  {
    CLog::Log(LOGERROR, "ZeroconfBrowserMDNS::BrowserCallback returned (error = %ld)", (int)errorCode);
  }
}

void DNSSD_API CZeroconfBrowserMDNS::GetAddrInfoCallback(DNSServiceRef                    sdRef,
                                                         DNSServiceFlags                  flags,
                                                         uint32_t                         interfaceIndex,
                                                         DNSServiceErrorType              errorCode,
                                                         const char                       *hostname,
                                                         const struct sockaddr            *address,
                                                         uint32_t                         ttl,
                                                         void                             *context
                                                         )
{

  if (errorCode)
  {
    CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: GetAddrInfoCallback failed with error = %ld", (int) errorCode);
    return;
  }

  std::string strIP;
  CZeroconfBrowserMDNS* p_instance = static_cast<CZeroconfBrowserMDNS*> ( context );

  if (address->sa_family == AF_INET)
    strIP = inet_ntoa(((const struct sockaddr_in *)address)->sin_addr);

  p_instance->m_resolving_service.SetIP(strIP);
  p_instance->m_addrinfo_event.Set();
}

void DNSSD_API CZeroconfBrowserMDNS::ResolveCallback(DNSServiceRef                       sdRef,
                                                    DNSServiceFlags                     flags,
                                                    uint32_t                            interfaceIndex,
                                                    DNSServiceErrorType                 errorCode,
                                                    const char                          *fullname,
                                                    const char                          *hosttarget,
                                                    uint16_t                            port,        /* In network byte order */
                                                    uint16_t                            txtLen,
                                                    const unsigned char                 *txtRecord,
                                                    void                                *context
                                                    )
{

  if (errorCode)
  {
    CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: ResolveCallback failed with error = %ld", (int) errorCode);
    return;
  }

  DNSServiceErrorType err;
  CZeroconfBrowser::ZeroconfService::tTxtRecordMap recordMap;
  std::string strIP;
  CZeroconfBrowserMDNS* p_instance = static_cast<CZeroconfBrowserMDNS*> ( context );

  p_instance->m_resolving_service.SetHostname(hosttarget);

  for(uint16_t i = 0; i < TXTRecordGetCount(txtLen, txtRecord); ++i)
  {
    char key[256];
    uint8_t valueLen;
    const void *value;
    std::string strvalue;
    err = TXTRecordGetItemAtIndex(txtLen, txtRecord,i ,sizeof(key) , key, &valueLen, &value);
    if(err != kDNSServiceErr_NoError)
      continue;

    if(value != NULL && valueLen > 0)
      strvalue.append((const char *)value, valueLen);

    recordMap.insert(std::make_pair(key, strvalue));
  }
  p_instance->m_resolving_service.SetTxtRecords(recordMap);
  p_instance->m_resolving_service.SetPort(ntohs(port));
  p_instance->m_resolved_event.Set();
}

/// adds the service to list of found services
void CZeroconfBrowserMDNS::addDiscoveredService(DNSServiceRef browser, CZeroconfBrowser::ZeroconfService const& fcr_service)
{
  CSingleLock lock(m_data_guard);
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
    services.push_back(std::make_pair(fcr_service, 1));
  else
    ++serviceIt->second;
}

void CZeroconfBrowserMDNS::removeDiscoveredService(DNSServiceRef browser, CZeroconfBrowser::ZeroconfService const& fcr_service)
{
  CSingleLock lock(m_data_guard);
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


bool CZeroconfBrowserMDNS::doAddServiceType(const std::string& fcr_service_type)
{
  DNSServiceErrorType err;
  DNSServiceRef browser = NULL;

#if !defined(HAS_MDNS_EMBEDDED)
  if(m_browser == NULL)
  {
    err = DNSServiceCreateConnection(&m_browser);
    if (err != kDNSServiceErr_NoError)
    {
      CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: DNSServiceCreateConnection failed with error = %ld", (int) err);
      return false;
    }
#if defined(TARGET_WINDOWS_DESKTOP)
    err = WSAAsyncSelect( (SOCKET) DNSServiceRefSockFD( m_browser ), g_hWnd, BONJOUR_BROWSER_EVENT, FD_READ | FD_CLOSE );
    if (err != kDNSServiceErr_NoError)
      CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: WSAAsyncSelect failed with error = %ld", (int) err);
#elif defined(TARGET_WINDOWS_STORE)
    // need to modify this code to use WSAEventSelect since WSAAsyncSelect is not supported
    CLog::Log(LOGERROR, "%s is not implemented for TARGET_WINDOWS_STORE", __FUNCTION__);
#endif // TARGET_WINDOWS_STORE
  }
#endif //!HAS_MDNS_EMBEDDED

  {
    CSingleLock lock(m_data_guard);
    browser = m_browser;
    err = DNSServiceBrowse(&browser, kDNSServiceFlagsShareConnection, kDNSServiceInterfaceIndexAny, fcr_service_type.c_str(), NULL, BrowserCallback, this);
  }

  if( err != kDNSServiceErr_NoError )
  {
    if (browser)
      DNSServiceRefDeallocate(browser);

    CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: DNSServiceBrowse returned (error = %ld)", (int) err);
    return false;
  }

  //store the browser
  {
    CSingleLock lock(m_data_guard);
    m_service_browsers.insert(std::make_pair(fcr_service_type, browser));
  }

  return true;
}

bool CZeroconfBrowserMDNS::doRemoveServiceType(const std::string& fcr_service_type)
{
  //search for this browser and remove it from the map
  DNSServiceRef browser = 0;
  {
    CSingleLock lock(m_data_guard);
    tBrowserMap::iterator it = m_service_browsers.find(fcr_service_type);
    if(it == m_service_browsers.end())
    {
      return false;
    }
    browser = it->second;
    m_service_browsers.erase(it);
  }

  //remove the services of this browser
  {
    CSingleLock lock(m_data_guard);
    tDiscoveredServicesMap::iterator it = m_discovered_services.find(browser);
    if(it != m_discovered_services.end())
      m_discovered_services.erase(it);
  }

  if (browser)
    DNSServiceRefDeallocate(browser);

  return true;
}

std::vector<CZeroconfBrowser::ZeroconfService> CZeroconfBrowserMDNS::doGetFoundServices()
{
  std::vector<CZeroconfBrowser::ZeroconfService> ret;
  CSingleLock lock(m_data_guard);
  for (const auto& it : m_discovered_services)
  {
    auto& services = it.second;
    for(unsigned int i = 0; i < services.size(); ++i)
    {
      ret.push_back(services[i].first);
    }
  }
  return ret;
}

bool CZeroconfBrowserMDNS::doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout)
{
  DNSServiceErrorType err;
  DNSServiceRef sdRef = NULL;

  //start resolving
  m_resolving_service = fr_service;
  m_resolved_event.Reset();

  err = DNSServiceResolve(&sdRef, 0, kDNSServiceInterfaceIndexAny, fr_service.GetName().c_str(), fr_service.GetType().c_str(), fr_service.GetDomain().c_str(), ResolveCallback, this);

  if( err != kDNSServiceErr_NoError )
  {
    if (sdRef)
      DNSServiceRefDeallocate(sdRef);

    CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: DNSServiceResolve returned (error = %ld)", (int) err);
    return false;
  }

  err = DNSServiceProcessResult(sdRef);

  if (err != kDNSServiceErr_NoError)
      CLog::Log(LOGERROR, "ZeroconfBrowserMDNS::doResolveService DNSServiceProcessResult returned (error = %ld)", (int) err);

#if defined(HAS_MDNS_EMBEDDED)
  // when using the embedded mdns service the call to DNSServiceProcessResult
  // above will not block until the resolving was finished - instead we have to
  // wait for resolve to return or timeout
  m_resolved_event.WaitMSec(f_timeout * 1000);
#endif //HAS_MDNS_EMBEDDED
  fr_service = m_resolving_service;

  if (sdRef)
    DNSServiceRefDeallocate(sdRef);

  // resolve the hostname
  if (!fr_service.GetHostname().empty())
  {
    std::string strIP;

    // use mdns resolving
    m_addrinfo_event.Reset();
    sdRef = NULL;

    err = DNSServiceGetAddrInfo(&sdRef, 0, kDNSServiceInterfaceIndexAny, kDNSServiceProtocol_IPv4, fr_service.GetHostname().c_str(), GetAddrInfoCallback, this);

    if (err != kDNSServiceErr_NoError)
      CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: DNSServiceGetAddrInfo returned (error = %ld)", (int) err);

    err = DNSServiceProcessResult(sdRef);

    if (err != kDNSServiceErr_NoError)
      CLog::Log(LOGERROR, "ZeroconfBrowserMDNS::doResolveService DNSServiceProcessResult returned (error = %ld)", (int) err);

#if defined(HAS_MDNS_EMBEDDED)
    // when using the embedded mdns service the call to DNSServiceProcessResult
    // above will not block until the resolving was finished - instead we have to
    // wait for resolve to return or timeout
    // give it 2 secs for resolving (resolving in mdns is cached and queued
    // in timeslices off 1 sec
    m_addrinfo_event.WaitMSec(2000);
#endif //HAS_MDNS_EMBEDDED
    fr_service = m_resolving_service;

    if (sdRef)
      DNSServiceRefDeallocate(sdRef);

    // fall back to our resolver
    if (fr_service.GetIP().empty())
    {
      CLog::Log(LOGWARNING, "ZeroconfBrowserMDNS: Could not resolve hostname %s falling back to CDNSNameCache", fr_service.GetHostname().c_str());
      if (CDNSNameCache::Lookup(fr_service.GetHostname(), strIP))
        fr_service.SetIP(strIP);
      else
        CLog::Log(LOGERROR, "ZeroconfBrowserMDNS: Could not resolve hostname %s", fr_service.GetHostname().c_str());
    }
  }

  return (!fr_service.GetIP().empty());
}

void CZeroconfBrowserMDNS::ProcessResults()
{
  CSingleLock lock(m_data_guard);
  DNSServiceErrorType err = DNSServiceProcessResult(m_browser);
  if (err != kDNSServiceErr_NoError)
    CLog::Log(LOGERROR, "ZeroconfWIN: DNSServiceProcessResult returned (error = %ld)", (int) err);
}
