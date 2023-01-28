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

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireContext;

class CPipewireCore
{
public:
  explicit CPipewireCore(CPipewireContext& context);
  CPipewireCore() = delete;
  ~CPipewireCore();

  pw_core* Get() const { return m_core.get(); }

  CPipewireContext& GetContext() const { return m_context; }

  void Sync();
  int GetSync() const { return m_sync; }

private:
  static void OnCoreDone(void* userdata, uint32_t id, int seq);

  static pw_core_events CreateCoreEvents();

  CPipewireContext& m_context;

  const pw_core_events m_coreEvents;

  spa_hook m_coreListener;

  struct PipewireCoreDeleter
  {
    void operator()(pw_core* p) { pw_core_disconnect(p); }
  };

  std::unique_ptr<pw_core, PipewireCoreDeleter> m_core;

  int m_sync;
};

} // namespace PIPEWIRE
} // namespace KODI
