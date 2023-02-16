/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireProxy.h"

#include "PipewireRegistry.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "utils/log.h"

#include <stdexcept>

using namespace KODI;
using namespace PIPEWIRE;

CPipewireProxy::CPipewireProxy(CPipewireRegistry& registry,
                               uint32_t id,
                               const char* type,
                               uint32_t version)
  : m_registry(registry), m_proxyEvents(CreateProxyEvents())
{
  m_proxy.reset(
      reinterpret_cast<pw_proxy*>(pw_registry_bind(registry.Get(), id, type, version, 0)));
  if (!m_proxy)
  {
    CLog::Log(LOGERROR, "CPipewireProxy: failed to create proxy: {}", strerror(errno));
    throw std::runtime_error("CPipewireProxy: failed to create proxy");
  }

  pw_proxy_add_listener(m_proxy.get(), &m_proxyListener, &m_proxyEvents, this);
}

CPipewireProxy::~CPipewireProxy()
{
  spa_hook_remove(&m_proxyListener);
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

pw_proxy_events CPipewireProxy::CreateProxyEvents()
{
  pw_proxy_events proxyEvents = {};
  proxyEvents.version = PW_VERSION_PROXY_EVENTS;
  proxyEvents.bound = Bound;
  proxyEvents.removed = Removed;

  return proxyEvents;
}
