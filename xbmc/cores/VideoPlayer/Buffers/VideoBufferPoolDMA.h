/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"

#include <memory>

class CVideoBufferDMA;

class CVideoBufferPoolDMA : public IVideoBufferPool
{
public:
  CVideoBufferPoolDMA() = default;
  ~CVideoBufferPoolDMA() override;

  // implementation of IVideoBufferPool
  CVideoBuffer* Get() override;
  void Return(int id) override;
  void Configure(AVPixelFormat format, int size) override;
  bool IsConfigured() override;
  bool IsCompatible(AVPixelFormat format, int size) override;
  void Released(CVideoBufferManager& videoBufferManager) override;

  static std::shared_ptr<IVideoBufferPool> CreatePool();

protected:
  CCriticalSection m_critSection;
  std::vector<CVideoBufferDMA*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;

private:
  uint32_t TranslateFormat(AVPixelFormat format);

  uint32_t m_fourcc = 0;
  uint64_t m_size = 0;
};
