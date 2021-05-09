/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfBrowserDarwin.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "platform/darwin/DarwinUtils.h"

#include <inttypes.h>

#include <arpa/inet.h>
#include <netinet/in.h>

namespace
{
  //helper for getting a the txt-records list
  //returns true on success, false if nothing found or error
  CZeroconfBrowser::ZeroconfService::tTxtRecordMap GetTxtRecords(CFNetServiceRef serviceRef)
  {
    CFIndex idx = 0;
    CZeroconfBrowser::ZeroconfService::tTxtRecordMap recordMap;
    CFDataRef data = NULL;

    data=CFNetServiceGetTXTData(serviceRef);
    if (data != NULL)
    {
      CFDictionaryRef dict = CFNetServiceCreateDictionaryWithTXTData(kCFAllocatorDefault, data);
      if (dict != NULL)
      {
        CFIndex numValues = 0;
        numValues = CFDictionaryGetCount(dict);
        if (numValues > 0)
        {
          CFStringRef keys[numValues];
          CFDataRef values[numValues];

          CFDictionaryGetKeysAndValues(dict, (const void **)&keys,  (const void **)&values);

          for(idx = 0; idx < numValues; idx++)
          {
            std::string key;
            if (CDarwinUtils::CFStringRefToUTF8String(keys[idx], key))
            {
              recordMap.insert(
                std::make_pair(
                  key,
                  std::string((const char *)CFDataGetBytePtr(values[idx]))
                )
              );
            }
          }
        }
        CFRelease(dict);
      }
    }
    return recordMap;
  }

  //helper to get (first) IP and port from a resolved service
  //returns true on success, false on if none was found
  bool CopyFirstIPv4Address(CFNetServiceRef serviceRef, std::string &fr_address, int &fr_port)
  {
    CFIndex idx;
    struct sockaddr_in address;
    char buffer[256];
    CFArrayRef addressResults = CFNetServiceGetAddressing( (CFNetServiceRef)serviceRef );

    if ( addressResults != NULL )
    {
      CFIndex numAddressResults = CFArrayGetCount( addressResults );
      CFDataRef sockAddrRef = NULL;
      struct sockaddr sockHdr;

      for ( idx = 0; idx < numAddressResults; idx++ )
      {
        sockAddrRef = (CFDataRef)CFArrayGetValueAtIndex( addressResults, idx );
        if ( sockAddrRef != NULL )
        {
          CFDataGetBytes( sockAddrRef, CFRangeMake(0, sizeof(sockHdr)), (UInt8*)&sockHdr );
          switch ( sockHdr.sa_family )
          {
            case AF_INET:
              CFDataGetBytes( sockAddrRef, CFRangeMake(0, sizeof(address)), (UInt8*)&address );
              if ( inet_ntop(sockHdr.sa_family, &address.sin_addr, buffer, sizeof(buffer)) != NULL )
              {
                fr_address = buffer;
                fr_port = ntohs(address.sin_port);
                return true;
              }
              break;
            case AF_INET6:
            default:
              break;
          }
        }
      }
    }
    return false;
  }
}

CZeroconfBrowserDarwin::CZeroconfBrowserDarwin()
{
  //acquire the main threads event loop
  m_runloop = CFRunLoopGetMain();
}

CZeroconfBrowserDarwin::~CZeroconfBrowserDarwin()
{
  CSingleLock lock(m_data_guard);
  //make sure there are no browsers anymore
  for (const auto& it : m_service_browsers)
    doRemoveServiceType(it.first);
}

