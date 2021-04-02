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

#include <pipewire/properties.h>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

class CPipewireThreadLoop;
class CPipewireContext;
class CPipewireCore;
class CPipewireRegistry;
class CPipewireStream;

class CPipewire
{
public:
  CPipewire();
  ~CPipewire();

  bool Start();

  CPipewireThreadLoop* GetThreadLoop() { return m_loop.get(); }
  CPipewireContext* GetContext() { return m_context.get(); }
  CPipewireCore* GetCore() { return m_core.get(); }
  CPipewireRegistry* GetRegistry() { return m_registry.get(); }
  std::shared_ptr<CPipewireStream>& GetStream() { return m_stream; }

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
  std::map<uint32_t, std::unique_ptr<global>> m_globals;

  std::unique_ptr<CPipewireThreadLoop> m_loop;
  std::unique_ptr<CPipewireContext> m_context;
  std::unique_ptr<CPipewireCore> m_core;
  std::unique_ptr<CPipewireRegistry> m_registry;
  std::shared_ptr<CPipewireStream> m_stream;
};

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
