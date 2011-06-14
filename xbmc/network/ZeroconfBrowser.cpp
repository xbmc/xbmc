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
#include "system.h" //HAS_ZEROCONF define
#include "ZeroconfBrowser.h"
#include "settings/Settings.h"
#include <stdexcept>
#include "utils/log.h"

#ifdef _LINUX
#ifndef __APPLE__
#include "linux/ZeroconfBrowserAvahi.h"
#else
//on osx use the native implementation
#include "osx/ZeroconfBrowserOSX.h"
#endif
#endif

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "threads/Atomics.h"

#ifndef HAS_ZEROCONF
//dummy implementation used if no zeroconf is present
//should be optimized away
class CZeroconfBrowserDummy : public CZeroconfBrowser
{
  virtual bool doAddServiceType(const CStdString&){return false;}
  virtual bool doRemoveServiceType(const CStdString&){return false;}
  virtual std::vector<ZeroconfService> doGetFoundServices(){return std::vector<ZeroconfService>();}
  virtual bool doResolveService(ZeroconfService&, double){return false;}
};
#endif

long CZeroconfBrowser::sm_singleton_guard = 0;
CZeroconfBrowser* CZeroconfBrowser::smp_instance = 0;

CZeroconfBrowser::CZeroconfBrowser():mp_crit_sec(new CCriticalSection),m_started(false)
{
  AddServiceType("_smb._tcp.");
  AddServiceType("_ftp._tcp.");
  AddServiceType("_htsp._tcp.");
  AddServiceType("_daap._tcp.");
  AddServiceType("_webdav._tcp.");
}

CZeroconfBrowser::~CZeroconfBrowser()
{
  delete mp_crit_sec;
}

void CZeroconfBrowser::Start()
{
  CSingleLock lock(*mp_crit_sec);
  if(m_started)
    return;
  m_started = true;
  for(tServices::const_iterator it = m_services.begin(); it != m_services.end(); ++it)
    doAddServiceType(*it);
}

void CZeroconfBrowser::Stop()
{
  CSingleLock lock(*mp_crit_sec);
  if(!m_started)
    return;
  for(tServices::iterator it = m_services.begin(); it != m_services.end(); ++it)
    RemoveServiceType(*it);
  m_started = false;
}

bool CZeroconfBrowser::AddServiceType(const CStdString& fcr_service_type /*const CStdString& domain*/ )
{
  CSingleLock lock(*mp_crit_sec);
  std::pair<tServices::iterator, bool> ret = m_services.insert(fcr_service_type);
  if(!ret.second)
  {
    //service already in list
    return false;
  }
  if(m_started)
    return doAddServiceType(*ret.first);
  //not yet started, so its just queued
  return true;
}

bool CZeroconfBrowser::RemoveServiceType(const CStdString& fcr_service_type)
{
  CSingleLock lock(*mp_crit_sec);
  tServices::iterator ret = m_services.find(fcr_service_type);
  if(ret == m_services.end())
    return false;
  if(m_started)
    return doRemoveServiceType(fcr_service_type);
  //not yet started, so its just queued
  return true;
}

std::vector<CZeroconfBrowser::ZeroconfService> CZeroconfBrowser::GetFoundServices()
{
  CSingleLock lock(*mp_crit_sec);
  if(m_started)
    return doGetFoundServices();
  else
  {
    CLog::Log(LOGDEBUG, "CZeroconfBrowser::GetFoundServices asked for services without browser running");
    return std::vector<ZeroconfService>();
  }
}

bool CZeroconfBrowser::ResolveService(ZeroconfService& fr_service, double f_timeout)
{
  CSingleLock lock(*mp_crit_sec);
  if(m_started)
  {
    return doResolveService(fr_service, f_timeout);
  }
  CLog::Log(LOGDEBUG, "CZeroconfBrowser::GetFoundServices asked for services without browser running");
  return false;
}

CZeroconfBrowser*  CZeroconfBrowser::GetInstance()
{
  if(!smp_instance)
  {
    //use double checked locking
    CAtomicSpinLock lock(sm_singleton_guard);
    if(!smp_instance)
    {
#ifndef HAS_ZEROCONF
      smp_instance = new CZeroconfBrowserDummy;
#else
#ifdef __APPLE__
      smp_instance = new CZeroconfBrowserOSX;
#elif defined(_LINUX)
      smp_instance  = new CZeroconfBrowserAvahi;
#endif
#endif
    }
  }
  assert(smp_instance);
  return smp_instance;
}

void CZeroconfBrowser::ReleaseInstance()
{
  CAtomicSpinLock lock(sm_singleton_guard);
  delete smp_instance;
  smp_instance = 0;
}


CZeroconfBrowser::ZeroconfService::ZeroconfService():m_port(0){}

CZeroconfBrowser::ZeroconfService::ZeroconfService(const CStdString& fcr_name, const CStdString& fcr_type, const CStdString& fcr_domain):
  m_name(fcr_name),
  m_domain(fcr_domain)
{
  SetType(fcr_type);
}
void CZeroconfBrowser::ZeroconfService::SetName(const CStdString& fcr_name)
{
  m_name = fcr_name;
}

void CZeroconfBrowser::ZeroconfService::SetType(const CStdString& fcr_type)
{
  if(fcr_type.empty())
    throw std::runtime_error("CZeroconfBrowser::ZeroconfService::SetType invalid type: "+ fcr_type);
  //make sure there's a "." as last char (differs for avahi and osx implementation of browsers)
  if(fcr_type[fcr_type.length() - 1] != '.')
    m_type = fcr_type + ".";
  else
    m_type = fcr_type;
}

void CZeroconfBrowser::ZeroconfService::SetDomain(const CStdString& fcr_domain)
{
  m_domain = fcr_domain;
}

void CZeroconfBrowser::ZeroconfService::SetIP(const CStdString& fcr_ip)
{
  m_ip = fcr_ip;
}

void CZeroconfBrowser::ZeroconfService::SetPort(int f_port)
{
  m_port = f_port;
}

CStdString CZeroconfBrowser::ZeroconfService::toPath(const ZeroconfService& fcr_service)
{
  return CStdString(fcr_service.m_type + "@" + fcr_service.m_domain + "@" + fcr_service.m_name);
}

CZeroconfBrowser::ZeroconfService CZeroconfBrowser::ZeroconfService::fromPath(const CStdString& fcr_path)
{
  if( fcr_path.empty() )
    throw std::runtime_error("CZeroconfBrowser::ZeroconfService::fromPath input string empty!");

  int pos1 = fcr_path.Find('@'); //first @
  int pos2 = fcr_path.Find('@', pos1+1); //second

  if( pos1 == -1 || pos2 == -1 )
    throw std::runtime_error("CZeroconfBrowser::ZeroconfService::fromPath invalid input path");

  return ZeroconfService(
    fcr_path.substr(pos2 + 1, fcr_path.length()), //name
    fcr_path.substr(0, pos1), //type
    fcr_path.substr(pos1 + 1, pos2-(pos1+1)) //domain
    );
}
