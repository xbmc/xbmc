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

class CPipewireCore
{
public:
  explicit CPipewireCore(pw_context* context);
  CPipewireCore() = delete;
  ~CPipewireCore();

  pw_core* Get() const { return m_core.get(); }

  void AddListener(void* userdata);
  void Sync();
  int GetSync() const { return m_sync; }

private:
  static void OnCoreDone(void* userdata, uint32_t id, int seq);

  const pw_core_events m_coreEvents = {.version = PW_VERSION_CORE_EVENTS,
                                       .info = nullptr,
                                       .done = OnCoreDone,
                                       .ping = nullptr,
                                       .error = nullptr,
                                       .remove_id = nullptr,
                                       .bound_id = nullptr,
                                       .add_mem = nullptr,
                                       .remove_mem = nullptr};

  spa_hook m_coreListener;

  struct PipewireCoreDeleter
  {
    void operator()(pw_core* p) { pw_core_disconnect(p); }
  };

  std::unique_ptr<pw_core, PipewireCoreDeleter> m_core;

  int m_sync;
};

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
