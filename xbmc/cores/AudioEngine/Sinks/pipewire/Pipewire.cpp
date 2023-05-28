/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Pipewire.h"

#include "PipewireContext.h"
#include "PipewireCore.h"
#include "PipewireRegistry.h"
#include "PipewireThreadLoop.h"
#include "commons/ilog.h"
#include "utils/log.h"

#include <pipewire/pipewire.h>

using namespace std::chrono_literals;

using namespace KODI;
using namespace PIPEWIRE;

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

  m_registry.reset();
  m_core.reset();
  m_context.reset();
  m_loop.reset();

  pw_deinit();
}

bool CPipewire::Start()
{
  m_loop = std::make_unique<CPipewireThreadLoop>();

  m_context = std::make_unique<CPipewireContext>(*m_loop);

  PIPEWIRE::CLoopLockGuard lock(*m_loop);

  if (!m_loop->Start())
  {
    CLog::Log(LOGERROR, "Pipewire: failed to start threaded mainloop: {}", strerror(errno));
    return false;
  }

  try
  {
    m_core = std::make_unique<CPipewireCore>(*m_context);
  }
  catch (std::exception& e)
  {
    CLog::Log(LOGERROR, "Pipewire: failed to connect to server");
    return false;
  }

  CLog::Log(LOGINFO, "Pipewire: connected to server");

  m_registry = std::make_unique<CPipewireRegistry>(*m_core);

  m_core->Sync();

  int ret = m_loop->Wait(5s);
  if (ret == -ETIMEDOUT)
  {
    CLog::Log(LOGDEBUG, "Pipewire: timed out out waiting for synchronization");
    return false;
  }

  return true;
}

std::unique_ptr<CPipewire> CPipewire::Create()
{
  struct PipewireMaker : public CPipewire
  {
    using CPipewire::CPipewire;
  };

  try
  {
    auto pipewire = std::make_unique<PipewireMaker>();

    if (!pipewire->Start())
      return {};

    return pipewire;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGWARNING, "Pipewire: Exception in 'Create': {}", e.what());
    return {};
  }
}
