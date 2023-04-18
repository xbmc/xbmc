/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireRegistry.h"

#include "PipewireCore.h"
#include "PipewireGlobal.h"
#include "PipewireNode.h"
#include "utils/log.h"

#include <stdexcept>

#include <pipewire/keys.h>
#include <pipewire/node.h>
#include <pipewire/type.h>

using namespace KODI;
using namespace PIPEWIRE;

CPipewireRegistry::CPipewireRegistry(CPipewireCore& core)
  : m_core(core), m_registryEvents(CreateRegistryEvents())
{
  m_registry.reset(pw_core_get_registry(core.Get(), PW_VERSION_REGISTRY, 0));
  if (!m_registry)
  {
    CLog::Log(LOGERROR, "CPipewireRegistry: failed to create registry: {}", strerror(errno));
    throw std::runtime_error("CPipewireRegistry: failed to create registry");
  }

  pw_registry_add_listener(m_registry.get(), &m_registryListener, &m_registryEvents, this);
}

CPipewireRegistry::~CPipewireRegistry() = default;

void CPipewireRegistry::OnGlobalAdded(void* userdata,
                                      uint32_t id,
                                      uint32_t permissions,
                                      const char* type,
                                      uint32_t version,
                                      const struct spa_dict* props)
{
  auto& registry = *reinterpret_cast<CPipewireRegistry*>(userdata);

  if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
    return;

  const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
  if (!mediaClass)
    return;

  if (strcmp(mediaClass, "Audio/Sink") != 0)
    return;

  const char* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
  if (!name)
    return;

  const char* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
  if (!desc)
    return;

  auto properties =
      std::unique_ptr<pw_properties, PipewirePropertiesDeleter>(pw_properties_new_dict(props));

  auto node = std::make_unique<CPipewireNode>(registry, id, type);

  auto& globals = registry.GetGlobals();

  globals[id] = std::make_unique<CPipewireGlobal>();
  globals[id]
      ->SetName(name)
      .SetDescription(desc)
      .SetID(id)
      .SetPermissions(permissions)
      .SetType(type)
      .SetVersion(version)
      .SetProperties(std::move(properties))
      .SetNode(std::move(node));
}

void CPipewireRegistry::OnGlobalRemoved(void* userdata, uint32_t id)
{
  auto& registry = *reinterpret_cast<CPipewireRegistry*>(userdata);
  auto& globals = registry.GetGlobals();

  auto it = globals.find(id);
  if (it != globals.end())
  {
    const auto& [globalId, global] = *it;
    CLog::Log(LOGDEBUG, "CPipewireRegistry::{} - id={} type={}", __FUNCTION__, id,
              global->GetType());

    globals.erase(it);
  }
}

pw_registry_events CPipewireRegistry::CreateRegistryEvents()
{
  pw_registry_events registryEvents = {};
  registryEvents.version = PW_VERSION_REGISTRY_EVENTS;
  registryEvents.global = OnGlobalAdded;
  registryEvents.global_remove = OnGlobalRemoved;

  return registryEvents;
}
