/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  virtual ~IScreenshotSurface() = default;

  //! \brief Read back the current framebuffer only; the caller guarantees a
  //! fully rendered frame and render-thread context.
  virtual bool Read(const ScreenshotContext& ctx) { return false; }

  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }
  int GetStride() const { return m_stride; }
  int GetBitDepth() const { return m_bitDepth; }
  unsigned char* GetBuffer() const { return m_buffer; }

  //! \brief Transfer buffer ownership to the caller.
  unsigned char* TakeBuffer()
  {
    unsigned char* buffer = m_buffer;
    m_buffer = nullptr;
    return buffer;
  }
  void ReleaseBuffer()
  {
    if (m_buffer)
    {
      delete m_buffer;
      m_buffer = nullptr;
    }
  };

protected:
  int m_width{0};
  int m_height{0};
  int m_stride{0};
  int m_bitDepth{8};
  unsigned char* m_buffer{nullptr};
};
