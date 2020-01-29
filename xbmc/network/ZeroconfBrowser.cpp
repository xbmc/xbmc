/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "ZeroconfBrowser.h"
#include <stdexcept>
#include "utils/log.h"
#include <cassert>

#if defined (HAS_AVAHI)
#include "platform/linux/network/zeroconf/ZeroconfBrowserAvahi.h"
#elif defined(TARGET_DARWIN)
//on osx use the native implementation
#include "platform/darwin/network/ZeroconfBrowserDarwin.h"
#elif defined(TARGET_ANDROID)
#include "platform/android/network/ZeroconfBrowserAndroid.h"
#elif defined(HAS_MDNS)
#include "mdns/ZeroconfBrowserMDNS.h"
#endif

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "threads/Atomics.h"

#if !defined(HAS_ZEROCONF)
//dummy implementation used if no zeroconf is present
//should be optimized away
class CZeroconfBrowserDummy : public CZeroconfBrowser
{
  virtual bool doAddServiceType(const std::string&){return false;}
  virtual bool doRemoveServiceType(const std::string&){return false;}
  virtual std::vector<ZeroconfService> doGetFoundServices(){return std::vector<ZeroconfService>();}
  virtual bool doResolveService(ZeroconfService&, double){return false;}
};
#endif

std::atomic_flag CZeroconfBrowser::sm_singleton_guard = ATOMIC_FLAG_INIT;
CZeroconfBrowser* CZeroconfBrowser::smp_instance = 0;

CZeroconfBrowser::CZeroconfBrowser():mp_crit_sec(new CCriticalSection)
{
#ifdef HAS_FILESYSTEM_SMB
  AddServiceType("_smb._tcp.");
#endif
  AddServiceType("_ftp._tcp.");
  AddServiceType("_webdav._tcp.");
#ifdef HAS_FILESYSTEM_NFS
  AddServiceType("_nfs._tcp.");
#endif// HAS_FILESYSTEM_NFS
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
  for (const auto& it : m_services)
    doAddServiceType(it);
}

void CZeroconfBrowser::Stop()
{
  CSingleLock lock(*mp_crit_sec);
  if(!m_started)
    return;
  for (const auto& it : m_services)
    RemoveServiceType(it);
  m_started = false;
}

bool CZeroconfBrowser::AddServiceType(const std::string& fcr_service_type /*const std::string& domain*/ )
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

bool CZeroconfBrowser::RemoveServiceType(const std::string& fcr_service_type)
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
      smp_instance = new CZeroconfBrowserDarwin;
#elif defined(HAS_AVAHI)
      smp_instance  = new CZeroconfBrowserAvahi;
#elif defined(TARGET_ANDROID)
      // WIP
      smp_instance  = new CZeroconfBrowserAndroid;
#elif defined(HAS_MDNS)
      smp_instance  = new CZeroconfBrowserMDNS;
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


CZeroconfBrowser::ZeroconfService::ZeroconfService(const std::string& fcr_name, const std::string& fcr_type, const std::string& fcr_domain):
  m_name(fcr_name),
  m_domain(fcr_domain)
{
  SetType(fcr_type);
}
void CZeroconfBrowser::ZeroconfService::SetName(const std::string& fcr_name)
{
  m_name = fcr_name;
}

void CZeroconfBrowser::ZeroconfService::SetType(const std::string& fcr_type)
{
  if(fcr_type.empty())
    throw std::runtime_error("CZeroconfBrowser::ZeroconfService::SetType invalid type: "+ fcr_type);
  //make sure there's a "." as last char (differs for avahi and osx implementation of browsers)
  if(fcr_type[fcr_type.length() - 1] != '.')
    m_type = fcr_type + ".";
  else
    m_type = fcr_type;
}

void CZeroconfBrowser::ZeroconfService::SetDomain(const std::string& fcr_domain)
{
  m_domain = fcr_domain;
}

void CZeroconfBrowser::ZeroconfService::SetHostname(const std::string& fcr_hostname)
{
  m_hostname = fcr_hostname;
}

void CZeroconfBrowser::ZeroconfService::SetIP(const std::string& fcr_ip)
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
  for (const auto& it : m_txtrecords_map)
  {
    CLog::Log(LOGDEBUG, "CZeroconfBrowser:  key: %s value: %s", it.first.c_str(),
              it.second.c_str());
  }
}

std::string CZeroconfBrowser::ZeroconfService::toPath(const ZeroconfService& fcr_service)
{
  return fcr_service.m_type + '@' + fcr_service.m_domain + '@' + fcr_service.m_name;
}

CZeroconfBrowser::ZeroconfService CZeroconfBrowser::ZeroconfService::fromPath(const std::string& fcr_path)
{
  if( fcr_path.empty() )
    throw std::runtime_error("CZeroconfBrowser::ZeroconfService::fromPath input string empty!");

  size_t pos1 = fcr_path.find('@'); //first @
  size_t pos2 = fcr_path.find('@', pos1 + 1); //second

  if(pos1 == std::string::npos || pos2 == std::string::npos)
    throw std::runtime_error("CZeroconfBrowser::ZeroconfService::fromPath invalid input path");

  return ZeroconfService(
    fcr_path.substr(pos2 + 1, fcr_path.length()), //name
    fcr_path.substr(0, pos1), //type
    fcr_path.substr(pos1 + 1, pos2-(pos1+1)) //domain
    );
}
