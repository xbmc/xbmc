/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SMBWSDiscovery.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "network/IWSDiscovery.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "platform/posix/filesystem/SMBWSDiscoveryListener.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

using namespace std::chrono;
using namespace WSDiscovery;

namespace WSDiscovery
{
std::unique_ptr<IWSDiscovery> IWSDiscovery::GetInstance()
{
  return std::make_unique<WSDiscovery::CWSDiscoveryPosix>();
}

CWSDiscoveryPosix::CWSDiscoveryPosix()
{
  // Set our wsd_instance ID to seconds since epoch
  auto epochduration = system_clock::now().time_since_epoch();
  wsd_instance_id = epochduration.count() * system_clock::period::num / system_clock::period::den;

  m_WSDListenerUDP = std::make_unique<CWSDiscoveryListenerUDP>();
}

CWSDiscoveryPosix::~CWSDiscoveryPosix()
{
  StopServices();
}

bool CWSDiscoveryPosix::StopServices()
{
  m_WSDListenerUDP->Stop();

  return true;
}

bool CWSDiscoveryPosix::StartServices()
{
  m_WSDListenerUDP->Start();

  return true;
}

bool CWSDiscoveryPosix::IsRunning()
{
  return m_WSDListenerUDP->IsRunning();
}

bool CWSDiscoveryPosix::GetServerList(CFileItemList& items)
{
  {
    CSingleLock lock(m_critWSD);

    // delim1 used to strip protocol from xaddrs
    // delim2 used to strip anything past the port
    const std::string delim1 = "://";
    const std::string delim2 = ":";
    for (auto item : m_vecWSDInfo)
    {
      int found = item.xaddrs.find(delim1);
      if (found == std::string::npos)
        continue;

      std::string tmpxaddrs = item.xaddrs.substr(found + delim1.size());
      found = tmpxaddrs.find(delim2);
      // fallback incase xaddrs doesnt return back "GetMetadata" expected address format (delim2)
      if (found == std::string::npos)
      {
        found = tmpxaddrs.find("/");
      }
      std::string host = tmpxaddrs.substr(0, found);

      CFileItemPtr pItem = std::make_shared<CFileItem>(host);
      pItem->SetPath("smb://" + host + '/');
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
  }
  return true;
}

void CWSDiscoveryPosix::SetItems(std::vector<wsd_req_info> entries)
{
  {
    CSingleLock lock(m_critWSD);
    m_vecWSDInfo = entries;
  }
}
} // namespace WSDiscovery
