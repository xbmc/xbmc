/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoBufferDRMPRIME.h"

#include "threads/SingleLock.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

namespace DRMPRIME
{

int GetColorEncoding(const VideoPicture& picture)
{
  switch (picture.color_space)
  {
    case AVCOL_SPC_BT2020_CL:
    case AVCOL_SPC_BT2020_NCL:
      return DRM_COLOR_YCBCR_BT2020;
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_FCC:
      return DRM_COLOR_YCBCR_BT601;
    case AVCOL_SPC_BT709:
      return DRM_COLOR_YCBCR_BT709;
    case AVCOL_SPC_RESERVED:
    case AVCOL_SPC_UNSPECIFIED:
    default:
      if (picture.iWidth > 1024 || picture.iHeight >= 600)
        return DRM_COLOR_YCBCR_BT709;
      else
        return DRM_COLOR_YCBCR_BT601;
  }
}

int GetColorRange(const VideoPicture& picture)
{
  if (picture.color_range)
    return DRM_COLOR_YCBCR_FULL_RANGE;
  return DRM_COLOR_YCBCR_LIMITED_RANGE;
}

uint8_t GetEOTF(const VideoPicture& picture)
{
  switch (picture.color_transfer)
  {
    case AVCOL_TRC_SMPTE2084:
      return HDMI_EOTF_SMPTE_ST2084;
    case AVCOL_TRC_ARIB_STD_B67:
    case AVCOL_TRC_BT2020_10:
      return HDMI_EOTF_BT_2100_HLG;
    default:
      return HDMI_EOTF_TRADITIONAL_GAMMA_SDR;
  }
}

const AVMasteringDisplayMetadata* GetMasteringDisplayMetadata(const VideoPicture& picture)
{
  return picture.hasDisplayMetadata ? &picture.displayMetadata : nullptr;
}

const AVContentLightMetadata* GetContentLightMetadata(const VideoPicture& picture)
{
  return picture.hasLightMetadata ? &picture.lightMetadata : nullptr;
}

} // namespace DRMPRIME

CVideoBufferDRMPRIME::CVideoBufferDRMPRIME(int id) : CVideoBuffer(id)
{
  m_pixFormat = AV_PIX_FMT_DRM_PRIME;
}

CVideoBufferDRMPRIMEFFmpeg::CVideoBufferDRMPRIMEFFmpeg(IVideoBufferPool& pool, int id)
  : CVideoBufferDRMPRIME(id)
{
  m_pFrame = av_frame_alloc();
}

CVideoBufferDRMPRIMEFFmpeg::~CVideoBufferDRMPRIMEFFmpeg()
{
  Unref();
  av_frame_free(&m_pFrame);
}

void CVideoBufferDRMPRIMEFFmpeg::SetRef(AVFrame* frame)
{
  av_frame_move_ref(m_pFrame, frame);
}

void CVideoBufferDRMPRIMEFFmpeg::Unref()
{
  av_frame_unref(m_pFrame);
}

bool CVideoBufferDRMPRIMEFFmpeg::IsValid() const
{
  AVDRMFrameDescriptor* descriptor = GetDescriptor();
  return descriptor && descriptor->nb_layers;
}

CVideoBufferPoolDRMPRIMEFFmpeg::~CVideoBufferPoolDRMPRIMEFFmpeg()
{
  for (auto buf : m_all)
    delete buf;
}

CVideoBuffer* CVideoBufferPoolDRMPRIMEFFmpeg::Get()
{
  CSingleLock lock(m_critSection);

  CVideoBufferDRMPRIMEFFmpeg* buf = nullptr;
  if (!m_free.empty())
  {
    int idx = m_free.front();
    m_free.pop_front();
    m_used.push_back(idx);
    buf = m_all[idx];
  }
  else
  {
    int id = m_all.size();
    buf = new CVideoBufferDRMPRIMEFFmpeg(*this, id);
    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CVideoBufferPoolDRMPRIMEFFmpeg::Return(int id)
{
  CSingleLock lock(m_critSection);

  m_all[id]->Unref();
  auto it = m_used.begin();
  while (it != m_used.end())
  {
    if (*it == id)
    {
      m_used.erase(it);
      break;
    }
    else
      ++it;
  }
  m_free.push_back(id);
}
