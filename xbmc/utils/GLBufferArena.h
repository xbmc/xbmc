/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <vector>

#include "system_gl.h"

namespace KODI
{
namespace UTILS
{
namespace GL
{

/*!
 * @brief Sub-allocates small vertex ranges from shared GL_ARRAY_BUFFER objects.
 *
 * Drivers back every buffer object with at least one page, so thousands of tiny per-object buffers
 * waste memory. Ranges come in a few fixed slot sizes, and each slot size has its own buffers of
 * bufferSize bytes. Construction performs no GL calls; everything else requires a current GL
 * context, so call Destroy() while the context is still valid.
 *
 * Uploads go to a CPU copy of the buffer, and Bind() respecifies the whole buffer when it changed.
 * Drivers give a respecified buffer new storage, whereas writing part of a buffer that pending
 * draws read makes tiled GPUs such as v3d flush the frame and wait for it.
 */
class CGLBufferArena
{
public:
  struct Range
  {
    std::size_t pool{0};
    std::size_t slot{0};
    unsigned int generation{0}; // 0 means not allocated
  };

  // slotSizes must be in ascending order.
  CGLBufferArena(std::vector<std::size_t> slotSizes, std::size_t bufferSize);
  ~CGLBufferArena();

  CGLBufferArena(const CGLBufferArena&) = delete;
  CGLBufferArena& operator=(const CGLBufferArena&) = delete;

  // Returns a range of at least size bytes, or an invalid range if size exceeds the largest slot.
  Range Allocate(std::size_t size);

  // Returns the range to the arena and resets it. Ranges from before Destroy() are only reset.
  void Free(Range& range);

  // False for ranges that were never allocated, were freed, or predate Destroy().
  bool IsValid(const Range& range) const;

  // Writes size bytes at the start of the range; the next Bind() of its buffer uploads them.
  void Upload(const Range& range, const void* data, std::size_t size);

  // Binds the range's buffer to GL_ARRAY_BUFFER and returns the range's byte offset in it.
  GLintptr Bind(const Range& range);

  // Deletes all buffers; every range allocated so far becomes invalid.
  void Destroy();

private:
  struct Buffer
  {
    GLuint name{0};
    std::vector<std::byte> data;
    bool dirty{true};
  };

  struct Pool
  {
    std::size_t slotSize;
    std::size_t slotsPerBuffer;
    std::vector<Buffer> buffers;
    std::vector<std::size_t> freeSlots;
    std::size_t nextSlot{0};
  };

  std::vector<Pool> m_pools;
  unsigned int m_generation{1};
};

} // namespace GL
} // namespace UTILS
} // namespace KODI
