/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CGUIWindowManager;
class CWinSystemBase;

//! \brief Subsystem dependencies a screenshot surface needs at capture time,
//! injected by CScreenShot so the surface does not reach for global state.
struct ScreenshotContext
{
  CWinSystemBase& winSystem;
  CGUIWindowManager& windowManager;
};

class IScreenshotSurface
{
public:
  virtual ~IScreenshotSurface() = default;
  virtual bool Capture(const ScreenshotContext& ctx) { return false; }

  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }
  int GetStride() const { return m_stride; }
  int GetBitDepth() const { return m_bitDepth; }
  unsigned char* GetBuffer() const { return m_buffer; }
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
