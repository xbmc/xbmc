/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ZeroconfBrowserOSX.h"
#include <Log.h>
#include <SingleLock.h>
#include <GUIWindowManager.h>
#include <GUIMessage.h>
#include <GUIUserMessages.h>

namespace
{
  CStdString CFStringToCStdString(const CFStringRef cfstr)
  {
    //first try the short path
    const char * p_tmp = CFStringGetCStringPtr (cfstr, kCFStringEncodingUTF8);
    if(p_tmp)
      return CStdString(p_tmp);
    
    // i'm not sure if CFStringGetMaximumSizeForEncoding
    // includes space for the termination character or not?
    // so i add 1 here to make sure..
    CFIndex buf_len = 1 + CFStringGetMaximumSizeForEncoding(
                                                            CFStringGetLength(cfstr), 
                                                            kCFStringEncodingUTF8);
    char *buffer = new char[buf_len];
    assert(buffer);
    Boolean bool2 = CFStringGetCString(cfstr, buffer, buf_len, kCFStringEncodingUTF8);
    assert(bool2);
    CStdString myString(buffer);
    delete[] buffer;
    return myString;
  }
  
  //helper to get (first) IP and port from a resolved service
  //returns true on success, false on if none was found
  bool CopyFirstIPv4Address(CFNetServiceRef service, CStdString& fr_address, int& fr_port)
  {
    struct sockaddr * socketAddress = NULL;
    CFArrayRef addresses;
    char buffer[256];
    int count;
    Boolean result = false;
    
    assert(service       != NULL);
    
    addresses = CFNetServiceGetAddressing(service);
    
    assert(addresses != NULL);
    assert(CFArrayGetCount(addresses) > 0);
    
    /* Search for the first IPv4 address in the array. */
    for (count = 0; count < CFArrayGetCount(addresses); count++) 
    {
      
      socketAddress = (struct sockaddr *)CFDataGetBytePtr((const CFDataRef)CFArrayGetValueAtIndex(addresses, count));
      
      /* Only continue if this is an IPv4 address. */
      if (socketAddress && socketAddress->sa_family == AF_INET)
      {
        
        if (inet_ntop(AF_INET, &((struct sockaddr_in *)socketAddress)->sin_addr, buffer, sizeof(buffer)))
        {
          fr_address = buffer;
          fr_port = ((struct sockaddr_in *)socketAddress)->sin_port;
          result = true;
        }
        break;
      }
    }
    return result;
  }
}

CZeroconfBrowserOSX::CZeroconfBrowserOSX():m_runloop(0)
{
  //aquire the main threads event loop
  EventLoopRef ref = GetMainEventLoop();
  m_runloop = (CFRunLoopRef)GetCFRunLoopFromEventLoop(ref);
}

CZeroconfBrowserOSX::~CZeroconfBrowserOSX()
{
  CSingleLock lock(m_data_guard);
  //make sure there are no browsers anymore
  for(tBrowserMap::iterator it = m_service_browsers.begin(); it != m_service_browsers.end(); ++it )
    doRemoveServiceType(it->first);
}

void CZeroconfBrowserOSX::BrowserCallback(CFNetServiceBrowserRef browser, CFOptionFlags flags, CFTypeRef domainOrService, CFStreamError *error, void *info)
{
  assert(info);

  if (error->error == noErr)
  {
    //make sure we receive a service
    assert(!(flags&kCFNetServiceFlagIsDomain));  
    CFNetServiceRef service = (CFNetServiceRef)domainOrService;
    assert(service);
    //get our instance
    CZeroconfBrowserOSX* p_this = reinterpret_cast<CZeroconfBrowserOSX*>(info);
    //store the service
    ZeroconfService s;
    s.name = CFStringToCStdString(CFNetServiceGetName(service));
    s.type = CFStringToCStdString(CFNetServiceGetType(service));
    s.domain = CFStringToCStdString(CFNetServiceGetDomain(service));
    if (flags & kCFNetServiceFlagRemove)
    {
      CLog::Log(LOGDEBUG, "CZeroconfBrowserOSX::BrowserCallback service named: %s, type: %s, domain: %s disappeared", 
                s.name.c_str(), s.type.c_str(), s.domain.c_str());
      p_this->removeDiscoveredService(browser, flags, s);      
    } else
    {
      CLog::Log(LOGDEBUG, "CZeroconfBrowserOSX::BrowserCallback found service named: %s, type: %s, domain: %s", 
                s.name.c_str(), s.type.c_str(), s.domain.c_str());
      p_this->addDiscoveredService(browser, flags, s);
    }
    if(! (flags & kCFNetServiceFlagMoreComing))
    {
      CGUIMessage message(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH);
      message.SetStringParam("zeroconf://");
      m_gWindowManager.SendThreadMessage(message);
      CLog::Log(LOGDEBUG, "CZeroconfBrowserOSX::BrowserCallback sent gui update for path zeroconf://");
    }
  } else
  {
    CLog::Log(LOGERROR, "CZeroconfBrowserOSX::BrowserCallback returned (domain = %d, error = %ld)\n", error->domain, error->error);
  }
}

