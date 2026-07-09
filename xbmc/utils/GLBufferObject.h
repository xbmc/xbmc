/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>

#include "system_gl.h"

namespace KODI
{
namespace UTILS
{
namespace GL
{

// Owns one GL buffer object, bound to `target` for its whole life and created lazily on the first
// SetData()/SetDataOnce() -- construction performs no GL calls, so instances can be constructed
// before any GL context exists. All other methods require a current GL context; if this object may
// outlive the context, call Destroy() while the context is still valid instead of relying on the
// destructor. SetData()/SetDataOnce()/Bind() all leave the buffer bound to its target.
class CGLBufferObject
{
public:
  explicit CGLBufferObject(GLenum target) : m_target(target) {}
  ~CGLBufferObject();

  CGLBufferObject(const CGLBufferObject&) = delete;
  CGLBufferObject& operator=(const CGLBufferObject&) = delete;

  // (Re)uploads `data` on every call. For per-draw data (usage GL_STREAM_DRAW/GL_DYNAMIC_DRAW).
  template<typename T>
  void SetData(const T* data, std::size_t count, GLenum usage)
  {
    SetData(static_cast<const void*>(data), count * sizeof(T), usage);
  }
  template<typename T, std::size_t N>
  void SetData(const T (&data)[N], GLenum usage)
  {
    SetData(static_cast<const void*>(data), N * sizeof(T), usage);
  }

  // Uploads `data` (GL_STATIC_DRAW) on the first call only; later calls ignore `data` and just
  // bind. Pass identical data on every call.
  template<typename T>
  void SetDataOnce(const T* data, std::size_t count)
  {
    SetDataOnce(static_cast<const void*>(data), count * sizeof(T));
  }
  template<typename T, std::size_t N>
  void SetDataOnce(const T (&data)[N])
  {
    SetDataOnce(static_cast<const void*>(data), N * sizeof(T));
  }

  // Binds the buffer; it must already hold data from an earlier SetData()/SetDataOnce().
  void Bind() const;

  // Deletes the GL buffer, if any; a later SetData()/SetDataOnce() lazily recreates it.
  void Destroy();

  // True once the GL buffer has been created.
  explicit operator bool() const { return m_buffer != 0; }

private:
  void SetData(const void* data, std::size_t size, GLenum usage);
  void SetDataOnce(const void* data, std::size_t size);

  GLenum m_target;
  GLuint m_buffer = 0;
};

} // namespace GL
} // namespace UTILS
} // namespace KODI
