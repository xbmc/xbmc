/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireThreadLoop;
class CPipewireContext;
class CPipewireCore;
class CPipewireRegistry;

class CPipewire
{
public:
  static std::unique_ptr<CPipewire> Create();

  ~CPipewire();

  bool Start();

  CPipewireThreadLoop& GetThreadLoop() { return *m_loop; }
  CPipewireContext& GetContext() { return *m_context; }
  CPipewireCore& GetCore() { return *m_core; }
  CPipewireRegistry& GetRegistry() { return *m_registry; }

private:
  CPipewire();

  std::unique_ptr<CPipewireThreadLoop> m_loop;
  std::unique_ptr<CPipewireContext> m_context;
  std::unique_ptr<CPipewireCore> m_core;
  std::unique_ptr<CPipewireRegistry> m_registry;
};

} // namespace PIPEWIRE
} // namespace KODI