void CZeroconfBrowserDarwin::BrowserCallback(CFNetServiceBrowserRef browser, CFOptionFlags flags, CFTypeRef domainOrService, CFStreamError *error, void *info)
{
  assert(info);

  if (error->error == noErr)
  {
    //make sure we receive a service
    assert(!(flags&kCFNetServiceFlagIsDomain));
    CFNetServiceRef service = (CFNetServiceRef)domainOrService;
    assert(service);
    //get our instance
    CZeroconfBrowserDarwin* p_this = reinterpret_cast<CZeroconfBrowserDarwin*>(info);

    //store the service
    std::string name, type, domain;
    if (!CDarwinUtils::CFStringRefToUTF8String(CFNetServiceGetName(service), name) ||
        !CDarwinUtils::CFStringRefToUTF8String(CFNetServiceGetType(service), type) ||
        !CDarwinUtils::CFStringRefToUTF8String(CFNetServiceGetDomain(service), domain))
    {
      CLog::Log(LOGWARNING, "CZeroconfBrowserDarwin::BrowserCallback failed to convert service strings.");
      return;
    }

    ZeroconfService s(name, type, domain);

    if (flags & kCFNetServiceFlagRemove)
    {
      CLog::Log(LOGDEBUG,
                "CZeroconfBrowserDarwin::BrowserCallback service named: {}, type: {}, domain: {} "
                "disappeared",
                s.GetName(), s.GetType(), s.GetDomain());
      p_this->removeDiscoveredService(browser, flags, s);
    }
    else
    {
      CLog::Log(
          LOGDEBUG,
          "CZeroconfBrowserDarwin::BrowserCallback found service named: {}, type: {}, domain: {}",
          s.GetName(), s.GetType(), s.GetDomain());
      p_this->addDiscoveredService(browser, flags, s);
    }
    if (! (flags & kCFNetServiceFlagMoreComing) )
    {
      CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
      message.SetStringParam("zeroconf://");
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
      CLog::Log(LOGDEBUG, "CZeroconfBrowserDarwin::BrowserCallback sent gui update for path zeroconf://");
    }
  } else
  {
    CLog::Log(LOGERROR,
              "CZeroconfBrowserDarwin::BrowserCallback returned"
              "(domain = {}, error = {})",
              (int)error->domain, (int64_t)error->error);
  }
}

/// adds the service to list of found services
void CZeroconfBrowserDarwin::
addDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, CZeroconfBrowser::ZeroconfService const &fcr_service)
{
  CSingleLock lock(m_data_guard);
  tDiscoveredServicesMap::iterator browserIt = m_discovered_services.find(browser);
  if (browserIt == m_discovered_services.end())
  {
     //first service by this browser
     browserIt = m_discovered_services.insert(make_pair(browser, std::vector<std::pair<ZeroconfService, unsigned int> >())).first;
  }
  //search this service
  std::vector<std::pair<ZeroconfService, unsigned int> >& services = browserIt->second;
  std::vector<std::pair<ZeroconfService, unsigned int> >::iterator serviceIt = services.begin();
  for( ; serviceIt != services.end(); ++serviceIt)
  {
    if (serviceIt->first == fcr_service)
      break;
  }
  if (serviceIt == services.end())
    services.emplace_back(fcr_service, 1);
  else
    ++serviceIt->second;
}

void CZeroconfBrowserDarwin::
removeDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, CZeroconfBrowser::ZeroconfService const &fcr_service)
{
  CSingleLock lock(m_data_guard);
  tDiscoveredServicesMap::iterator browserIt = m_discovered_services.find(browser);
  assert(browserIt != m_discovered_services.end());
  //search this service
  std::vector<std::pair<ZeroconfService, unsigned int> >& services = browserIt->second;
  std::vector<std::pair<ZeroconfService, unsigned int> >::iterator serviceIt = services.begin();
  for( ; serviceIt != services.end(); ++serviceIt)
    if (serviceIt->first == fcr_service)
      break;
  if (serviceIt != services.end())
  {
    //decrease refCount
    --serviceIt->second;
    if (!serviceIt->second)
    {
      //eventually remove the service
      services.erase(serviceIt);
    }
  }
  else
  {
    //looks like we missed the announce, no problem though..
  }
}


