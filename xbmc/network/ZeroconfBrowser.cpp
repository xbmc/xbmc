/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h" //HAS_ZEROCONF define
#include "ZeroconfBrowser.h"
#include "settings/Settings.h"
#include <stdexcept>
#include "utils/log.h"

#ifdef _LINUX
#if !defined(TARGET_DARWIN)
#include "linux/ZeroconfBrowserAvahi.h"
#else
//on osx use the native implementation
#include "osx/ZeroconfBrowserOSX.h"
#endif
#elif defined(TARGET_WINDOWS)
#include "windows/ZeroconfBrowserWIN.h"
#endif

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "threads/Atomics.h"

#if !defined(HAS_ZEROCONF)
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
#ifdef HAS_FILESYSTEM_SMB
  AddServiceType("_smb._tcp.");
#endif
  AddServiceType("_ftp._tcp.");
  AddServiceType("_htsp._tcp.");
  AddServiceType("_daap._tcp.");
  AddServiceType("_webdav._tcp.");
#ifdef HAS_FILESYSTEM_NFS
  AddServiceType("_nfs._tcp.");  
#endif// HAS_FILESYSTEM_NFS
#ifdef HAS_FILESYSTEM_AFP
  AddServiceType("_afpovertcp._tcp.");   
#endif
  AddServiceType("_sftp-ssh._tcp.");
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
#if !defined(HAS_ZEROCONF)
      smp_instance = new CZeroconfBrowserDummy;
#else
#if defined(TARGET_DARWIN)
      smp_instance = new CZeroconfBrowserOSX;
#elif defined(_LINUX)
      smp_instance  = new CZeroconfBrowserAvahi;
#elif defined(TARGET_WINDOWS)
      smp_instance  = new CZeroconfBrowserWIN;
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
  m_domain(fcr_domain),
  m_port(0)
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

void CZeroconfBrowser::ZeroconfService::SetTxtRecords(const tTxtRecordMap& txt_records)
{
  m_txtrecords_map = txt_records;
  
  CLog::Log(LOGDEBUG,"CZeroconfBrowser: dump txt-records");
  for(tTxtRecordMap::const_iterator it = m_txtrecords_map.begin(); it != m_txtrecords_map.end(); ++it)
  {
    CLog::Log(LOGDEBUG,"CZeroconfBrowser:  key: %s value: %s",it->first.c_str(), it->second.c_str());
  }
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
