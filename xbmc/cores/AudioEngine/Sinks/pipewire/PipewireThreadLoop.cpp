/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipewireThreadLoop.h"

#include "utils/log.h"

#include <stdexcept>

using namespace KODI;
using namespace PIPEWIRE;

CPipewireThreadLoop::CPipewireThreadLoop()
{
  m_mainloop.reset(pw_thread_loop_new("pipewire", nullptr));
  if (!m_mainloop)
  {
    CLog::Log(LOGERROR, "CPipewireThreadLoop: failed to create main loop: {}", strerror(errno));
    throw std::runtime_error("CPipewireThreadLoop: failed to create main loop");
  }
}

bool CPipewireThreadLoop::Start()
{
  return pw_thread_loop_start(m_mainloop.get()) == 0;
}

void CPipewireThreadLoop::Stop()
{
  pw_thread_loop_stop(m_mainloop.get());
}

void CPipewireThreadLoop::Lock() const
{
  pw_thread_loop_lock(m_mainloop.get());
}

void CPipewireThreadLoop::Unlock() const
{
  pw_thread_loop_unlock(m_mainloop.get());
}

int CPipewireThreadLoop::Wait(std::chrono::nanoseconds timeout)
{
  timespec abstime;
  pw_thread_loop_get_time(m_mainloop.get(), &abstime, timeout.count());

  return pw_thread_loop_timed_wait_full(m_mainloop.get(), &abstime);
}

void CPipewireThreadLoop::Signal(bool accept)
{
  pw_thread_loop_signal(m_mainloop.get(), accept);
}
