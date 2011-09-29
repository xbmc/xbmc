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

#include "ZeroconfWIN.h"

#include <string>
#include <sstream>
#include <threads/SingleLock.h>
#include <utils/log.h>

#pragma comment(lib, "dnssd.lib")

CZeroconfWIN::CZeroconfWIN()
{
}

CZeroconfWIN::~CZeroconfWIN()
{
  doStop();
}


//methods to implement for concrete implementations
bool CZeroconfWIN::doPublishService(const std::string& fcr_identifier,
                      const std::string& fcr_type,
                      const std::string& fcr_name,
                      unsigned int f_port,
                      std::map<std::string, std::string> txt)
{
  DNSServiceFlags flags = 0;
  DNSServiceRef netService   = NULL;

  CLog::Log(LOGDEBUG, "CZeroconfWIN::doPublishService identifier: %s type: %s name:%s port:%i", fcr_identifier.c_str(), fcr_type.c_str(), fcr_name.c_str(), f_port);

  //add txt records
  if(!txt.empty())
  {
    // nothin useful yet and I dunno what to do with it
    for(std::map<std::string, std::string>::const_iterator it = txt.begin(); it != txt.end(); ++it)
    {
      CLog::Log(LOGDEBUG, "CZeroconfWIN: key:%4, value:%s",it->first.c_str(),it->second.c_str());
    }
  }

  DNSServiceErrorType err = DNSServiceRegister(&netService, flags, 0, fcr_name.c_str(), fcr_type.c_str(), NULL, NULL, f_port, 0, NULL, registerCallback, NULL);

  if (err != kDNSServiceErr_NoError)
  {
    // Something went wrong so lets clean up.
    if (netService)
      DNSServiceRefDeallocate(netService);

    CLog::Log(LOGERROR, "CZeroconfWIN::doPublishService CFNetServiceRegister returned (error = %ld)\n", (int) err);
  } 
  else
  {
    CSingleLock lock(m_data_guard);
    m_services.insert(make_pair(fcr_identifier, netService));
  }

  return err == kDNSServiceErr_NoError;
}

bool CZeroconfWIN::doRemoveService(const std::string& fcr_ident)
{
  CSingleLock lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_ident);
  if(it != m_services.end())
  {
    DNSServiceRefDeallocate(it->second);
    m_services.erase(it);
    return true;
  } 
  else
    return false;
}

void CZeroconfWIN::doStop()
{
  CSingleLock lock(m_data_guard);
  for(tServiceMap::iterator it = m_services.begin(); it != m_services.end(); ++it)
    DNSServiceRefDeallocate(it->second);
  m_services.clear();
}

void DNSSD_API CZeroconfWIN::registerCallback(DNSServiceRef sdref, const DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
{
  (void)sdref;    // Unused
  (void)flags;    // Unused
  (void)context;  // Unused

  if (errorCode == kDNSServiceErr_NoError)
  {
    if (flags & kDNSServiceFlagsAdd)
      CLog::Log(LOGDEBUG, "CZeroconfWIN: %s.%s%s now registered and active", name, regtype, domain);
    else
      CLog::Log(LOGDEBUG, "CZeroconfWIN: %s.%s%s registration removed", name, regtype, domain);
  }
  else if (errorCode == kDNSServiceErr_NameConflict)
     CLog::Log(LOGDEBUG, "CZeroconfWIN: %s.%s%s Name in use, please choose another", name, regtype, domain);
  else
    CLog::Log(LOGDEBUG, "CZeroconfWIN: %s.%s%s error code %d", name, regtype, domain, errorCode);

}


std::string CZeroconfWIN::assemblePublishedName(const std::string& fcr_given_name)
{
  std::stringstream ss;
  ss << fcr_given_name << '@';

  // get our hostname
  char lp_hostname[256];
  if (gethostname(lp_hostname, sizeof(lp_hostname)))
  {
    //TODO
    CLog::Log(LOGERROR, "CZeroconfWIN::assemblePublishedName: could not get hostname.. hm... waaaah! PANIC!");
    ss << "DummyThatCantResolveItsName";
  }
  else
  {
    ss << lp_hostname;
  }
  return ss.str();
}