/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Process/VideoBuffer.h"

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/hwcontext_drm.h>
}

// Color enums is copied from linux include/drm/drm_color_mgmt.h (strangely not part of uapi)
enum drm_color_encoding
{
  DRM_COLOR_YCBCR_BT601,
  DRM_COLOR_YCBCR_BT709,
  DRM_COLOR_YCBCR_BT2020,
};
enum drm_color_range
{
  DRM_COLOR_YCBCR_LIMITED_RANGE,
  DRM_COLOR_YCBCR_FULL_RANGE,
};

class CVideoBufferDRMPRIME : public CVideoBuffer
{
public:
  CVideoBufferDRMPRIME() = delete;
  ~CVideoBufferDRMPRIME() override = default;

  virtual AVDRMFrameDescriptor* GetDescriptor() const = 0;
  virtual uint32_t GetWidth() const = 0;
  virtual uint32_t GetHeight() const = 0;
  virtual int GetColorEncoding() const { return DRM_COLOR_YCBCR_BT709; }
  virtual int GetColorRange() const { return DRM_COLOR_YCBCR_LIMITED_RANGE; }

  virtual bool IsValid() const { return true; }
  virtual bool Map() { return true; }
  virtual void Unmap() {}

  uint32_t m_fb_id = 0;
  uint32_t m_handles[AV_DRM_MAX_PLANES] = {};

protected:
  explicit CVideoBufferDRMPRIME(int id);
};

class CVideoBufferDRMPRIMEFFmpeg : public CVideoBufferDRMPRIME
{
public:
  CVideoBufferDRMPRIMEFFmpeg(IVideoBufferPool& pool, int id);
  ~CVideoBufferDRMPRIMEFFmpeg() override;
  void SetRef(AVFrame* frame);
  void Unref();

  AVDRMFrameDescriptor* GetDescriptor() const override
  {
    return reinterpret_cast<AVDRMFrameDescriptor*>(m_pFrame->data[0]);
  }
  uint32_t GetWidth() const override { return m_pFrame->width; }
  uint32_t GetHeight() const override { return m_pFrame->height; }
  int GetColorEncoding() const override;
  int GetColorRange() const override;

  bool IsValid() const override;

protected:
  AVFrame* m_pFrame = nullptr;
};

class CVideoBufferPoolDRMPRIMEFFmpeg : public IVideoBufferPool
{
public:
  ~CVideoBufferPoolDRMPRIMEFFmpeg() override;
  void Return(int id) override;
  CVideoBuffer* Get() override;

protected:
  CCriticalSection m_critSection;
  std::vector<CVideoBufferDRMPRIMEFFmpeg*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;
};
