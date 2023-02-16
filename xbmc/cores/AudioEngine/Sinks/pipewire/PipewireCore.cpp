/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireCore.h"

#include "PipewireContext.h"
#include "PipewireThreadLoop.h"
#include "utils/log.h"

#include <stdexcept>

using namespace KODI;
using namespace PIPEWIRE;

CPipewireCore::CPipewireCore(CPipewireContext& context)
  : m_context(context), m_coreEvents(CreateCoreEvents())
{
  m_core.reset(pw_context_connect(context.Get(), nullptr, 0));
  if (!m_core)
  {
    CLog::Log(LOGERROR, "CPipewireCore: failed to create core: {}", strerror(errno));
    throw std::runtime_error("CPipewireCore: failed to create core");
  }

  pw_core_add_listener(m_core.get(), &m_coreListener, &m_coreEvents, this);
}

CPipewireCore::~CPipewireCore()
{
  spa_hook_remove(&m_coreListener);
}

void CPipewireCore::Sync()
{
  m_sync = pw_core_sync(m_core.get(), 0, m_sync);
}

void CPipewireCore::OnCoreDone(void* userdata, uint32_t id, int seq)
{
  auto& core = *reinterpret_cast<CPipewireCore*>(userdata);
  auto& loop = core.GetContext().GetThreadLoop();

  if (core.GetSync() == seq)
    loop.Signal(false);
}

pw_core_events CPipewireCore::CreateCoreEvents()
{
  pw_core_events coreEvents = {};
  coreEvents.version = PW_VERSION_CORE_EVENTS;
  coreEvents.done = OnCoreDone;

  return coreEvents;
}
