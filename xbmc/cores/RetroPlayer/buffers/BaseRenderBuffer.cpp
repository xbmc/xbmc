/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BaseRenderBuffer.h"

#include "IRenderBufferPool.h"

#include <cassert>

using namespace KODI;
using namespace RETRO;

CBaseRenderBuffer::CBaseRenderBuffer() : m_refCount(0)
{
}

void CBaseRenderBuffer::Acquire()
{
  m_refCount++;
}

void CBaseRenderBuffer::Acquire(std::shared_ptr<IRenderBufferPool> pool)
{
  m_refCount++;
  m_pool = pool;
}

void CBaseRenderBuffer::Release()
{
  if (--m_refCount <= 0 && m_pool)
  {
    std::shared_ptr<IRenderBufferPool> pool = m_pool->GetPtr();
    m_pool.reset();
    pool->Return(this);
  }
}

DataAccess CBaseRenderBuffer::GetMemoryAccess() const
{
  assert(m_pool.get() != nullptr);
  return m_pool->GetMemoryAccess();
}

DataAlignment CBaseRenderBuffer::GetMemoryAlignment() const
{
  assert(m_pool.get() != nullptr);
  return m_pool->GetMemoryAlignment();
}
