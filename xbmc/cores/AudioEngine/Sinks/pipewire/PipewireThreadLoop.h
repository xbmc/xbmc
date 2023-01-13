/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <memory>

#include <pipewire/thread-loop.h>

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireThreadLoop
{
public:
  CPipewireThreadLoop();
  ~CPipewireThreadLoop() = default;

  pw_loop* Get() const { return pw_thread_loop_get_loop(m_mainloop.get()); }

  bool Start();
  void Stop();

  void Lock() const;
  void Unlock() const;

  int Wait(std::chrono::nanoseconds timeout);
  void Signal(bool accept);

private:
  struct PipewireThreadLoopDeleter
  {
    void operator()(pw_thread_loop* p) { pw_thread_loop_destroy(p); }
  };

  std::unique_ptr<pw_thread_loop, PipewireThreadLoopDeleter> m_mainloop;
};

class CLoopLockGuard
{
public:
  explicit CLoopLockGuard(const CPipewireThreadLoop& loop) : m_loop(loop) { m_loop.Lock(); }

  ~CLoopLockGuard() { m_loop.Unlock(); }

  CLoopLockGuard() = delete;
  CLoopLockGuard(const CLoopLockGuard&) = delete;
  CLoopLockGuard& operator=(const CLoopLockGuard&) = delete;

private:
  const CPipewireThreadLoop& m_loop;
};

} // namespace PIPEWIRE
} // namespace KODI
