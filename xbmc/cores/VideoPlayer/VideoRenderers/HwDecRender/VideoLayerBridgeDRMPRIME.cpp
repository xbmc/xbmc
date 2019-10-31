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
using namespace DRMPRIME;

CVideoLayerBridgeDRMPRIME::CVideoLayerBridgeDRMPRIME(std::shared_ptr<CDRMUtils> drm) : m_DRM(drm)
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

void CVideoLayerBridgeDRMPRIME::Acquire(CVideoBufferDRMPRIME* buffer)
{
  // release the buffer that is no longer presented on screen
  Release(m_prev_buffer);

  // release the buffer currently being presented next call
  m_prev_buffer = m_buffer;

  // reference count the buffer that is going to be presented on screen
  m_buffer = buffer;
  m_buffer->Acquire();
}

void CVideoLayerBridgeDRMPRIME::Release(CVideoBufferDRMPRIME* buffer)
{
  if (!buffer)
    return;

  Unmap(buffer);
  buffer->Release();
}

bool CVideoLayerBridgeDRMPRIME::Map(CVideoBufferDRMPRIME* buffer)
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
    ret = drmPrimeFDToHandle(m_DRM->GetFileDescriptor(), descriptor->objects[object].fd,
                             &buffer->m_handles[object]);
    if (ret < 0)
    {
      CLog::Log(LOGERROR,
                "CVideoLayerBridgeDRMPRIME::{} - failed to convert prime fd {} to gem handle {}, "
                "ret = {}",
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
  ret = drmModeAddFB2WithModifiers(m_DRM->GetFileDescriptor(), buffer->GetWidth(),
                                   buffer->GetHeight(), layer->format, handles, pitches, offsets,
                                   modifier, &buffer->m_fb_id, flags);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CVideoLayerBridgeDRMPRIME::{} - failed to add fb {}, ret = {}",
              __FUNCTION__, buffer->m_fb_id, ret);
    return false;
  }

  Acquire(buffer);
  return true;
}

void CVideoLayerBridgeDRMPRIME::Unmap(CVideoBufferDRMPRIME* buffer)
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
      struct drm_gem_close gem_close = {.handle = buffer->m_handles[i]};
      drmIoctl(m_DRM->GetFileDescriptor(), DRM_IOCTL_GEM_CLOSE, &gem_close);
      buffer->m_handles[i] = 0;
    }
  }

  buffer->Unmap();
}

void CVideoLayerBridgeDRMPRIME::Configure(CVideoBufferDRMPRIME* buffer)
{
  const VideoPicture& picture = buffer->GetPicture();

  struct plane* plane = m_DRM->GetVideoPlane();
  if (m_DRM->SupportsProperty(plane, "COLOR_ENCODING") &&
      m_DRM->SupportsProperty(plane, "COLOR_RANGE"))
  {
    m_DRM->AddProperty(plane, "COLOR_ENCODING", GetColorEncoding(picture));
    m_DRM->AddProperty(plane, "COLOR_RANGE", GetColorRange(picture));
  }

  struct connector* connector = m_DRM->GetConnector();
  if (m_DRM->SupportsProperty(connector, "HDR_OUTPUT_METADATA"))
  {
    m_hdr_metadata.metadata_type = HDMI_STATIC_METADATA_TYPE1;
    m_hdr_metadata.hdmi_metadata_type1 = {
        .eotf = GetEOTF(picture),
        .metadata_type = HDMI_STATIC_METADATA_TYPE1,
    };

    if (m_hdr_blob_id)
      drmModeDestroyPropertyBlob(m_DRM->GetFileDescriptor(), m_hdr_blob_id);
    m_hdr_blob_id = 0;

    if (m_hdr_metadata.hdmi_metadata_type1.eotf)
    {
      const AVMasteringDisplayMetadata* mdmd = GetMasteringDisplayMetadata(picture);
      if (mdmd && mdmd->has_primaries)
      {
        // Convert to unsigned 16-bit values in units of 0.00002,
        // where 0x0000 represents zero and 0xC350 represents 1.0000
        for (int i = 0; i < 3; i++)
        {
          m_hdr_metadata.hdmi_metadata_type1.display_primaries[i].x =
              std::round(av_q2d(mdmd->display_primaries[i][0]) * 50000.0);
          m_hdr_metadata.hdmi_metadata_type1.display_primaries[i].y =
              std::round(av_q2d(mdmd->display_primaries[i][1]) * 50000.0);
        }
        m_hdr_metadata.hdmi_metadata_type1.white_point.x =
            std::round(av_q2d(mdmd->white_point[0]) * 50000.0);
        m_hdr_metadata.hdmi_metadata_type1.white_point.y =
            std::round(av_q2d(mdmd->white_point[1]) * 50000.0);
      }
      if (mdmd && mdmd->has_luminance)
      {
        // Convert to unsigned 16-bit value in units of 1 cd/m2,
        // where 0x0001 represents 1 cd/m2 and 0xFFFF represents 65535 cd/m2
        m_hdr_metadata.hdmi_metadata_type1.max_display_mastering_luminance =
            std::round(av_q2d(mdmd->max_luminance));

        // Convert to unsigned 16-bit value in units of 0.0001 cd/m2,
        // where 0x0001 represents 0.0001 cd/m2 and 0xFFFF represents 6.5535 cd/m2
        m_hdr_metadata.hdmi_metadata_type1.min_display_mastering_luminance =
            std::round(av_q2d(mdmd->min_luminance) * 10000.0);
      }

      const AVContentLightMetadata* clmd = GetContentLightMetadata(picture);
      if (clmd)
      {
        m_hdr_metadata.hdmi_metadata_type1.max_cll = clmd->MaxCLL;
        m_hdr_metadata.hdmi_metadata_type1.max_fall = clmd->MaxFALL;
      }

      drmModeCreatePropertyBlob(m_DRM->GetFileDescriptor(), &m_hdr_metadata, sizeof(m_hdr_metadata),
                                &m_hdr_blob_id);
    }

    m_DRM->AddProperty(connector, "HDR_OUTPUT_METADATA", m_hdr_blob_id);
    m_DRM->SetActive(true);
  }
}

void CVideoLayerBridgeDRMPRIME::SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect)
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
