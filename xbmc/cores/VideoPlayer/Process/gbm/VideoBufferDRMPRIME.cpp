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

IVideoBufferDRMPRIME::IVideoBufferDRMPRIME(int id)
  : CVideoBuffer(id)
{
}

CVideoBufferDRMPRIME::CVideoBufferDRMPRIME(IVideoBufferPool& pool, int id)
  : IVideoBufferDRMPRIME(id)
{
  m_pFrame = av_frame_alloc();
}

CVideoBufferDRMPRIME::~CVideoBufferDRMPRIME()
{
  Unref();
  av_frame_free(&m_pFrame);
}

void CVideoBufferDRMPRIME::SetRef(AVFrame* frame)
{
  av_frame_move_ref(m_pFrame, frame);
}

void CVideoBufferDRMPRIME::Unref()
{
  av_frame_unref(m_pFrame);
}

int CVideoBufferDRMPRIME::GetColorEncoding() const
{
  switch (m_pFrame->colorspace)
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
    if (m_pFrame->width > 1024 || m_pFrame->height >= 600)
      return DRM_COLOR_YCBCR_BT709;
    else
      return DRM_COLOR_YCBCR_BT601;
  }
}

int CVideoBufferDRMPRIME::GetColorRange() const
{
  switch (m_pFrame->color_range)
  {
  case AVCOL_RANGE_JPEG:
    return DRM_COLOR_YCBCR_FULL_RANGE;
  case AVCOL_RANGE_MPEG:
  default:
    return DRM_COLOR_YCBCR_LIMITED_RANGE;
  }
}

uint8_t CVideoBufferDRMPRIME::GetEOTF() const
{
  if (m_pFrame->color_trc == AVCOL_TRC_SMPTE2084)
    return HDMI_EOTF_SMPTE_ST2084;
  if (m_pFrame->color_trc == AVCOL_TRC_ARIB_STD_B67 ||
      m_pFrame->color_trc == AVCOL_TRC_BT2020_10)
    return HDMI_EOTF_BT_2100_HLG;

  return HDMI_EOTF_TRADITIONAL_GAMMA_SDR;
}

AVMasteringDisplayMetadata* CVideoBufferDRMPRIME::GetMasteringDisplayMetadata() const
{
  AVFrameSideData* sd = av_frame_get_side_data(m_pFrame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
  return (sd && sd->data) ? reinterpret_cast<AVMasteringDisplayMetadata*>(sd->data) : nullptr;
}

AVContentLightMetadata* CVideoBufferDRMPRIME::GetContentLightMetadata() const
{
  AVFrameSideData* sd = av_frame_get_side_data(m_pFrame, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
  return (sd && sd->data) ? reinterpret_cast<AVContentLightMetadata*>(sd->data) : nullptr;
}

bool CVideoBufferDRMPRIME::IsValid() const
{
  AVDRMFrameDescriptor* descriptor = GetDescriptor();
  return descriptor && descriptor->nb_layers;
}

CVideoBufferPoolDRMPRIME::~CVideoBufferPoolDRMPRIME()
{
  for (auto buf : m_all)
    delete buf;
}

CVideoBuffer* CVideoBufferPoolDRMPRIME::Get()
{
  CSingleLock lock(m_critSection);

  CVideoBufferDRMPRIME* buf = nullptr;
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
    buf = new CVideoBufferDRMPRIME(*this, id);
    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CVideoBufferPoolDRMPRIME::Return(int id)
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
