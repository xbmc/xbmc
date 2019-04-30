/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLayerBridgeDRMPRIME.h"

#include "cores/VideoPlayer/Process/gbm/VideoBufferDRMPRIME.h"
#include "utils/log.h"
#include "windowing/gbm/DRMUtils.h"

using namespace KODI::WINDOWING::GBM;

CVideoLayerBridgeDRMPRIME::CVideoLayerBridgeDRMPRIME(std::shared_ptr<CDRMUtils> drm)
  : m_DRM(drm)
{
}

CVideoLayerBridgeDRMPRIME::~CVideoLayerBridgeDRMPRIME()
{
  Release(m_prev_buffer);
  Release(m_buffer);
}

void CVideoLayerBridgeDRMPRIME::Disable()
{
  // disable video plane
  struct plane* plane = m_DRM->GetVideoPlane();
  m_DRM->AddProperty(plane, "FB_ID", 0);
  m_DRM->AddProperty(plane, "CRTC_ID", 0);

  // disable HDR metadata
  struct connector* connector = m_DRM->GetConnector();
  if (m_DRM->SupportsProperty(connector, "HDR_OUTPUT_METADATA"))
  {
    m_DRM->AddProperty(connector, "HDR_OUTPUT_METADATA", 0);
    m_DRM->SetActive(true);

    if (m_hdr_blob_id)
      drmModeDestroyPropertyBlob(m_DRM->GetFileDescriptor(), m_hdr_blob_id);
    m_hdr_blob_id = 0;
  }
}

void CVideoLayerBridgeDRMPRIME::Acquire(IVideoBufferDRMPRIME* buffer)
{
  // release the buffer that is no longer presented on screen
  Release(m_prev_buffer);

  // release the buffer currently being presented next call
  m_prev_buffer = m_buffer;

  // reference count the buffer that is going to be presented on screen
  m_buffer = buffer;
  m_buffer->Acquire();
}

void CVideoLayerBridgeDRMPRIME::Release(IVideoBufferDRMPRIME* buffer)
{
  if (!buffer)
    return;

  Unmap(buffer);
  buffer->Release();
}

bool CVideoLayerBridgeDRMPRIME::Map(IVideoBufferDRMPRIME* buffer)
{
  if (buffer->m_fb_id)
    return true;

  if (!buffer->Map())
    return false;

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0}, flags = 0;
  uint64_t modifier[4] = {0};
  int ret;

  // convert Prime FD to GEM handle
  for (int object = 0; object < descriptor->nb_objects; object++)
  {
    ret = drmPrimeFDToHandle(m_DRM->GetFileDescriptor(), descriptor->objects[object].fd, &buffer->m_handles[object]);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "CVideoLayerBridgeDRMPRIME::%s - failed to convert prime fd %d to gem handle %u, ret = %d",
                __FUNCTION__, descriptor->objects[object].fd, buffer->m_handles[object], ret);
      return false;
    }
  }

  AVDRMLayerDescriptor* layer = &descriptor->layers[0];

  for (int plane = 0; plane < layer->nb_planes; plane++)
  {
    int object = layer->planes[plane].object_index;
    uint32_t handle = buffer->m_handles[object];
    if (handle && layer->planes[plane].pitch)
    {
      handles[plane] = handle;
      pitches[plane] = layer->planes[plane].pitch;
      offsets[plane] = layer->planes[plane].offset;
      modifier[plane] = descriptor->objects[object].format_modifier;
    }
  }

  if (modifier[0] && modifier[0] != DRM_FORMAT_MOD_INVALID)
    flags = DRM_MODE_FB_MODIFIERS;

  // add the video frame FB
  ret = drmModeAddFB2WithModifiers(m_DRM->GetFileDescriptor(), buffer->GetWidth(), buffer->GetHeight(), layer->format,
                                   handles, pitches, offsets, modifier, &buffer->m_fb_id, flags);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CVideoLayerBridgeDRMPRIME::%s - failed to add fb %d, ret = %d", __FUNCTION__, buffer->m_fb_id, ret);
    return false;
  }

  Acquire(buffer);
  return true;
}

