/*
 *  Copyright (C) 2017 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "network/Zeroconf.h"

#include <androidjni/NsdManager.h>
#include <androidjni/NsdServiceInfo.h>

#include "threads/CriticalSection.h"
#include "platform/android/activity/JNIXBMCNsdManagerRegistrationListener.h"

class CZeroconfAndroid : public CZeroconf
{
public:
  CZeroconfAndroid();
  virtual ~CZeroconfAndroid();

  // CZeroconf interface
protected:
  bool doPublishService(const std::string& fcr_identifier, const std::string& fcr_type, const std::string& fcr_name, unsigned int f_port, const std::vector<std::pair<std::string, std::string> >& txt);
  bool doForceReAnnounceService(const std::string& fcr_identifier);
  bool doRemoveService(const std::string& fcr_ident);
  void doStop();

private:
  jni::CJNINsdManager m_manager;

  //lock + data (accessed from runloop(main thread) + the rest)
  CCriticalSection m_data_guard;
  struct tServiceRef
  {
    jni::CJNINsdServiceInfo serviceInfo;
    jni::CJNIXBMCNsdManagerRegistrationListener registrationListener;
    int updateNumber;
  };
  typedef std::map<std::string, struct tServiceRef> tServiceMap;
  tServiceMap m_services;
};
