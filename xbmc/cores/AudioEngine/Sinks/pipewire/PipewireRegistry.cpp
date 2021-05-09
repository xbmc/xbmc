/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireRegistry.h"

#include "cores/AudioEngine/Sinks/pipewire/Pipewire.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireNode.h"
#include "utils/log.h"

#include <stdexcept>

#include <pipewire/keys.h>
#include <pipewire/node.h>
#include <pipewire/type.h>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

CPipewireRegistry::CPipewireRegistry(pw_core* core)
{
  m_registry.reset(pw_core_get_registry(core, PW_VERSION_REGISTRY, 0));
  if (!m_registry)
  {
    CLog::Log(LOGERROR, "CPipewireRegistry: failed to create registry: {}", strerror(errno));
    throw std::runtime_error("CPipewireRegistry: failed to create registry");
  }
}

void CPipewireRegistry::AddListener(void* userdata)
{
  pw_registry_add_listener(m_registry.get(), &m_registryListener, &m_registryEvents, userdata);
}

void CPipewireRegistry::OnGlobalAdded(void* userdata,
                                      uint32_t id,
                                      uint32_t permissions,
                                      const char* type,
                                      uint32_t version,
                                      const struct spa_dict* props)
{
  auto pipewire = reinterpret_cast<CPipewire*>(userdata);

  if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0)
  {
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

    auto& globals = pipewire->GetGlobals();

    globals[id] = std::make_unique<CPipewire::global>();
    globals[id]->name = std::string(name);
    globals[id]->description = std::string(desc);
    globals[id]->id = id;
    globals[id]->permissions = permissions;
    globals[id]->type = std::string(type);
    globals[id]->version = version;
    globals[id]->properties.reset(pw_properties_new_dict(props));
    globals[id]->proxy = std::make_unique<CPipewireNode>(pipewire->GetRegistry()->Get(), id, type);
    globals[id]->proxy->AddListener(userdata);
  }
}

void CPipewireRegistry::OnGlobalRemoved(void* userdata, uint32_t id)
{
  auto pipewire = reinterpret_cast<CPipewire*>(userdata);
  auto& globals = pipewire->GetGlobals();

  auto global = globals.find(id);
  if (global != globals.end())
  {
    CLog::Log(LOGDEBUG, "CPipewireRegistry::{} - id={} type={}", __FUNCTION__, id,
              global->second->type);

    globals.erase(global);
  }
}

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
