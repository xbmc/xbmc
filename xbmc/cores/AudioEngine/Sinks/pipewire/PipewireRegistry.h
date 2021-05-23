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

class CPipewireRegistry
{
public:
  explicit CPipewireRegistry(pw_core* core);
  CPipewireRegistry() = delete;
  ~CPipewireRegistry() = default;

  pw_registry* Get() const { return m_registry.get(); }

  void AddListener(void* userdata);

private:
  static void OnGlobalAdded(void* userdata,
                            uint32_t id,
                            uint32_t permissions,
                            const char* type,
                            uint32_t version,
                            const struct spa_dict* props);
  static void OnGlobalRemoved(void* userdata, uint32_t id);

  const pw_registry_events m_registryEvents = {
      .version = PW_VERSION_REGISTRY_EVENTS,
      .global = OnGlobalAdded,
      .global_remove = OnGlobalRemoved,
  };

  spa_hook m_registryListener;
  struct PipewireRegistryDeleter
  {
    void operator()(pw_registry* p) { pw_proxy_destroy(reinterpret_cast<pw_proxy*>(p)); }
  };

  std::unique_ptr<pw_registry, PipewireRegistryDeleter> m_registry;
};

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
