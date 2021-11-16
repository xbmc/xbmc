/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireCore.h"

#include "cores/AudioEngine/Sinks/pipewire/Pipewire.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireThreadLoop.h"
#include "utils/log.h"

#include <stdexcept>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

CPipewireCore::CPipewireCore(pw_context* context) : m_coreEvents(CreateCoreEvents())
{
  m_core.reset(pw_context_connect(context, nullptr, 0));
  if (!m_core)
  {
    CLog::Log(LOGERROR, "CPipewireCore: failed to create core: {}", strerror(errno));
    throw std::runtime_error("CPipewireCore: failed to create core");
  }
}

CPipewireCore::~CPipewireCore()
{
  spa_hook_remove(&m_coreListener);
}

void CPipewireCore::AddListener(void* userdata)
{
  pw_core_add_listener(m_core.get(), &m_coreListener, &m_coreEvents, userdata);
}

void CPipewireCore::Sync()
{
  m_sync = pw_core_sync(m_core.get(), 0, m_sync);
}

void CPipewireCore::OnCoreDone(void* userdata, uint32_t id, int seq)
{
  auto pipewire = reinterpret_cast<CPipewire*>(userdata);
  auto core = pipewire->GetCore();
  auto loop = pipewire->GetThreadLoop();

  if (core->GetSync() == seq)
    loop->Signal(false);
}

pw_core_events CPipewireCore::CreateCoreEvents()
{
  pw_core_events coreEvents = {};
  coreEvents.version = PW_VERSION_CORE_EVENTS;
  coreEvents.done = OnCoreDone;

  return coreEvents;
}

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
