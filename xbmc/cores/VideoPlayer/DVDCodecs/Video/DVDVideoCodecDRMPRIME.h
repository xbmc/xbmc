/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavutil/hwcontext_drm.h"
}

// Color enums is copied from linux include/drm/drm_color_mgmt.h (strangely not part of uapi)
enum drm_color_encoding {
  DRM_COLOR_YCBCR_BT601,
  DRM_COLOR_YCBCR_BT709,
  DRM_COLOR_YCBCR_BT2020,
};
enum drm_color_range {
  DRM_COLOR_YCBCR_LIMITED_RANGE,
  DRM_COLOR_YCBCR_FULL_RANGE,
};

class CVideoBufferPoolDRMPRIME;

class CVideoBufferDRMPRIME
  : public CVideoBuffer
{
public:
  CVideoBufferDRMPRIME(IVideoBufferPool& pool, int id);
  ~CVideoBufferDRMPRIME();
  void SetRef(AVFrame* frame);
  void Unref();

  uint32_t m_fb_id = 0;
  uint32_t m_handles[AV_DRM_MAX_PLANES] = {0};

  AVDRMFrameDescriptor* GetDescriptor() const { return reinterpret_cast<AVDRMFrameDescriptor*>(m_pFrame->data[0]); }
  uint32_t GetWidth() const { return m_pFrame->width; }
  uint32_t GetHeight() const { return m_pFrame->height; }
  int GetColorEncoding() const;
  int GetColorRange() const;
protected:
  AVFrame* m_pFrame = nullptr;
};

class CDVDVideoCodecDRMPRIME
  : public CDVDVideoCodec
{
public:
  explicit CDVDVideoCodecDRMPRIME(CProcessInfo& processInfo);
  ~CDVDVideoCodecDRMPRIME();

  static CDVDVideoCodec* Create(CProcessInfo& processInfo);
  static void Register();

  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  bool AddData(const DemuxPacket& packet) override;
  void Reset() override;
  CDVDVideoCodec::VCReturn GetPicture(VideoPicture* pVideoPicture) override;
  const char* GetName() override { return m_name.c_str(); };
  unsigned GetAllowedReferences() override { return 5; };
  void SetCodecControl(int flags) override { m_codecControlFlags = flags; };

protected:
  void Drain();
  void SetPictureParams(VideoPicture* pVideoPicture);

  std::string m_name;
  int m_codecControlFlags = 0;
  AVCodecContext* m_pCodecContext = nullptr;
  AVFrame* m_pFrame = nullptr;
  std::shared_ptr<CVideoBufferPoolDRMPRIME> m_videoBufferPool;
};
