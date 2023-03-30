/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/RetroPlayerTypes.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <memory>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
class IRenderBufferPool;

class IRenderBuffer
{
public:
  virtual ~IRenderBuffer() = default;

  // Pool functions
  virtual void Acquire() = 0;
  virtual void Acquire(std::shared_ptr<IRenderBufferPool> pool) = 0;
  virtual void Release() = 0;
  virtual IRenderBufferPool* GetPool() = 0;
  virtual DataAccess GetMemoryAccess() const = 0;
  virtual DataAlignment GetMemoryAlignment() const = 0;

  // Buffer functions
  virtual bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) = 0;
  virtual void Update() {} //! @todo Remove me
  virtual size_t GetFrameSize() const = 0;
  virtual uint8_t* GetMemory() = 0;
  virtual void ReleaseMemory() {}
  virtual bool UploadTexture() = 0;
  virtual void BindToUnit(unsigned int unit) {}
  virtual void SetHeader(void* header) {}

  // Buffer properties
  AVPixelFormat GetFormat() const { return m_format; }
  unsigned int GetWidth() const { return m_width; }
  unsigned int GetHeight() const { return m_height; }
  bool IsLoaded() const { return m_bLoaded; }
  void SetLoaded(bool bLoaded) { m_bLoaded = bLoaded; }
  bool IsRendered() const { return m_bRendered; }
  void SetRendered(bool bRendered) { m_bRendered = bRendered; }
  unsigned int GetRotation() const { return m_rotationDegCCW; }
  void SetRotation(unsigned int rotationDegCCW) { m_rotationDegCCW = rotationDegCCW; }

protected:
  AVPixelFormat m_format = AV_PIX_FMT_NONE;
  unsigned int m_width = 0;
  unsigned int m_height = 0;
  bool m_bLoaded = false;
  bool m_bRendered = false;
  unsigned int m_rotationDegCCW = 0;
};
} // namespace RETRO
} // namespace KODI
