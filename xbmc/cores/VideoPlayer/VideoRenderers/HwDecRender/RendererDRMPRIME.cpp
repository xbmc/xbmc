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

CRendererDRMPRIME::CRendererDRMPRIME(std::shared_ptr<CDRMUtils> drm)
  : m_DRM(drm)
{
}

CRendererDRMPRIME::~CRendererDRMPRIME()
{
  Reset();
}

CBaseRenderer* CRendererDRMPRIME::Create(CVideoBuffer* buffer)
{
  if (buffer && dynamic_cast<CVideoBufferDRMPRIME*>(buffer) &&
      CServiceBroker::GetSettings().GetInt(SETTING_VIDEOPLAYER_USEPRIMERENDERER) == 0)
  {
    CWinSystemGbmGLESContext* winSystem = dynamic_cast<CWinSystemGbmGLESContext*>(CServiceBroker::GetWinSystem());
    if (winSystem)
      return new CRendererDRMPRIME(winSystem->GetDrm());
  }

  return nullptr;
}

void CRendererDRMPRIME::Register()
{
  CWinSystemGbmGLESContext* winSystem = dynamic_cast<CWinSystemGbmGLESContext*>(CServiceBroker::GetWinSystem());
  if (winSystem && winSystem->GetDrm()->GetPrimaryPlane()->plane &&
      std::dynamic_pointer_cast<CDRMAtomic>(winSystem->GetDrm()))
  {
    VIDEOPLAYER::CRendererFactory::RegisterRenderer("drm_prime", CRendererDRMPRIME::Create);
    return;
  }

  CServiceBroker::GetSettings().SetInt(SETTING_VIDEOPLAYER_USEPRIMERENDERER, 1);
  CServiceBroker::GetSettings().GetSetting(SETTING_VIDEOPLAYER_USEPRIMERENDERER)->SetVisible(false);
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

  Reset();

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

  // delay Release of videoBuffer after a Flush call to prevent drmModeRmFB of a videoBuffer tied to a drm plane
  // TODO: move Release to Flush once current videoBuffer tied to a drm plane is reference counted
  if (buf.videoBuffer)
    buf.videoBuffer->Release();

  buf.videoBuffer = picture.videoBuffer;
  buf.videoBuffer->Acquire();
}

void CRendererDRMPRIME::Reset()
{
  for (int i = 0; i < NUM_BUFFERS; i++)
    ReleaseBuffer(i);

  m_iLastRenderBuffer = -1;
}

void CRendererDRMPRIME::Flush()
{
  m_iLastRenderBuffer = -1;
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
  return m_iLastRenderBuffer == index;
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
  if (buffer)
    SetVideoPlane(buffer);

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

void CRendererDRMPRIME::SetVideoPlane(CVideoBufferDRMPRIME* buffer)
{
  buffer->m_drm_fd = m_DRM->GetFileDescriptor();

  AVDRMFrameDescriptor* descriptor = buffer->GetDescriptor();
  if (descriptor && descriptor->nb_layers)
  {
    uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
    uint64_t modifier[4] = {0};
    int ret;

    // convert Prime FD to GEM handle
    for (int object = 0; object < descriptor->nb_objects; object++)
    {
      ret = drmPrimeFDToHandle(m_DRM->GetFileDescriptor(), descriptor->objects[object].fd, &buffer->m_handles[object]);
      if (ret < 0)
      {
        CLog::Log(LOGERROR, "CRendererDRMPRIME::%s - failed to retrieve the GEM handle from prime fd %d, ret = %d", __FUNCTION__, descriptor->objects[object].fd, ret);
        return;
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

    // add the video frame FB
    ret = drmModeAddFB2WithModifiers(m_DRM->GetFileDescriptor(), buffer->GetWidth(), buffer->GetHeight(), layer->format, handles, pitches, offsets, modifier, &buffer->m_fb_id, 0);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "CRendererDRMPRIME::%s - failed to add drm layer %d, ret = %d", __FUNCTION__, buffer->m_fb_id, ret);
      return;
    }

    int32_t crtc_x = static_cast<int32_t>(m_destRect.x1) & ~1;
    int32_t crtc_y = static_cast<int32_t>(m_destRect.y1) & ~1;
    uint32_t crtc_w = (static_cast<uint32_t>(m_destRect.Width()) + 1) & ~1;
    uint32_t crtc_h = (static_cast<uint32_t>(m_destRect.Height()) + 1) & ~1;
    uint32_t src_x = 0;
    uint32_t src_y = 0;
    uint32_t src_w = buffer->GetWidth() << 16;
    uint32_t src_h = buffer->GetHeight() << 16;

    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "FB_ID",   buffer->m_fb_id);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "CRTC_ID", m_DRM->GetCrtc()->crtc->crtc_id);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "SRC_X",   src_x);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "SRC_Y",   src_y);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "SRC_W",   src_w);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "SRC_H",   src_h);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "CRTC_X",  crtc_x);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "CRTC_Y",  crtc_y);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "CRTC_W",  crtc_w);
    m_DRM->AddProperty(m_DRM->GetPrimaryPlane(), "CRTC_H",  crtc_h);
  }
}
