/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

//! \brief Consumer-thread access to a capture's pixels regardless of producer:
//! a GL readback backs it with a heap buffer, a direct-to-plane capture with a
//! mapped dma-buf. Lock/Unlock bound the read so producer resources stay alive.
class ICapturePixels
{
public:
  virtual ~ICapturePixels() = default;
  //! consumer thread: first row in memory; valid until the matching Unlock
  virtual const uint8_t* Lock() = 0;
  virtual void Unlock() {}
};

//! RAII Lock/Unlock around ICapturePixels for one consumer read.
class CScopedCapturePixels
{
public:
  explicit CScopedCapturePixels(ICapturePixels& pixels) : m_pixels(pixels), m_data(pixels.Lock()) {}
  ~CScopedCapturePixels() { m_pixels.Unlock(); }
  CScopedCapturePixels(const CScopedCapturePixels&) = delete;
  CScopedCapturePixels& operator=(const CScopedCapturePixels&) = delete;

  const uint8_t* data() const { return m_data; }

private:
  ICapturePixels& m_pixels;
  const uint8_t* m_data;
};

//! Pixels held in a plain heap buffer, the GL readback and Direct3D staging path.
class CHeapCapturePixels : public ICapturePixels
{
public:
  explicit CHeapCapturePixels(std::unique_ptr<uint8_t[]> buffer) : m_buffer(std::move(buffer)) {}
  const uint8_t* Lock() override { return m_buffer.get(); }

private:
  std::unique_ptr<uint8_t[]> m_buffer;
};

//! swscale source pointer for a capture buffer whose stride may be negative
//! (bottom-up): point at the last row so swscale walks backward from it.
inline const uint8_t* CaptureSrcRow0(const uint8_t* base, int stride, unsigned int height)
{
  if (stride >= 0)
    return base;
  return base + static_cast<std::size_t>(height - 1) * static_cast<std::size_t>(-stride);
}

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
