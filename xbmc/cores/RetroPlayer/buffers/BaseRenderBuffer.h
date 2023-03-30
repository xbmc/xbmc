/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRenderBuffer.h"

#include <atomic>
#include <memory>

namespace KODI
{
namespace RETRO
{
class CBaseRenderBuffer : public IRenderBuffer
{
public:
  CBaseRenderBuffer();
  ~CBaseRenderBuffer() override = default;

  // Partial implementation of IRenderBuffer
  void Acquire() override;
  void Acquire(std::shared_ptr<IRenderBufferPool> pool) override;
  void Release() override;
  IRenderBufferPool* GetPool() override { return m_pool.get(); }
  DataAccess GetMemoryAccess() const override;
  DataAlignment GetMemoryAlignment() const override;

protected:
  // Reference counting
  std::atomic_int m_refCount;

  // Pool callback
  std::shared_ptr<IRenderBufferPool> m_pool;
};
} // namespace RETRO
} // namespace KODI
