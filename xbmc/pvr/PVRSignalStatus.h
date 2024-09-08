/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_channels.h"

#include <string>

namespace PVR
{

class CPVRSignalStatus
{
public:
  CPVRSignalStatus() = default;

  CPVRSignalStatus(const std::string& adapterName, const std::string& adapterStatus)
    : m_adapterName(adapterName), m_adapterStatus(adapterStatus)
  {
  }

  explicit CPVRSignalStatus(const PVR_SIGNAL_STATUS& status)
    : m_status(status),
      m_adapterName(status.strAdapterName ? status.strAdapterName : ""),
      m_adapterStatus(status.strAdapterStatus ? status.strAdapterStatus : ""),
      m_serviceName(status.strServiceName ? status.strServiceName : ""),
      m_providerName(status.strProviderName ? status.strProviderName : ""),
      m_muxName(status.strMuxName ? status.strMuxName : "")
  {
  }

  virtual ~CPVRSignalStatus() = default;

  int Signal() const { return m_status.iSignal; }
  int SNR() const { return m_status.iSNR; }
  long UNC() const { return m_status.iUNC; }
  long BER() const { return m_status.iBER; }
  const std::string& AdapterName() const { return m_adapterName; }
  const std::string& AdapterStatus() const { return m_adapterStatus; }
  const std::string& ServiceName() const { return m_serviceName; }
  const std::string& ProviderName() const { return m_providerName; }
  const std::string& MuxName() const { return m_muxName; }

private:
  PVR_SIGNAL_STATUS m_status{};
  std::string m_adapterName;
  std::string m_adapterStatus;
  std::string m_serviceName;
  std::string m_providerName;
  std::string m_muxName;
};

} // namespace PVR
