/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireContext.h"

#include "cores/AudioEngine/Sinks/pipewire/PipewireThreadLoop.h"
#include "utils/log.h"

#include <stdexcept>

namespace AE
{
namespace SINK
{
namespace PIPEWIRE
{

CPipewireContext::CPipewireContext(CPipewireThreadLoop& loop) : m_threadLoop(loop)
{
  m_context.reset(pw_context_new(loop.Get(), nullptr, 0));
  if (!m_context)
  {
    CLog::Log(LOGERROR, "CPipewireContext: failed to create context: {}", strerror(errno));
    throw std::runtime_error("CPipewireContext: failed to create context");
  }
}

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
