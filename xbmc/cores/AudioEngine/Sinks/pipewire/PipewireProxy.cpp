/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireProxy.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "utils/log.h"

#include <stdexcept>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

CPipewireProxy::CPipewireProxy(pw_registry* registry,
                               uint32_t id,
                               const char* type,
                               uint32_t version)
{
  m_proxy.reset(reinterpret_cast<pw_proxy*>(pw_registry_bind(registry, id, type, version, 0)));
  if (!m_proxy)
  {
    CLog::Log(LOGERROR, "CPipewireProxy: failed to create proxy: {}", strerror(errno));
    throw std::runtime_error("CPipewireProxy: failed to create proxy");
  }
}

CPipewireProxy::~CPipewireProxy()
{
  spa_hook_remove(&m_proxyListener);
}

void CPipewireProxy::AddListener(void* userdata)
{
  pw_proxy_add_listener(m_proxy.get(), &m_proxyListener, &m_proxyEvents, nullptr);
}

void CPipewireProxy::Bound(void* userdata, uint32_t id)
{
  CLog::Log(LOGDEBUG, "CPipewireProxy::{} - id={}", __FUNCTION__, id);

  auto AE = CServiceBroker::GetActiveAE();
  if (AE)
    AE->DeviceCountChange("PIPEWIRE");
}

void CPipewireProxy::Removed(void* userdata)
{
  CLog::Log(LOGDEBUG, "CPipewireProxy::{}", __FUNCTION__);

  auto AE = CServiceBroker::GetActiveAE();
  if (AE)
    AE->DeviceCountChange("PIPEWIRE");
}

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