/// adds the service to list of found services
void CZeroconfBrowserOSX::
addDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, CZeroconfBrowser::ZeroconfService const& fcr_service)
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
  {
    services.push_back(std::make_pair(fcr_service, 1));
  } else
  {
    ++serviceIt->second;
  }
}

/// removes the service from list of found services
void CZeroconfBrowserOSX::
removeDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, CZeroconfBrowser::ZeroconfService const& fcr_service)
{
  CSingleLock lock(m_data_guard);
  tDiscoveredServicesMap::iterator browserIt = m_discovered_services.find(browser);
  assert(browserIt != m_discovered_services.end());
  //search this service
  std::vector<std::pair<ZeroconfService, unsigned int> >& services = browserIt->second;
  std::vector<std::pair<ZeroconfService, unsigned int> >::iterator serviceIt = services.begin();
  for( ; serviceIt != services.end(); ++serviceIt)
  {
    if(serviceIt->first == fcr_service)
    {
      break;
    }
  }
  assert(serviceIt != services.end());
  //decrease refCount
  --serviceIt->second;
  if(!serviceIt->second)
  {
    //eventually remove the service
    services.erase(serviceIt);
  }  
}


bool CZeroconfBrowserOSX::doAddServiceType(const CStdString& fcr_service_type)
{
  CFNetServiceClientContext clientContext = { 0, this, NULL, NULL, NULL };
  CFStringRef domain = CFSTR("");
  CFNetServiceBrowserRef p_browser = CFNetServiceBrowserCreate(kCFAllocatorDefault, CZeroconfBrowserOSX::BrowserCallback, &clientContext);
  assert(p_browser != NULL);

  //schedule the browser
  CFNetServiceBrowserScheduleWithRunLoop(p_browser, m_runloop, kCFRunLoopCommonModes);
  CFStreamError error;
  CFStringRef type = CFStringCreateWithCString (NULL,
                                                fcr_service_type.c_str(),
                                                kCFStringEncodingUTF8
                                                );
  assert(type != NULL);
  Boolean result = CFNetServiceBrowserSearchForServices(p_browser, domain, type, &error);
  CFRelease(type);
  if (result == false)
  {
    // Something went wrong so lets clean up.
    CFNetServiceBrowserUnscheduleFromRunLoop(p_browser, m_runloop, kCFRunLoopCommonModes);         
    CFRelease(p_browser);
    p_browser = NULL;
    CLog::Log(LOGERROR, "CFNetServiceBrowserSearchForServices returned (domain = %d, error = %ld)\n", error.domain, error.error);
  } else
  {
    //store the browser
    CSingleLock lock(m_data_guard);
    m_service_browsers.insert(std::make_pair(fcr_service_type, p_browser));
  }
  return result;
}

bool CZeroconfBrowserOSX::doRemoveServiceType(const CStdString& fcr_service_type)
{
  //search for this browser and remove it from the map
  CFNetServiceBrowserRef browser = 0;
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
    if(it != m_discovered_services.end())
      m_discovered_services.erase(it);
  }
  CFRelease(browser);
  return true;
}

std::vector<CZeroconfBrowser::ZeroconfService> CZeroconfBrowserOSX::doGetFoundServices()
{
  std::vector<CZeroconfBrowser::ZeroconfService> ret;
  CSingleLock lock(m_data_guard);
  for(tDiscoveredServicesMap::const_iterator it = m_discovered_services.begin();
      it != m_discovered_services.end(); ++it)
  {
    const std::vector<std::pair<CZeroconfBrowser::ZeroconfService, unsigned int> >& services = it->second;
    for(unsigned int i = 0; i < services.size(); ++i)
    {
      ret.push_back(services[i].first);
    }
  }
  return ret;
}

bool CZeroconfBrowserOSX::doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout)
{
  bool ret = false;
  CFStringRef type = CFStringCreateWithCString (NULL,
                                                fr_service.type.c_str(),
                                                kCFStringEncodingUTF8
                                                );
  CFStringRef name = CFStringCreateWithCString (NULL,
                                                fr_service.name.c_str(),
                                                kCFStringEncodingUTF8
                                                );
  CFStringRef domain = CFStringCreateWithCString (NULL,
                                                  fr_service.domain.c_str(),
                                                  kCFStringEncodingUTF8
                                                  );
  CFNetServiceRef service = CFNetServiceCreate (NULL, domain, type, name, 0);
  if(CFNetServiceResolveWithTimeout (service, f_timeout, NULL) )
  {
    ret = CopyFirstIPv4Address(service, fr_service.ip, fr_service.port);
  }
  CFRelease(type);
  CFRelease(name);
  CFRelease(domain);
  CFRelease(service);
  return ret;
}
