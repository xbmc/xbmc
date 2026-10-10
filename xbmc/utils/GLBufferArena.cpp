/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GLBufferArena.h"

#include <cassert>
#include <cstring>

using namespace KODI::UTILS::GL;

CGLBufferArena::CGLBufferArena(std::vector<std::size_t> slotSizes, std::size_t bufferSize)
{
  for (std::size_t slotSize : slotSizes)
  {
    assert(slotSize > 0 && slotSize <= bufferSize);
    assert(m_pools.empty() || m_pools.back().slotSize < slotSize);
    m_pools.push_back({slotSize, bufferSize / slotSize, {}, {}});
  }
}

CGLBufferArena::~CGLBufferArena()
{
  Destroy();
}

CGLBufferArena::Range CGLBufferArena::Allocate(std::size_t size)
{
  for (std::size_t pool = 0; pool < m_pools.size(); ++pool)
  {
    Pool& p = m_pools[pool];
    if (p.slotSize < size)
      continue;

    std::size_t slot;
    if (!p.freeSlots.empty())
    {
      slot = p.freeSlots.back();
      p.freeSlots.pop_back();
    }
    else
    {
      slot = p.nextSlot++;
      if (slot / p.slotsPerBuffer >= p.buffers.size())
      {
        Buffer& buffer = p.buffers.emplace_back();
        glGenBuffers(1, &buffer.name);
        buffer.data.resize(p.slotsPerBuffer * p.slotSize);
      }
    }
    return {pool, slot, m_generation};
  }

  return {};
}

void CGLBufferArena::Free(Range& range)
{
  if (IsValid(range))
    m_pools[range.pool].freeSlots.push_back(range.slot);
  range = {};
}

bool CGLBufferArena::IsValid(const Range& range) const
{
  return range.generation == m_generation;
}

void CGLBufferArena::Upload(const Range& range, const void* data, std::size_t size)
{
  assert(IsValid(range) && size <= m_pools[range.pool].slotSize);
  Pool& p = m_pools[range.pool];
  Buffer& buffer = p.buffers[range.slot / p.slotsPerBuffer];
  std::memcpy(buffer.data.data() + (range.slot % p.slotsPerBuffer) * p.slotSize, data, size);
  buffer.dirty = true;
}

GLintptr CGLBufferArena::Bind(const Range& range)
{
  assert(IsValid(range));
  Pool& p = m_pools[range.pool];
  Buffer& buffer = p.buffers[range.slot / p.slotsPerBuffer];
  glBindBuffer(GL_ARRAY_BUFFER, buffer.name);
  if (buffer.dirty)
  {
    glBufferData(GL_ARRAY_BUFFER, buffer.data.size(), buffer.data.data(), GL_DYNAMIC_DRAW);
    buffer.dirty = false;
  }
  return static_cast<GLintptr>((range.slot % p.slotsPerBuffer) * p.slotSize);
}

void CGLBufferArena::Destroy()
{
  for (Pool& p : m_pools)
  {
    for (const Buffer& buffer : p.buffers)
      glDeleteBuffers(1, &buffer.name);
    p.buffers.clear();
    p.freeSlots.clear();
    p.nextSlot = 0;
  }

  if (++m_generation == 0)
    m_generation = 1;
}