void CVideoLayerBridgeDRMPRIME::Unmap(IVideoBufferDRMPRIME* buffer)
{
  if (buffer->m_fb_id)
  {
    drmModeRmFB(m_DRM->GetFileDescriptor(), buffer->m_fb_id);
    buffer->m_fb_id = 0;
  }

  for (int i = 0; i < AV_DRM_MAX_PLANES; i++)
  {
    if (buffer->m_handles[i])
    {
      struct drm_gem_close gem_close = { .handle = buffer->m_handles[i] };
      drmIoctl(m_DRM->GetFileDescriptor(), DRM_IOCTL_GEM_CLOSE, &gem_close);
      buffer->m_handles[i] = 0;
    }
  }

  buffer->Unmap();
}

void CVideoLayerBridgeDRMPRIME::Configure(IVideoBufferDRMPRIME* buffer)
{
  struct plane* plane = m_DRM->GetVideoPlane();
  if (m_DRM->SupportsProperty(plane, "COLOR_ENCODING") &&
      m_DRM->SupportsProperty(plane, "COLOR_RANGE"))
  {
    m_DRM->AddProperty(plane, "COLOR_ENCODING", buffer->GetColorEncoding());
    m_DRM->AddProperty(plane, "COLOR_RANGE", buffer->GetColorRange());
  }

  struct connector* connector = m_DRM->GetConnector();
  if (m_DRM->SupportsProperty(connector, "HDR_OUTPUT_METADATA"))
  {
    CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - supports HDR_OUTPUT_METADATA", __FUNCTION__);

    m_hdr_metadata.metadata_type = HDMI_STATIC_METADATA_TYPE1;
    m_hdr_metadata.hdmi_metadata_type1 = {
      .eotf = buffer->GetEOTF(),
      .metadata_type = HDMI_STATIC_METADATA_TYPE1,
    };

    CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - EOTF={}", __FUNCTION__, m_hdr_metadata.hdmi_metadata_type1.eotf);

    if (m_hdr_blob_id)
      drmModeDestroyPropertyBlob(m_DRM->GetFileDescriptor(), m_hdr_blob_id);
    m_hdr_blob_id = 0;

    if (m_hdr_metadata.hdmi_metadata_type1.eotf)
    {
      AVMasteringDisplayMetadata* mdmd = buffer->GetMasteringDisplayMetadata();
      if (mdmd && mdmd->has_primaries)
      {
        for (int i = 0; i < 3; i++)
        {
          m_hdr_metadata.hdmi_metadata_type1.display_primaries[i].x = std::round(av_q2d(mdmd->display_primaries[i][0]) * 50000.0);
          m_hdr_metadata.hdmi_metadata_type1.display_primaries[i].y = std::round(av_q2d(mdmd->display_primaries[i][1]) * 50000.0);

          CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - display_primaries[{}].x={}", __FUNCTION__, i, m_hdr_metadata.hdmi_metadata_type1.display_primaries[i].x);
          CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - display_primaries[{}].y={}", __FUNCTION__, i, m_hdr_metadata.hdmi_metadata_type1.display_primaries[i].y);
        }
        m_hdr_metadata.hdmi_metadata_type1.white_point.x = std::round(av_q2d(mdmd->white_point[0]) * 50000.0);
        m_hdr_metadata.hdmi_metadata_type1.white_point.y = std::round(av_q2d(mdmd->white_point[1]) * 50000.0);

        CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - white_point.x={}", __FUNCTION__, m_hdr_metadata.hdmi_metadata_type1.white_point.x);
        CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - white_point.y={}", __FUNCTION__, m_hdr_metadata.hdmi_metadata_type1.white_point.y);
      }
      if (mdmd && mdmd->has_luminance)
      {
        m_hdr_metadata.hdmi_metadata_type1.max_display_mastering_luminance = std::round(av_q2d(mdmd->max_luminance));
        m_hdr_metadata.hdmi_metadata_type1.min_display_mastering_luminance = std::round(av_q2d(mdmd->min_luminance) * 10000.0);

        CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - max_display_mastering_luminance={}", __FUNCTION__, m_hdr_metadata.hdmi_metadata_type1.max_display_mastering_luminance);
        CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - min_display_mastering_luminance={}", __FUNCTION__, m_hdr_metadata.hdmi_metadata_type1.min_display_mastering_luminance);
      }

      AVContentLightMetadata* clmd = buffer->GetContentLightMetadata();
      if (clmd)
      {
        m_hdr_metadata.hdmi_metadata_type1.max_cll = clmd->MaxCLL;
        m_hdr_metadata.hdmi_metadata_type1.max_fall = clmd->MaxFALL;
        CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - max_cll={}", __FUNCTION__, clmd->MaxCLL);
        CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - max_fall={}", __FUNCTION__, clmd->MaxFALL);
      }

      drmModeCreatePropertyBlob(m_DRM->GetFileDescriptor(), &m_hdr_metadata, sizeof(m_hdr_metadata), &m_hdr_blob_id);
    }

    if (m_DRM->SupportsProperty(connector, "Colorspace"))
    {
      CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - Colorspace={}", __FUNCTION__, 9);
      m_DRM->AddProperty(connector, "Colorspace", 9);
    }

    /*
  132 Colorspace:
    flags: enum
    enums: Default=0 SMPTE_170M_YCC=1 BT709_YCC=2 XVYCC_601=3 XVYCC_709=4 SYCC_601=5 opYCC_601=6 opRGB=7 BT2020_CYCC=8 BT2020_RGB=9 BT2020_YCC=10 DCI-P3_RGB_D65=11 DCI-P3_RGB_Theater=12
    value: 0
    */

    CLog::Log(LOGDEBUG, "CVideoLayerBridgeDRMPRIME::{} - m_hdr_blob_id={}", __FUNCTION__, m_hdr_blob_id);
    m_DRM->AddProperty(connector, "HDR_OUTPUT_METADATA", m_hdr_blob_id);
    m_DRM->SetActive(true);
  }
}

