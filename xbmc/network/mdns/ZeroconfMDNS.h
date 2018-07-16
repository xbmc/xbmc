/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Zeroconf.h"
#include "threads/CriticalSection.h"
#include <dns_sd.h>
#include "threads/Thread.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CZeroconfMDNS : public CZeroconf,public CThread
{
public:
  CZeroconfMDNS();
  ~CZeroconfMDNS();

protected:

  //CThread interface
  void Process();

  //implement base CZeroConf interface
  bool doPublishService(const std::string& fcr_identifier,
                        const std::string& fcr_type,
                        const std::string& fcr_name,
                        unsigned int f_port,
                        const std::vector<std::pair<std::string, std::string> >& txt);

  bool doForceReAnnounceService(const std::string& fcr_identifier);
  bool doRemoveService(const std::string& fcr_ident);

  virtual void doStop();

  bool IsZCdaemonRunning();

  void ProcessResults();

private:

  static void DNSSD_API registerCallback(DNSServiceRef sdref,
                                         const DNSServiceFlags flags,
                                         DNSServiceErrorType errorCode,
                                         const char *name,
                                         const char *regtype,
                                         const char *domain,
                                         void *context);


  //lock + data (accessed from runloop(main thread) + the rest)
  CCriticalSection m_data_guard;
  struct tServiceRef
  {
    DNSServiceRef serviceRef;
    TXTRecordRef txtRecordRef;
    int updateNumber;
  };
  typedef std::map<std::string, struct tServiceRef> tServiceMap;
  tServiceMap m_services;
  DNSServiceRef m_service;
};
