/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C"
{
#include <libavutil/pixfmt.h>
}

class CWinSystemBase;

//! \brief Subsystem dependencies a capture readback needs, injected so the
//! surface does not reach for global state.
struct ScreenshotContext
{
  CWinSystemBase& winSystem;
};

class IScreenshotSurface
{
public:
  //! frees the buffer only when the caller did not take ownership via TakeBuffer
  virtual ~IScreenshotSurface() { delete[] m_buffer; }

  //! \brief Read back the current framebuffer only; the caller guarantees a
  //! fully rendered frame and render-thread context.
  virtual bool Read(const ScreenshotContext& ctx) { return false; }

  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }
  //! signed linesize; negative when the delivered rows are bottom-up
  int GetStride() const { return m_stride; }
  //! source coding of the delivered bytes, handed to the consumer's swscale
  AVPixelFormat GetFormat() const { return m_format; }
  unsigned char* GetBuffer() const { return m_buffer; }

  //! \brief Transfer buffer ownership to the caller.
  unsigned char* TakeBuffer()
  {
    unsigned char* buffer = m_buffer;
    m_buffer = nullptr;
    return buffer;
  }

protected:
  int m_width{0};
  int m_height{0};
  int m_stride{0};
  AVPixelFormat m_format{AV_PIX_FMT_NONE};
  unsigned char* m_buffer{nullptr};
};
