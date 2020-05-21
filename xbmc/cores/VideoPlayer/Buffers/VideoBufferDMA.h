/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"

#include <memory>

class IBufferObject;

class CVideoBufferDMA : public CVideoBufferDRMPRIMEFFmpeg
{
public:
  CVideoBufferDMA(IVideoBufferPool& pool, int id, uint32_t fourcc, uint64_t size);
  ~CVideoBufferDMA() override;

  // implementation of CVideoBufferDRMPRIME via CVideoBufferDRMPRIMEFFmpeg
  uint32_t GetWidth() const override { return m_width; }
  uint32_t GetHeight() const override { return m_height; }
  AVDRMFrameDescriptor* GetDescriptor() const override;

  // implementation of CVideoBuffer via CVideoBufferDRMPRIMEFFmpeg
  void GetPlanes(uint8_t* (&planes)[YuvImage::MAX_PLANES]) override;
  void GetStrides(int (&strides)[YuvImage::MAX_PLANES]) override;
  uint8_t* GetMemPtr() override;
  void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES]) override;
  void SetDimensions(int width,
                     int height,
                     const int (&strides)[YuvImage::MAX_PLANES],
                     const int (&planeOffsets)[YuvImage::MAX_PLANES]) override;

  void SetDimensions(int width, int height);
  bool Alloc();
  void Export(AVFrame* frame, uint32_t width, uint32_t height);

  void SyncStart();
  void SyncEnd();

private:
  void Destroy();

  std::unique_ptr<IBufferObject> m_bo;

  int m_offsets[YuvImage::MAX_PLANES]{0};
  int m_strides[YuvImage::MAX_PLANES]{0};

  AVDRMFrameDescriptor m_descriptor{};
  uint32_t m_planes{0};
  uint32_t m_width{0};
  uint32_t m_height{0};
  uint32_t m_fourcc{0};
  uint64_t m_size{0};
  uint8_t* m_addr{nullptr};
  int m_fd{-1};
};
