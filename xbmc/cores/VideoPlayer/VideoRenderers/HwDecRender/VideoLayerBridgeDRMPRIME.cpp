/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLayerBridgeDRMPRIME.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/gbm/drm/DRMAtomic.h"

#include <utility>

using namespace KODI::WINDOWING::GBM;
using namespace DRMPRIME;

CVideoLayerBridgeDRMPRIME::CVideoLayerBridgeDRMPRIME(std::shared_ptr<CDRMAtomic> drm)
  : m_DRM(std::move(drm))
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
  auto plane = m_DRM->GetVideoPlane();
  m_DRM->AddProperty(plane, "FB_ID", 0);
  m_DRM->AddProperty(plane, "CRTC_ID", 0);

  auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  // disable HDR metadata
  winSystem->SetHDR(nullptr);
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

  if (!buffer->AcquireDescriptor())
  {
    CLog::Log(LOGERROR, "CVideoLayerBridgeDRMPRIME::{} - failed to acquire descriptor",
              __FUNCTION__);
    return false;
  }

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  uint32_t handles[4] = {}, pitches[4] = {}, offsets[4] = {}, flags = 0;
  uint64_t modifier[4] = {};
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
    if (handle)
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
      struct drm_gem_close gem_close;
      gem_close.handle = buffer->m_handles[i];
      drmIoctl(m_DRM->GetFileDescriptor(), DRM_IOCTL_GEM_CLOSE, &gem_close);
      buffer->m_handles[i] = 0;
    }
  }

  buffer->ReleaseDescriptor();
}

void CVideoLayerBridgeDRMPRIME::Configure(CVideoBufferDRMPRIME* buffer)
{
  auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  const VideoPicture& picture = buffer->GetPicture();

  winSystem->SetHDR(&picture);

  auto plane = m_DRM->GetVideoPlane();

  std::optional<uint64_t> colorEncoding =
      plane->GetPropertyValue("COLOR_ENCODING", GetColorEncoding(picture));
  if (colorEncoding)
    m_DRM->AddProperty(plane, "COLOR_ENCODING", colorEncoding.value());

  std::optional<uint64_t> colorRange =
      plane->GetPropertyValue("COLOR_RANGE", GetColorRange(picture));
  if (colorRange)
    m_DRM->AddProperty(plane, "COLOR_RANGE", colorRange.value());
}

void CVideoLayerBridgeDRMPRIME::SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect)
{
  if (!Map(buffer))
  {
    Unmap(buffer);
    return;
  }

  auto plane = m_DRM->GetVideoPlane();
  m_DRM->AddProperty(plane, "FB_ID", buffer->m_fb_id);
  m_DRM->AddProperty(plane, "CRTC_ID", m_DRM->GetCrtc()->GetCrtcId());
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

  auto plane = m_DRM->GetVideoPlane();
  m_DRM->AddProperty(plane, "FB_ID", m_buffer->m_fb_id);
  m_DRM->AddProperty(plane, "CRTC_ID", m_DRM->GetCrtc()->GetCrtcId());
}
