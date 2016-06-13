#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "utils/Job.h"

class CCriticalSection;
/// this class provides support for zeroconf
/// while the different zeroconf implementations have asynchronous APIs
/// this class hides it and provides only few ways to interact
/// with the services. If more control is needed, feel
/// free to add it. The main purpose currently is to provide an easy
/// way to publish services in the different StartXXX/StopXXX methods
/// in CApplication
//! @todo Make me safe for use in static initialization. CritSec is a static member :/
//!       use e.g. loki's singleton implementation to make do it properly
class CZeroconf
{
public:

  //tries to publish this service via zeroconf
  //fcr_identifier can be used to stop or reannounce this service later
  //fcr_type is the zeroconf service type to publish (e.g. _http._tcp for webserver)
  //fcr_name is the name of the service to publish. The hostname is currently automatically appended
  //         and used for name collisions. e.g. XBMC would get published as fcr_name@Martn or, after collision fcr_name@Martn-2
  //f_port port of the service to publish
  // returns false if fcr_identifier was already present
  bool PublishService(const std::string& fcr_identifier,
                      const std::string& fcr_type,
                      const std::string& fcr_name,
                      unsigned int f_port,
                      std::vector<std::pair<std::string, std::string> > txt /*= std::vector<std::pair<std::string, std::string> >()*/);
  
  //tries to rebroadcast that service on the network without removing/readding
  //this can be achieved by changing a fake txt record. Implementations should
  //implement it by doing so.
  //
  //fcr_identifier - the identifier of the already published service which should be reannounced
  // returns true on successfull reannonuce - false if this service isn't published yet
  bool ForceReAnnounceService(const std::string& fcr_identifier);

  ///removes the specified service
  ///returns false if fcr_identifier does not exist
  bool RemoveService(const std::string& fcr_identifier);

  ///returns true if fcr_identifier exists
  bool HasService(const std::string& fcr_identifier) const;

  //starts publishing
  //services that were added with PublishService(...) while Zeroconf wasn't
  //started, get published now.
  bool Start();

  // unpublishs all services (but keeps them stored in this class)
  // a call to Start() will republish them
  void Stop();

  // class methods
  // access to singleton; singleton gets created on call if not existent
  // if zeroconf is disabled (!HAS_ZEROCONF), this will return a dummy implementation that
  // just does nothings, otherwise the platform specific one
  static CZeroconf* GetInstance();
  // release the singleton; (save to call multiple times)
  static void   ReleaseInstance();
  // returns false if ReleaseInstance() was called befores
  static bool   IsInstantiated() { return  smp_instance != 0; }
  // win32: process results from the bonjour daemon
  virtual void  ProcessResults() {}
  // returns if the service is started and services are announced
  bool IsStarted() { return m_started; }

protected:
  //methods to implement for concrete implementations
  //publishs this service
  virtual bool doPublishService(const std::string& fcr_identifier,
                                const std::string& fcr_type,
                                const std::string& fcr_name,
                                unsigned int f_port,
                                const std::vector<std::pair<std::string, std::string> >& txt) = 0;

  //methods to implement for concrete implementations
  //update this service
  virtual bool doForceReAnnounceService(const std::string& fcr_identifier) = 0;
  
  //removes the service if published
  virtual bool doRemoveService(const std::string& fcr_ident) = 0;

  //removes all services (short hand for "for i in m_service_map doRemoveService(i)")
  virtual void doStop() = 0;

  // return true if the zeroconf daemon is running
  virtual bool IsZCdaemonRunning() { return  true; }

protected:
  //singleton: we don't want to get instantiated nor copied or deleted from outside
  CZeroconf();
  CZeroconf(const CZeroconf&);
  virtual ~CZeroconf();

private:
  struct PublishInfo{
    std::string type;
    std::string name;
    unsigned int port;
    std::vector<std::pair<std::string, std::string> > txt;
  };

  //protects data
  CCriticalSection* mp_crit_sec;
  typedef std::map<std::string, PublishInfo> tServiceMap;
  tServiceMap m_service_map;
  bool m_started;

  //protects singleton creation/destruction
  static long sm_singleton_guard;
  static CZeroconf* smp_instance;

  class CPublish : public CJob
  {
  public:
    CPublish(const std::string& fcr_identifier, const PublishInfo& pubinfo);
    CPublish(const tServiceMap& servmap);

    bool DoWork();

  private:
    tServiceMap m_servmap;
  };
};
