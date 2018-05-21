/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "BaseRenderBuffer.h"
#include "IRenderBufferPool.h"

using namespace KODI;
using namespace RETRO;

CBaseRenderBuffer::CBaseRenderBuffer() :
  m_refCount(0)
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
