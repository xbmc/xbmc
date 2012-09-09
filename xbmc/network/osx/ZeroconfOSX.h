#pragma once
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

#include <memory>

#include "network/Zeroconf.h"
#include "threads/CriticalSection.h"

#include <CoreFoundation/CoreFoundation.h>
#if defined(TARGET_DARWIN_OSX)
  #include <CoreServices/CoreServices.h>
#else
  #include <CFNetwork/CFNetServices.h>
#endif

class CZeroconfOSX : public CZeroconf
{
public:
  CZeroconfOSX();
  ~CZeroconfOSX();
protected:
  //implement base CZeroConf interface
  bool doPublishService(const std::string& fcr_identifier,
                        const std::string& fcr_type,
                        const std::string& fcr_name,
                        unsigned int f_port,
                        std::map<std::string, std::string> txt);

  bool doRemoveService(const std::string& fcr_ident);

  virtual void doStop();

private:
  static void registerCallback(CFNetServiceRef theService, CFStreamError* error, void* info);
  void cancelRegistration(CFNetServiceRef theService);

  //CF runloop ref; we're using main-threads runloop
  CFRunLoopRef m_runloop;

  //lock + data (accessed from runloop(main thread) + the rest)
  CCriticalSection m_data_guard;
  typedef std::map<std::string, CFNetServiceRef> tServiceMap;
  tServiceMap m_services;
};
