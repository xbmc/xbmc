/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

class CVideoBufferPoolDRMPRIME;

class CVideoBufferDRMPRIME
  : public CVideoBuffer
{
public:
  CVideoBufferDRMPRIME(IVideoBufferPool& pool, int id);
  ~CVideoBufferDRMPRIME();
  void SetRef(AVFrame* frame);
  void Unref();

  uint32_t m_drm_fd = -1;
  uint32_t m_fb_id = 0;
  uint32_t m_handles[AV_DRM_MAX_PLANES] = {0};

  AVDRMFrameDescriptor* GetDescriptor() const { return reinterpret_cast<AVDRMFrameDescriptor*>(m_pFrame->data[0]); }
  uint32_t GetWidth() const { return m_pFrame->width; }
  uint32_t GetHeight() const { return m_pFrame->height; }
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
  unsigned GetAllowedReferences() override { return 4; };
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
