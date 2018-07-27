/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stddef.h> // for size_t

namespace XUTILS
{

  class auto_buffer
  {
  public:
    /**
     * Create buffer with zero size
     */
    auto_buffer(void) = default;
    /**
     * Create buffer with specified size
     * @param size of created buffer
     */
    explicit auto_buffer(size_t size);
    ~auto_buffer();

    /**
     * Allocate specified size for buffer, discarding current buffer content
     * @param size of buffer to allocate
     * @return reference to itself
     */
    auto_buffer& allocate(size_t size);
    /**
     * Resize current buffer to new size. Buffer will be extended or truncated at the end.
     * @param newSize of buffer
     * @return reference to itself
     */
    auto_buffer& resize(size_t newSize);
    /**
     * Reset buffer to zero size
     * @return reference to itself
     */
    auto_buffer& clear(void);

    /**
     * Get pointer to buffer content
     * @return pointer to buffer content or NULL if buffer is zero size
     */
    inline char* get(void) { return static_cast<char*>(p); }
    /**
     * Get constant pointer to buffer content
     * @return constant pointer to buffer content
     */
    inline const char* get(void) const { return static_cast<char*>(p); }
    /**
     * Get size of the buffer
     * @return size of the buffer
     */
    inline size_t size(void) const { return s; }
    /**
     * Get size of the buffer
     * @return size of the buffer
     */
    inline size_t length(void) const { return s; }

    /**
     * Attach malloc'ed pointer to the buffer, discarding current buffer content
     * Pointer must be acquired by malloc() or realloc().
     * Pointer will be automatically freed on destroy of the buffer.
     * @param pointer to attach
     * @param size of new memory region pointed by pointer
     * @return reference to itself
     */
    auto_buffer& attach(void* pointer, size_t size);
    /**
     * Detach current buffer content from the buffer, reset buffer to zero size
     * Caller is responsible to free memory by calling free() for returned pointer
     * when pointer in not needed anymore
     * @return detached from buffer pointer to content
     */
    void* detach(void);

  private:
    auto_buffer(const auto_buffer& other) = delete; // disallow copy constructor
    auto_buffer& operator=(const auto_buffer& other) = delete; // disallow assignment

    void* p = 0;
    size_t s = 0;
  };
}
