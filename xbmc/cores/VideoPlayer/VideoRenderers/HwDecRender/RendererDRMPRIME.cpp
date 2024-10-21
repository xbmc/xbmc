/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererDRMPRIME.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/VideoLayerBridgeDRMPRIME.h"
#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/gbm/WinSystemGbm.h"
#include "windowing/gbm/drm/DRMAtomic.h"

using namespace KODI::WINDOWING::GBM;

const std::string SETTING_VIDEOPLAYER_USEPRIMERENDERER = "videoplayer.useprimerenderer";

CRendererDRMPRIME::~CRendererDRMPRIME()
{
  Flush(false);
}

CBaseRenderer* CRendererDRMPRIME::Create(CVideoBuffer* buffer)
{
  if (buffer && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                    SETTING_VIDEOPLAYER_USEPRIMERENDERER) == 0)
  {
    auto buf = dynamic_cast<CVideoBufferDRMPRIME*>(buffer);
    if (!buf)
      return nullptr;

    auto winSystem = static_cast<CWinSystemGbm*>(CServiceBroker::GetWinSystem());
    if (!winSystem)
      return nullptr;

    auto drm = std::static_pointer_cast<CDRMAtomic>(winSystem->GetDrm());
    if (!drm)
      return nullptr;

    if (!buf->AcquireDescriptor())
      return nullptr;

    AVDRMFrameDescriptor* desc = buf->GetDescriptor();
    if (!desc)
    {
      buf->ReleaseDescriptor();
      return nullptr;
    }

    AVDRMLayerDescriptor* layer = &desc->layers[0];
    uint32_t format = layer->format;
    uint64_t modifier = desc->objects[0].format_modifier;

    buf->ReleaseDescriptor();

    auto gui = drm->GetGuiPlane();
    if (!gui)
      return nullptr;

    if (!gui->SupportsFormat(CDRMUtils::FourCCWithAlpha(gui->GetFormat())))
      return nullptr;

    auto plane = drm->GetVideoPlane();
    if (!plane)
      return nullptr;

    if (!plane->SupportsFormatAndModifier(format, modifier))
      return nullptr;

    return new CRendererDRMPRIME();
  }

  return nullptr;
}

void CRendererDRMPRIME::Register()
{
  CWinSystemGbm* winSystem = dynamic_cast<CWinSystemGbm*>(CServiceBroker::GetWinSystem());
  if (winSystem && winSystem->GetDrm()->GetVideoPlane() &&
      std::dynamic_pointer_cast<CDRMAtomic>(winSystem->GetDrm()))
  {
    CServiceBroker::GetSettingsComponent()
        ->GetSettings()
        ->GetSetting(SETTING_VIDEOPLAYER_USEPRIMERENDERER)
        ->SetVisible(true);
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
    CalcDestRect(0, 0, info.iScreenWidth, info.iScreenHeight,
                 GetAspectRatio() * CDisplaySettings::GetInstance().GetPixelRatio(),
                 CDisplaySettings::GetInstance().GetZoomAmount(),
                 CDisplaySettings::GetInstance().GetVerticalShift(), m_planeDestRect);
  }
  else
  {
    m_planeDestRect = m_destRect;
  }
}

void CRendererDRMPRIME::AddVideoPicture(const VideoPicture& picture, int index)
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

void CRendererDRMPRIME::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (m_iLastRenderBuffer == index && m_videoLayerBridge)
  {
    m_videoLayerBridge->UpdateVideoPlane();
    return;
  }

  CVideoBufferDRMPRIME* buffer = dynamic_cast<CVideoBufferDRMPRIME*>(m_buffers[index].videoBuffer);
  if (!buffer || !buffer->IsValid())
    return;

  if (!m_videoLayerBridge)
  {
    CWinSystemGbm* winSystem = static_cast<CWinSystemGbm*>(CServiceBroker::GetWinSystem());
    m_videoLayerBridge =
        std::dynamic_pointer_cast<CVideoLayerBridgeDRMPRIME>(winSystem->GetVideoLayerBridge());
    if (!m_videoLayerBridge)
      m_videoLayerBridge = std::make_shared<CVideoLayerBridgeDRMPRIME>(
          std::dynamic_pointer_cast<CDRMAtomic>(winSystem->GetDrm()));
    winSystem->RegisterVideoLayerBridge(m_videoLayerBridge);
  }

  if (m_iLastRenderBuffer == -1)
    m_videoLayerBridge->Configure(buffer);

  m_videoLayerBridge->SetVideoPlane(buffer, m_planeDestRect);

  m_iLastRenderBuffer = index;
}

bool CRendererDRMPRIME::RenderCapture(int index, CRenderCapture* capture)
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

bool CRendererDRMPRIME::Supports(ERENDERFEATURE feature) const
{
  switch (feature)
  {
    case RENDERFEATURE_STRETCH:
    case RENDERFEATURE_ZOOM:
    case RENDERFEATURE_VERTICAL_SHIFT:
    case RENDERFEATURE_PIXEL_RATIO:
      return true;
    default:
      return false;
  }
}

bool CRendererDRMPRIME::Supports(ESCALINGMETHOD method) const
{
  return false;
}
