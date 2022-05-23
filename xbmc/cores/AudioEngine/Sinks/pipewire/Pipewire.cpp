/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Pipewire.h"

#include "cores/AudioEngine/Sinks/pipewire/PipewireContext.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireCore.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireRegistry.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireThreadLoop.h"
#include "utils/log.h"

#include <pipewire/pipewire.h>

using namespace std::chrono_literals;

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

CPipewire::CPipewire()
{
  pw_init(nullptr, nullptr);
}

CPipewire::~CPipewire()
{
  if (m_loop)
  {
    m_loop->Unlock();
    m_loop->Stop();
  }

  m_stream.reset();
  m_registry.reset();
  m_core.reset();
  m_context.reset();
  m_loop.reset();

  pw_deinit();
}

bool CPipewire::Start()
{
  m_loop = std::make_unique<CPipewireThreadLoop>();

  m_context = std::make_unique<CPipewireContext>(m_loop->Get());

  m_loop->Lock();

  if (!m_loop->Start())
  {
    CLog::Log(LOGERROR, "Pipewire: failed to start threaded mainloop: {}", strerror(errno));
    return false;
  }

  m_core = std::make_unique<CPipewireCore>(m_context->Get());
  m_core->AddListener(this);

  m_registry = std::make_unique<CPipewireRegistry>(m_core->Get());
  m_registry->AddListener(this);

  m_core->Sync();

  int ret = m_loop->Wait(5s);

  m_loop->Unlock();

  if (ret == -ETIMEDOUT)
  {
    CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - timed out out waiting for synchronization",
              __FUNCTION__);
    return false;
  }

  return true;
}

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
