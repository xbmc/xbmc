/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "Zeroconf.h"

#include "ServiceBroker.h"

#include <mutex>
#if defined(HAS_MDNS)
#include "mdns/ZeroconfMDNS.h"
#endif
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Atomics.h"
#include "threads/CriticalSection.h"
#include "utils/JobManager.h"

#if defined(TARGET_ANDROID)
#include "platform/android/network/ZeroconfAndroid.h"
#elif defined(TARGET_DARWIN)
//on osx use the native implementation
#include "platform/darwin/network/ZeroconfDarwin.h"
#elif defined(HAS_AVAHI)
#include "platform/linux/network/zeroconf/ZeroconfAvahi.h"
#endif

#include <cassert>
#include <utility>

#ifndef HAS_ZEROCONF
//dummy implementation used if no zeroconf is present
//should be optimized away
class CZeroconfDummy : public CZeroconf
{
  virtual bool doPublishService(const std::string&, const std::string&, const std::string&, unsigned int, const std::vector<std::pair<std::string, std::string> >&)
  {
    return false;
  }

  virtual bool doForceReAnnounceService(const std::string&){return false;}
  virtual bool doRemoveService(const std::string& fcr_ident){return false;}
  virtual void doStop(){}
};
#endif

std::atomic_flag CZeroconf::sm_singleton_guard = ATOMIC_FLAG_INIT;
CZeroconf* CZeroconf::smp_instance = 0;

CZeroconf::CZeroconf():mp_crit_sec(new CCriticalSection)
{
}

CZeroconf::~CZeroconf()
{
  delete mp_crit_sec;
}

bool CZeroconf::PublishService(const std::string& fcr_identifier,
                               const std::string& fcr_type,
                               const std::string& fcr_name,
                               unsigned int f_port,
                               std::vector<std::pair<std::string, std::string> > txt /* = std::vector<std::pair<std::string, std::string> >() */)
{
  std::unique_lock<CCriticalSection> lock(*mp_crit_sec);
  CZeroconf::PublishInfo info = {fcr_type, fcr_name, f_port, std::move(txt)};
  std::pair<tServiceMap::const_iterator, bool> ret = m_service_map.insert(std::make_pair(fcr_identifier, info));
  if(!ret.second) //identifier exists
    return false;
  if(m_started)
    CServiceBroker::GetJobManager()->AddJob(new CPublish(fcr_identifier, info), nullptr);

  //not yet started, so its just queued
  return true;
}

bool CZeroconf::RemoveService(const std::string& fcr_identifier)
{
  std::unique_lock<CCriticalSection> lock(*mp_crit_sec);
  tServiceMap::iterator it = m_service_map.find(fcr_identifier);
  if(it == m_service_map.end())
    return false;
  m_service_map.erase(it);
  if(m_started)
    return doRemoveService(fcr_identifier);
  else
    return true;
}

bool CZeroconf::ForceReAnnounceService(const std::string& fcr_identifier)
{
  if (HasService(fcr_identifier) && m_started)
  {
    return doForceReAnnounceService(fcr_identifier);
  }
  return false;
}

bool CZeroconf::HasService(const std::string& fcr_identifier) const
{
  return (m_service_map.find(fcr_identifier) != m_service_map.end());
}

bool CZeroconf::Start()
{
  std::unique_lock<CCriticalSection> lock(*mp_crit_sec);
  if(!IsZCdaemonRunning())
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetBool(CSettings::SETTING_SERVICES_ZEROCONF, false);
    if (settings->GetBool(CSettings::SETTING_SERVICES_AIRPLAY))
      settings->SetBool(CSettings::SETTING_SERVICES_AIRPLAY, false);
    return false;
  }
  if(m_started)
    return true;
  m_started = true;

  CServiceBroker::GetJobManager()->AddJob(new CPublish(m_service_map), nullptr);
  return true;
}

void CZeroconf::Stop()
{
  std::unique_lock<CCriticalSection> lock(*mp_crit_sec);
  if(!m_started)
    return;
  doStop();
  m_started = false;
}

CZeroconf*  CZeroconf::GetInstance()
{
  CAtomicSpinLock lock(sm_singleton_guard);
  if(!smp_instance)
  {
#ifndef HAS_ZEROCONF
    smp_instance = new CZeroconfDummy;
#else
#if defined(TARGET_DARWIN)
    smp_instance = new CZeroconfDarwin;
#elif defined(HAS_AVAHI)
    smp_instance  = new CZeroconfAvahi;
#elif defined(TARGET_ANDROID)
    smp_instance  = new CZeroconfAndroid;
#elif defined(HAS_MDNS)
    smp_instance  = new CZeroconfMDNS;
#endif
#endif
  }
  assert(smp_instance);
  return smp_instance;
}

void CZeroconf::ReleaseInstance()
{
  CAtomicSpinLock lock(sm_singleton_guard);
  delete smp_instance;
  smp_instance = 0;
}

CZeroconf::CPublish::CPublish(const std::string& fcr_identifier, const PublishInfo& pubinfo)
{
  m_servmap.insert(std::make_pair(fcr_identifier, pubinfo));
}

CZeroconf::CPublish::CPublish(const tServiceMap& servmap)
  : m_servmap(servmap)
{
}

bool CZeroconf::CPublish::DoWork()
{
  for (const auto& it : m_servmap)
    CZeroconf::GetInstance()->doPublishService(it.first, it.second.type, it.second.name,
                                               it.second.port, it.second.txt);

  return true;
}
