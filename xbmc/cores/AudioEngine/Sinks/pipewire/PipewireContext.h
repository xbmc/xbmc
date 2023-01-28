/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include <pipewire/context.h>

namespace KODI
{
namespace PIPEWIRE
{

class CPipewireThreadLoop;

class CPipewireContext
{
public:
  explicit CPipewireContext(CPipewireThreadLoop& loop);
  CPipewireContext() = delete;
  ~CPipewireContext() = default;

  pw_context* Get() const { return m_context.get(); }

  CPipewireThreadLoop& GetThreadLoop() const { return m_threadLoop; }

private:
  CPipewireThreadLoop& m_threadLoop;

  struct PipewireContextDeleter
  {
    void operator()(pw_context* p) { pw_context_destroy(p); }
  };

  std::unique_ptr<pw_context, PipewireContextDeleter> m_context;
};

} // namespace PIPEWIRE
} // namespace KODI