void CVideoLayerBridgeDRMPRIME::SetVideoPlane(IVideoBufferDRMPRIME* buffer, const CRect& destRect)
{
  if (!Map(buffer))
  {
    Unmap(buffer);
    return;
  }

  struct plane* plane = m_DRM->GetVideoPlane();
  m_DRM->AddProperty(plane, "FB_ID", buffer->m_fb_id);
  m_DRM->AddProperty(plane, "CRTC_ID", m_DRM->GetCrtc()->crtc->crtc_id);
  m_DRM->AddProperty(plane, "SRC_X", 0);
  m_DRM->AddProperty(plane, "SRC_Y", 0);
  m_DRM->AddProperty(plane, "SRC_W", buffer->GetWidth() << 16);
  m_DRM->AddProperty(plane, "SRC_H", buffer->GetHeight() << 16);
  m_DRM->AddProperty(plane, "CRTC_X", static_cast<int32_t>(destRect.x1) & ~1);
  m_DRM->AddProperty(plane, "CRTC_Y", static_cast<int32_t>(destRect.y1) & ~1);
  m_DRM->AddProperty(plane, "CRTC_W", (static_cast<uint32_t>(destRect.Width()) + 1) & ~1);
  m_DRM->AddProperty(plane, "CRTC_H", (static_cast<uint32_t>(destRect.Height()) + 1) & ~1);
}

void CVideoLayerBridgeDRMPRIME::UpdateVideoPlane()
{
  if (!m_buffer || !m_buffer->m_fb_id)
    return;

  // release the buffer that is no longer presented on screen
  Release(m_prev_buffer);
  m_prev_buffer = nullptr;

  struct plane* plane = m_DRM->GetVideoPlane();
  m_DRM->AddProperty(plane, "FB_ID", m_buffer->m_fb_id);
  m_DRM->AddProperty(plane, "CRTC_ID", m_DRM->GetCrtc()->crtc->crtc_id);
}
