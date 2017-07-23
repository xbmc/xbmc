#pragma once
/*
 *      Copyright (C) 2017 Christian Browet
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
