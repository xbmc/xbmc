/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include <pipewire/core.h>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

class CPipewireProxy
{
public:
  CPipewireProxy() = delete;
  virtual ~CPipewireProxy();

  virtual void AddListener(void* userdata);

protected:
  explicit CPipewireProxy(pw_registry* registry, uint32_t id, const char* type, uint32_t version);

  struct PipewireProxyDeleter
  {
    void operator()(pw_proxy* p) { pw_proxy_destroy(p); }
  };

  std::unique_ptr<pw_proxy, PipewireProxyDeleter> m_proxy;

private:
  static void Bound(void* userdata, uint32_t id);
  static void Removed(void* userdata);

  static pw_proxy_events CreateProxyEvents();

  const pw_proxy_events m_proxyEvents;

  spa_hook m_proxyListener;
};

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
