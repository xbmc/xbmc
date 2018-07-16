/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfOSX.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <string>
#include <sstream>

CZeroconfOSX::CZeroconfOSX():m_runloop(0)
{
  //acquire the main threads event loop
  m_runloop = CFRunLoopGetMain();
}

CZeroconfOSX::~CZeroconfOSX()
{
  doStop();
}

CFHashCode CFHashNullVersion (CFTypeRef cf)
{
  return 0;
}


//methods to implement for concrete implementations
bool CZeroconfOSX::doPublishService(const std::string& fcr_identifier,
                      const std::string& fcr_type,
                      const std::string& fcr_name,
                      unsigned int f_port,
                      const std::vector<std::pair<std::string, std::string> >& txt)
{
  CLog::Log(LOGDEBUG, "CZeroconfOSX::doPublishService identifier: %s type: %s name:%s port:%i", fcr_identifier.c_str(),
            fcr_type.c_str(), fcr_name.c_str(), f_port);

  CFStringRef name = CFStringCreateWithCString (NULL,
                                                fcr_name.c_str(),
                                                kCFStringEncodingUTF8
                                                );
  CFStringRef type = CFStringCreateWithCString (NULL,
                                                fcr_type.c_str(),
                                                kCFStringEncodingUTF8
                                                );
  CFNetServiceRef netService = CFNetServiceCreate(NULL, CFSTR(""), type, name, f_port);
  CFRelease(name);
  CFRelease(type);

  //now register it
  CFNetServiceClientContext clientContext = { 0, this, NULL, NULL, NULL };

  CFStreamError error;
  CFNetServiceSetClient(netService, registerCallback, &clientContext);
  CFNetServiceScheduleWithRunLoop(netService, m_runloop, kCFRunLoopCommonModes);

  //add txt records
  if(!txt.empty())
  {

    CFDictionaryKeyCallBacks key_cb = kCFTypeDictionaryKeyCallBacks;
    key_cb.hash = CFHashNullVersion;

    //txt map to dictionary
    CFDataRef txtData = NULL;
    CFMutableDictionaryRef txtDict = CFDictionaryCreateMutable(NULL, 0, &key_cb, &kCFTypeDictionaryValueCallBacks);
    for(std::vector<std::pair<std::string, std::string> >::const_iterator it = txt.begin(); it != txt.end(); ++it)
    {
      CFStringRef key = CFStringCreateWithCString (NULL,
                                                   it->first.c_str(),
                                                   kCFStringEncodingUTF8
                                                  );
      CFDataRef value = CFDataCreate              ( NULL,
                                                    (UInt8 *)it->second.c_str(),
                                                    strlen(it->second.c_str())
                                                  );

      CFDictionaryAddValue(txtDict,key, value);
      CFRelease(key);
      CFRelease(value);
    }

    //add txt records to service
    txtData = CFNetServiceCreateTXTDataWithDictionary(NULL, txtDict);
    CFNetServiceSetTXTData(netService, txtData);
    CFRelease(txtData);
    CFRelease(txtDict);
  }

  Boolean result = CFNetServiceRegisterWithOptions (netService, 0, &error);
  if (result == false)
  {
    // Something went wrong so lets clean up.
    CFNetServiceUnscheduleFromRunLoop(netService, m_runloop, kCFRunLoopCommonModes);
    CFNetServiceSetClient(netService, NULL, NULL);
    CFRelease(netService);
    netService = NULL;
    CLog::Log(LOGERROR, "CZeroconfOSX::doPublishService CFNetServiceRegister returned "
      "(domain = %d, error = %" PRId64")", (int)error.domain, (int64_t)error.error);
  } else
  {
    CSingleLock lock(m_data_guard);
    m_services.insert(make_pair(fcr_identifier, netService));
  }

  return result;
}

bool CZeroconfOSX::doForceReAnnounceService(const std::string& fcr_identifier)
{
  bool ret = false;
  CSingleLock lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_identifier);
  if(it != m_services.end())
  {
    CFNetServiceRef service = it->second;

    CFDataRef txtData = CFNetServiceGetTXTData(service);
    // convert the txtdata back and forth is enough to trigger a reannounce later
    CFDictionaryRef txtDict = CFNetServiceCreateDictionaryWithTXTData(NULL, txtData);
    CFMutableDictionaryRef txtDictMutable =CFDictionaryCreateMutableCopy(NULL, 0, txtDict);
    txtData = CFNetServiceCreateTXTDataWithDictionary(NULL, txtDictMutable);

    // this triggers the reannounce
    ret = CFNetServiceSetTXTData(service, txtData);

    CFRelease(txtDictMutable);
    CFRelease(txtDict);
    CFRelease(txtData);
  }
  return ret;
}


bool CZeroconfOSX::doRemoveService(const std::string& fcr_ident)
{
  CSingleLock lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_ident);
  if(it != m_services.end())
  {
    cancelRegistration(it->second);
    m_services.erase(it);
    return true;
  } else
    return false;
}

void CZeroconfOSX::doStop()
{
  CSingleLock lock(m_data_guard);
  for(tServiceMap::iterator it = m_services.begin(); it != m_services.end(); ++it)
    cancelRegistration(it->second);
  m_services.clear();
}


void CZeroconfOSX::registerCallback(CFNetServiceRef theService, CFStreamError* error, void* info)
{
  if (error->domain == kCFStreamErrorDomainNetServices)
  {
    CZeroconfOSX* p_this = reinterpret_cast<CZeroconfOSX*>(info);
    switch(error->error) {
      case kCFNetServicesErrorCollision:
        CLog::Log(LOGERROR, "CZeroconfOSX::registerCallback name collision occured");
        break;
      default:
        CLog::Log(LOGERROR, "CZeroconfOSX::registerCallback returned "
          "(domain = %d, error = %" PRId64")", (int)error->domain, (int64_t)error->error);
        break;
    }
    p_this->cancelRegistration(theService);
    //remove it
    CSingleLock lock(p_this->m_data_guard);
    for(tServiceMap::iterator it = p_this->m_services.begin(); it != p_this->m_services.end(); ++it)
    {
      if(it->second == theService)
      {
        p_this->m_services.erase(it);
        break;
      }
    }
  }
}

void CZeroconfOSX::cancelRegistration(CFNetServiceRef theService)
{
  assert(theService != NULL);
  CFNetServiceUnscheduleFromRunLoop(theService, m_runloop, kCFRunLoopCommonModes);
  CFNetServiceSetClient(theService, NULL, NULL);
  CFNetServiceCancel(theService);
  CFRelease(theService);
}
