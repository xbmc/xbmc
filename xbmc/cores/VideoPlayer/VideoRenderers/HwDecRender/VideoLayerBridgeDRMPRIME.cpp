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

  // the plane-off commit from Disable has run by now, so these are plain frees
  for (uint32_t fbId : m_fbCache.TakeAll())
    drmModeRmFB(m_DRM->GetFileDescriptor(), fbId);
}

void CVideoLayerBridgeDRMPRIME::Disable()
{
  auto plane = m_DRM->GetVideoPlane();
  if (!plane)
    return;

  // disable video plane
  m_DRM->AddProperty(plane, "FB_ID", 0);
  m_DRM->AddProperty(plane, "CRTC_ID", 0);
}

void CVideoLayerBridgeDRMPRIME::Acquire(CVideoBufferDRMPRIME* buffer, uint32_t fbId)
{
  // release the buffer that is no longer presented on screen
  Release(m_prev_buffer);

  // release the buffer currently being presented next call
  m_prev_buffer = m_buffer;
  m_prev_fb_id = m_fb_id;

  // reference count the buffer that is going to be presented on screen
  m_buffer = buffer;
  m_fb_id = fbId;
  m_buffer->Acquire();
}

void CVideoLayerBridgeDRMPRIME::Release(CVideoBufferDRMPRIME* buffer)
{
  if (!buffer)
    return;

  buffer->Release();
}

bool CVideoLayerBridgeDRMPRIME::PrepareBuffer(CVideoBufferDRMPRIME* buffer)
{
  if (!buffer->AcquireDescriptor())
  {
    CLog::Log(LOGERROR, "CVideoLayerBridgeDRMPRIME::{} - failed to acquire descriptor",
              __FUNCTION__);
    return false;
  }

  const auto identity = DRMPRIME::ComputeDmaBufIdentity(buffer->GetDescriptor(), buffer->GetWidth(),
                                                        buffer->GetHeight());
  if (!identity)
  {
    buffer->ReleaseDescriptor();
    CLog::Log(LOGERROR, "CVideoLayerBridgeDRMPRIME::{} - failed to identify buffer memory",
              __FUNCTION__);
    return false;
  }

  uint32_t fbId = m_fbCache.Lookup(*identity);
  if (!fbId)
  {
    fbId = CreateFramebuffer(buffer);
    if (!fbId)
    {
      buffer->ReleaseDescriptor();
      return false;
    }
    m_fbCache.Insert(*identity, fbId);
  }
  buffer->ReleaseDescriptor();

  if (m_buffer != buffer)
    Acquire(buffer, fbId);
  else
    m_fb_id = fbId;

  // reap after the id shift so protection covers the new presented pair
  for (uint32_t doomed : m_fbCache.Reap(m_fb_id, m_prev_fb_id))
    drmModeRmFB(m_DRM->GetFileDescriptor(), doomed);

  return true;
}

uint32_t CVideoLayerBridgeDRMPRIME::CreateFramebuffer(CVideoBufferDRMPRIME* buffer)
{
  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  uint32_t fbId = 0;
  uint32_t objectHandles[AV_DRM_MAX_PLANES] = {};
  uint32_t handles[4] = {}, pitches[4] = {}, offsets[4] = {}, flags = 0;
  uint64_t modifier[4] = {};
  int ret = 0;

  // convert Prime FD to GEM handle
  for (int object = 0; object < descriptor->nb_objects; object++)
  {
    ret = drmPrimeFDToHandle(m_DRM->GetFileDescriptor(), descriptor->objects[object].fd,
                             &objectHandles[object]);
    if (ret < 0)
    {
      CLog::Log(LOGERROR,
                "CVideoLayerBridgeDRMPRIME::{} - failed to convert prime fd {} to gem handle {}, "
                "ret = {}",
                __FUNCTION__, descriptor->objects[object].fd, objectHandles[object], ret);
      break;
    }
  }

  if (ret == 0)
  {
    AVDRMLayerDescriptor* layer = &descriptor->layers[0];

    for (int plane = 0; plane < layer->nb_planes; plane++)
    {
      int object = layer->planes[plane].object_index;
      uint32_t handle = objectHandles[object];
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
                                     modifier, &fbId, flags);
    if (ret < 0)
      CLog::Log(LOGERROR,
                "CVideoLayerBridgeDRMPRIME::{} - failed to add fb, format {:#x} modifier {:#x} "
                "ret = {}",
                __FUNCTION__, layer->format, modifier[0], ret);
  }

  // close the GEM handles now: the fb holds its own references, and the
  // dedup entry drops with them, so a later importer of this dma-buf (the
  // screencap EGL import) solely owns a fresh handle instead of sharing ours
  for (int i = 0; i < AV_DRM_MAX_PLANES; i++)
  {
    if (objectHandles[i])
    {
      struct drm_gem_close gem_close;
      gem_close.handle = objectHandles[i];
      drmIoctl(m_DRM->GetFileDescriptor(), DRM_IOCTL_GEM_CLOSE, &gem_close);
    }
  }

  if (ret < 0)
    return 0;

  return fbId;
}

void CVideoLayerBridgeDRMPRIME::Configure(CVideoBufferDRMPRIME* buffer)
{
  // a new renderer generation brings a new buffer pool; old entries can never match again
  m_fbCache.InvalidateAll();
  for (uint32_t doomed : m_fbCache.Reap(m_fb_id, m_prev_fb_id))
    drmModeRmFB(m_DRM->GetFileDescriptor(), doomed);

  auto plane = m_DRM->GetVideoPlane();
  if (!plane)
    return;

  const VideoPicture& picture = buffer->GetPicture();

  std::optional<uint64_t> colorEncoding =
      plane->GetPropertyEnumValue("COLOR_ENCODING", GetColorEncoding(picture));
  if (colorEncoding)
    m_DRM->AddProperty(plane, "COLOR_ENCODING", colorEncoding.value());

  std::optional<uint64_t> colorRange =
      plane->GetPropertyEnumValue("COLOR_RANGE", GetColorRange(picture));
  if (colorRange)
    m_DRM->AddProperty(plane, "COLOR_RANGE", colorRange.value());
}

void CVideoLayerBridgeDRMPRIME::SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect)
{
  auto plane = m_DRM->GetVideoPlane();
  if (!plane)
    return;

  if (!PrepareBuffer(buffer))
    return;

  m_DRM->AddProperty(plane, "FB_ID", m_fb_id);
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
  if (!m_buffer || !m_fb_id)
    return;

  auto plane = m_DRM->GetVideoPlane();
  if (!plane)
    return;

  m_DRM->AddProperty(plane, "FB_ID", m_fb_id);
  m_DRM->AddProperty(plane, "CRTC_ID", m_DRM->GetCrtc()->GetCrtcId());
}
