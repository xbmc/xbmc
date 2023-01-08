/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Sinks/pipewire/PipewireProxy.h"

#include <map>
#include <memory>
#include <string>

#include <pipewire/core.h>
#include <pipewire/properties.h>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

class CPipewireCore;

class CPipewireRegistry
{
public:
  explicit CPipewireRegistry(CPipewireCore& core);
  CPipewireRegistry() = delete;
  ~CPipewireRegistry() = default;

  pw_registry* Get() const { return m_registry.get(); }

  CPipewireCore& GetCore() const { return m_core; }

  void AddListener(void* userdata);

  struct PipewirePropertiesDeleter
  {
    void operator()(pw_properties* p) { pw_properties_free(p); }
  };

  struct global
  {
    std::string name;
    std::string description;
    uint32_t id;
    uint32_t permissions;
    std::string type;
    uint32_t version;
    std::unique_ptr<pw_properties, PipewirePropertiesDeleter> properties;
    std::unique_ptr<CPipewireProxy> proxy;
  };

  std::map<uint32_t, std::unique_ptr<global>>& GetGlobals() { return m_globals; }

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

  std::map<uint32_t, std::unique_ptr<global>> m_globals;
};

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