bool CZeroconfBrowserDarwin::doAddServiceType(const std::string& fcr_service_type)
{
  CFNetServiceClientContext clientContext = { 0, this, NULL, NULL, NULL };
  CFStringRef domain = CFSTR("");
  CFNetServiceBrowserRef p_browser = CFNetServiceBrowserCreate(kCFAllocatorDefault,
    CZeroconfBrowserDarwin::BrowserCallback, &clientContext);
  assert(p_browser != NULL);

  //schedule the browser
  CFNetServiceBrowserScheduleWithRunLoop(p_browser, m_runloop, kCFRunLoopCommonModes);
  CFStreamError error;
  CFStringRef type = CFStringCreateWithCString(NULL, fcr_service_type.c_str(), kCFStringEncodingUTF8);

  assert(type != NULL);
  Boolean result = CFNetServiceBrowserSearchForServices(p_browser, domain, type, &error);
  CFRelease(type);
  if (result == false)
  {
    // Something went wrong so lets clean up.
    CFNetServiceBrowserUnscheduleFromRunLoop(p_browser, m_runloop, kCFRunLoopCommonModes);
    CFRelease(p_browser);
    p_browser = NULL;
    CLog::Log(LOGERROR,
              "CFNetServiceBrowserSearchForServices returned"
              "(domain = {}, error = {})",
              (int)error.domain, (int64_t)error.error);
  }
  else
  {
    //store the browser
    CSingleLock lock(m_data_guard);
    m_service_browsers.insert(std::make_pair(fcr_service_type, p_browser));
  }

  return result;
}

bool CZeroconfBrowserDarwin::doRemoveServiceType(const std::string &fcr_service_type)
{
  //search for this browser and remove it from the map
  CFNetServiceBrowserRef browser = 0;
  {
    CSingleLock lock(m_data_guard);
    tBrowserMap::iterator it = m_service_browsers.find(fcr_service_type);
    if (it == m_service_browsers.end())
      return false;

    browser = it->second;
    m_service_browsers.erase(it);
  }
  assert(browser);

  //now kill the browser
  CFStreamError streamerror;
  CFNetServiceBrowserStopSearch(browser, &streamerror);
  CFNetServiceBrowserUnscheduleFromRunLoop(browser, m_runloop, kCFRunLoopCommonModes);
  CFNetServiceBrowserInvalidate(browser);
  //remove the services of this browser
  {
    CSingleLock lock(m_data_guard);
    tDiscoveredServicesMap::iterator it = m_discovered_services.find(browser);
    if (it != m_discovered_services.end())
      m_discovered_services.erase(it);
  }
  CFRelease(browser);

  return true;
}

std::vector<CZeroconfBrowser::ZeroconfService> CZeroconfBrowserDarwin::doGetFoundServices()
{
  std::vector<CZeroconfBrowser::ZeroconfService> ret;
  CSingleLock lock(m_data_guard);
  for (const auto& it : m_discovered_services)
  {
    const auto& services = it.second;
    for(unsigned int i = 0; i < services.size(); ++i)
    {
      ret.push_back(services[i].first);
    }
  }

  return ret;
}

bool CZeroconfBrowserDarwin::doResolveService(CZeroconfBrowser::ZeroconfService &fr_service, double f_timeout)
{
  bool ret = false;
  CFStringRef type = CFStringCreateWithCString(NULL, fr_service.GetType().c_str(), kCFStringEncodingUTF8);

  CFStringRef name = CFStringCreateWithCString(NULL, fr_service.GetName().c_str(), kCFStringEncodingUTF8);

  CFStringRef domain = CFStringCreateWithCString(NULL, fr_service.GetDomain().c_str(), kCFStringEncodingUTF8);

  CFNetServiceRef service = CFNetServiceCreate (NULL, domain, type, name, 0);
  if (CFNetServiceResolveWithTimeout(service, f_timeout, NULL) )
  {
    std::string ip;
    int port = 0;
    ret = CopyFirstIPv4Address(service, ip, port);
    fr_service.SetIP(ip);
    fr_service.SetPort(port);
    //get txt-record list
    fr_service.SetTxtRecords(GetTxtRecords(service));
  }
  CFRelease(type);
  CFRelease(name);
  CFRelease(domain);
  CFRelease(service);

  return ret;
}
