/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRenderBufferPool.h"
#include "threads/CriticalSection.h"

#include <deque>
#include <memory>
#include <vector>

namespace KODI
{
namespace RETRO
{
class CBaseRenderBufferPool : public IRenderBufferPool
{
public:
  CBaseRenderBufferPool() = default;
  ~CBaseRenderBufferPool() override;

  // Partial implementation of IRenderBufferPool
  void RegisterRenderer(CRPBaseRenderer* renderer) override;
  void UnregisterRenderer(CRPBaseRenderer* renderer) override;
  bool HasVisibleRenderer() const override;
  bool Configure(AVPixelFormat format) override;
  bool IsConfigured() const override { return m_bConfigured; }
  IRenderBuffer* GetBuffer(unsigned int width, unsigned int height) override;
  void Return(IRenderBuffer* buffer) override;
  void Prime(unsigned int width, unsigned int height) override;
  void Flush() override;

  // Buffer properties
  AVPixelFormat Format() const { return m_format; }

protected:
  virtual IRenderBuffer* CreateRenderBuffer(void* header = nullptr) = 0;
  virtual bool ConfigureInternal() { return true; }
  virtual void* GetHeader(unsigned int timeoutMs = 0) { return nullptr; }
  virtual bool GetHeaderWithTimeout(void*& header)
  {
    header = nullptr;
    return true;
  }
  virtual bool SendBuffer(IRenderBuffer* buffer) { return false; }

  // Configuration parameters
  bool m_bConfigured = false;
  AVPixelFormat m_format = AV_PIX_FMT_NONE;

private:
  // Buffer properties
  std::deque<std::unique_ptr<IRenderBuffer>> m_free;

  std::vector<CRPBaseRenderer*> m_renderers;
  mutable CCriticalSection m_rendererMutex;
  CCriticalSection m_bufferMutex;
};
} // namespace RETRO
} // namespace KODI
