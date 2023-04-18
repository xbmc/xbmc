/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include <pipewire/core.h>

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireCore;
class CPipewireGlobal;

class CPipewireRegistry
{
public:
  explicit CPipewireRegistry(CPipewireCore& core);
  CPipewireRegistry() = delete;
  ~CPipewireRegistry();

  pw_registry* Get() const { return m_registry.get(); }

  CPipewireCore& GetCore() const { return m_core; }

  std::map<uint32_t, std::unique_ptr<CPipewireGlobal>>& GetGlobals() { return m_globals; }

private:
  static void OnGlobalAdded(void* userdata,
                            uint32_t id,
                            uint32_t permissions,
                            const char* type,
                            uint32_t version,
                            const struct spa_dict* props);
  static void OnGlobalRemoved(void* userdata, uint32_t id);

  static pw_registry_events CreateRegistryEvents();

  CPipewireCore& m_core;

  const pw_registry_events m_registryEvents;

  spa_hook m_registryListener;
  struct PipewireRegistryDeleter
  {
    void operator()(pw_registry* p) { pw_proxy_destroy(reinterpret_cast<pw_proxy*>(p)); }
  };

  std::unique_ptr<pw_registry, PipewireRegistryDeleter> m_registry;

  std::map<uint32_t, std::unique_ptr<CPipewireGlobal>> m_globals;
};

} // namespace PIPEWIRE
} // namespace KODI
