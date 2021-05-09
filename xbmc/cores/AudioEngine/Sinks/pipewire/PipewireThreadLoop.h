/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include <pipewire/thread-loop.h>

namespace AE
{
namespace SINK
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

  void Lock();
  void Unlock();

  void Wait();
  void Signal(bool accept);

private:
  struct PipewireThreadLoopDeleter
  {
    void operator()(pw_thread_loop* p) { pw_thread_loop_destroy(p); }
  };

  std::unique_ptr<pw_thread_loop, PipewireThreadLoopDeleter> m_mainloop;
};

} // namespace PIPEWIRE
} // namespace SINK
} // namespace AE
