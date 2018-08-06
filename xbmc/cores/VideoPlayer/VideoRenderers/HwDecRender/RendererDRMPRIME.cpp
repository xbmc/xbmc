/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererDRMPRIME.h"

#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "ServiceBroker.h"
#include "settings/DisplaySettings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "windowing/gbm/DRMAtomic.h"
#include "windowing/GraphicContext.h"

const std::string SETTING_VIDEOPLAYER_USEPRIMERENDERER = "videoplayer.useprimerenderer";

CRendererDRMPRIME::~CRendererDRMPRIME()
{
  Flush(false);
}

CBaseRenderer* CRendererDRMPRIME::Create(CVideoBuffer* buffer)
{
  if (buffer && dynamic_cast<CVideoBufferDRMPRIME*>(buffer) &&
      CServiceBroker::GetSettings().GetInt(SETTING_VIDEOPLAYER_USEPRIMERENDERER) == 0)
  {
    CWinSystemGbmEGLContext* winSystem = dynamic_cast<CWinSystemGbmEGLContext*>(CServiceBroker::GetWinSystem());
    if (winSystem && winSystem->GetDrm()->GetPrimaryPlane()->plane &&
        std::dynamic_pointer_cast<CDRMAtomic>(winSystem->GetDrm()))
      return new CRendererDRMPRIME();
  }

  return nullptr;
}

void CRendererDRMPRIME::Register()
{
  CWinSystemGbmEGLContext* winSystem = dynamic_cast<CWinSystemGbmEGLContext*>(CServiceBroker::GetWinSystem());
  if (winSystem && winSystem->GetDrm()->GetPrimaryPlane()->plane &&
      std::dynamic_pointer_cast<CDRMAtomic>(winSystem->GetDrm()))
  {
    CServiceBroker::GetSettings().GetSetting(SETTING_VIDEOPLAYER_USEPRIMERENDERER)->SetVisible(true);
    VIDEOPLAYER::CRendererFactory::RegisterRenderer("drm_prime", CRendererDRMPRIME::Create);
    return;
  }
}

bool CRendererDRMPRIME::Configure(const VideoPicture& picture, float fps, unsigned int orientation)
{
  m_format = picture.videoBuffer->GetFormat();
  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;

  m_iFlags = GetFlagsChromaPosition(picture.chroma_position) |
             GetFlagsColorMatrix(picture.color_space, picture.iWidth, picture.iHeight) |
             GetFlagsColorPrimaries(picture.color_primaries) |
             GetFlagsStereoMode(picture.stereoMode);

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  Flush(false);

  m_bConfigured = true;
  return true;
}

void CRendererDRMPRIME::ManageRenderArea()
{
  CBaseRenderer::ManageRenderArea();

  RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  if (info.iScreenWidth != info.iWidth)
  {
    CalcNormalRenderRect(0, 0, info.iScreenWidth, info.iScreenHeight,
                         GetAspectRatio() * CDisplaySettings::GetInstance().GetPixelRatio(),
                         CDisplaySettings::GetInstance().GetZoomAmount(),
                         CDisplaySettings::GetInstance().GetVerticalShift());
  }
}

void CRendererDRMPRIME::AddVideoPicture(const VideoPicture& picture, int index, double currentClock)
{
  BUFFER& buf = m_buffers[index];
  if (buf.videoBuffer)
  {
    CLog::LogF(LOGERROR, "unreleased video buffer");
    buf.videoBuffer->Release();
  }
  buf.videoBuffer = picture.videoBuffer;
  buf.videoBuffer->Acquire();
}

bool CRendererDRMPRIME::Flush(bool saveBuffers)
{
  if (!saveBuffers)
    for (int i = 0; i < NUM_BUFFERS; i++)
      ReleaseBuffer(i);

  m_iLastRenderBuffer = -1;
  return saveBuffers;
}

void CRendererDRMPRIME::ReleaseBuffer(int index)
{
  BUFFER& buf = m_buffers[index];
  if (buf.videoBuffer)
  {
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

bool CRendererDRMPRIME::NeedBuffer(int index)
{
  if (m_iLastRenderBuffer == index)
    return true;

  CVideoBufferDRMPRIME* buffer = dynamic_cast<CVideoBufferDRMPRIME*>(m_buffers[index].videoBuffer);
  if (buffer && buffer->m_fb_id)
    return true;

  return false;
}

CRenderInfo CRendererDRMPRIME::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = NUM_BUFFERS;
  return info;
}

void CRendererDRMPRIME::Update()
{
  if (!m_bConfigured)
    return;

  ManageRenderArea();
}

void CRendererDRMPRIME::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (m_iLastRenderBuffer == index)
    return;

  CVideoBufferDRMPRIME* buffer = dynamic_cast<CVideoBufferDRMPRIME*>(m_buffers[index].videoBuffer);
  if (!buffer)
    return;

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  if (!descriptor || !descriptor->nb_layers)
    return;

  if (!m_videoLayerBridge)
  {
    CWinSystemGbmEGLContext* winSystem = static_cast<CWinSystemGbmEGLContext*>(CServiceBroker::GetWinSystem());
    m_videoLayerBridge = std::dynamic_pointer_cast<CVideoLayerBridgeDRMPRIME>(winSystem->GetVideoLayerBridge());
    if (!m_videoLayerBridge)
      m_videoLayerBridge = std::make_shared<CVideoLayerBridgeDRMPRIME>(winSystem->GetDrm());
    winSystem->RegisterVideoLayerBridge(m_videoLayerBridge);
  }

  if (m_iLastRenderBuffer == -1)
    m_videoLayerBridge->Configure(buffer);

  m_videoLayerBridge->SetVideoPlane(buffer, m_destRect);

  m_iLastRenderBuffer = index;
}

bool CRendererDRMPRIME::RenderCapture(CRenderCapture* capture)
{
  capture->BeginRender();
  capture->EndRender();
  return true;
}

bool CRendererDRMPRIME::ConfigChanged(const VideoPicture& picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
    return true;

  return false;
}

bool CRendererDRMPRIME::Supports(ERENDERFEATURE feature)
{
  if (feature == RENDERFEATURE_ZOOM ||
      feature == RENDERFEATURE_STRETCH ||
      feature == RENDERFEATURE_PIXEL_RATIO)
    return true;

  return false;
}

bool CRendererDRMPRIME::Supports(ESCALINGMETHOD method)
{
  return false;
}

//------------------------------------------------------------------------------

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
  struct plane* plane = m_DRM->GetPrimaryPlane();
  m_DRM->AddProperty(plane, "FB_ID", 0);
  m_DRM->AddProperty(plane, "CRTC_ID", 0);
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
      struct drm_gem_close gem_close = { .handle = buffer->m_handles[i] };
      drmIoctl(m_DRM->GetFileDescriptor(), DRM_IOCTL_GEM_CLOSE, &gem_close);
      buffer->m_handles[i] = 0;
    }
  }
}

void CVideoLayerBridgeDRMPRIME::Configure(CVideoBufferDRMPRIME* buffer)
{
}

void CVideoLayerBridgeDRMPRIME::SetVideoPlane(CVideoBufferDRMPRIME* buffer, const CRect& destRect)
{
  if (!Map(buffer))
  {
    Unmap(buffer);
    return;
  }

  struct plane* plane = m_DRM->GetPrimaryPlane();
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
