/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererStarfish.h"

#include "../RenderFactory.h"
#include "DVDCodecs/Video/DVDVideoCodecStarfish.h"
#include "ServiceBroker.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"
#include "windowing/wayland/WinSystemWaylandWebOS.h"

#include <appswitching-control-block/AcbAPI.h>

CRendererStarfish::CRendererStarfish()
{
  CLog::LogF(LOGINFO, "CRendererStarfish: Instanced");
}

CRendererStarfish::~CRendererStarfish() = default;

CBaseRenderer* CRendererStarfish::Create(CVideoBuffer* buffer)
{
  if (buffer && dynamic_cast<CStarfishVideoBuffer*>(buffer))
    return new CRendererStarfish();
  return nullptr;
}

bool CRendererStarfish::Configure(const VideoPicture& picture, float fps, unsigned int orientation)
{
  auto buffer = static_cast<CStarfishVideoBuffer*>(picture.videoBuffer);
  m_acbId = buffer->m_acbId;
  if (m_acbId)
  {
    EnableAlwaysClip();
  }
  m_iFlags = GetFlagsChromaPosition(picture.chroma_position) |
             GetFlagsColorMatrix(picture.color_space, picture.iWidth, picture.iHeight) |
             GetFlagsColorPrimaries(picture.color_primaries) |
             GetFlagsStereoMode(picture.stereoMode);

  m_format = picture.videoBuffer->GetFormat();
  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  m_configured = true;

  return true;
}

bool CRendererStarfish::IsConfigured()
{
  return m_configured;
}

bool CRendererStarfish::ConfigChanged(const VideoPicture& picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
  {
    return true;
  }

  return false;
}

bool CRendererStarfish::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("starfish", CRendererStarfish::Create);
  return true;
}

void CRendererStarfish::ManageRenderArea()
{
  CBaseRenderer::ManageRenderArea();

  if ((m_exportedDestRect != m_destRect || m_exportedSourceRect != m_sourceRect) &&
      !m_sourceRect.IsEmpty() && !m_destRect.IsEmpty())
  {
    auto origRect =
        CRect{0, 0, static_cast<float>(m_sourceWidth), static_cast<float>(m_sourceHeight)};
    using namespace KODI::WINDOWING::WAYLAND;
    auto winSystem = static_cast<CWinSystemWaylandWebOS*>(CServiceBroker::GetWinSystem());
    if (winSystem->SupportsExportedWindow())
    {
      winSystem->SetExportedWindow(origRect, m_sourceRect, m_destRect);
    }
    else if (m_acbId)
    {
      AcbAPI_setCustomDisplayWindow(m_acbId, m_sourceRect.x1, m_sourceRect.y1, m_sourceRect.Width(),
                                    m_sourceRect.Height(), m_destRect.x1, m_destRect.y1,
                                    m_destRect.Width(), m_destRect.Height(), false, nullptr);
    }
    m_exportedSourceRect = m_sourceRect;
    m_exportedDestRect = m_destRect;
  }
}

bool CRendererStarfish::Supports(ERENDERFEATURE feature) const
{
  return (feature == RENDERFEATURE_ZOOM || feature == RENDERFEATURE_STRETCH ||
          feature == RENDERFEATURE_PIXEL_RATIO || feature == RENDERFEATURE_VERTICAL_SHIFT ||
          feature == RENDERFEATURE_ROTATION);
}

bool CRendererStarfish::Supports(ESCALINGMETHOD method) const
{
  return false;
}

bool CRendererStarfish::SupportsMultiPassRendering()
{
  return false;
}

void CRendererStarfish::AddVideoPicture(const VideoPicture& picture, int index)
{
}

void CRendererStarfish::ReleaseBuffer(int idx)
{
}

CRenderInfo CRendererStarfish::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = 4;
  return info;
}

bool CRendererStarfish::IsGuiLayer()
{
  return false;
}

bool CRendererStarfish::RenderCapture(CRenderCapture* capture)
{
  return false;
}

void CRendererStarfish::UnInit()
{
  m_configured = false;
}

void CRendererStarfish::Update()
{
  if (!m_configured)
  {
    return;
  }

  ManageRenderArea();
}

void CRendererStarfish::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (!m_configured)
  {
    return;
  }
}
