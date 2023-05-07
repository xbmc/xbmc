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
  m_iFlags = GetFlagsChromaPosition(picture.chroma_position) |
             GetFlagsColorMatrix(picture.color_space, picture.iWidth, picture.iHeight) |
             GetFlagsColorPrimaries(picture.color_primaries) |
             GetFlagsStereoMode(picture.stereoMode);

  return CLinuxRendererGLES::Configure(picture, fps, orientation);
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

    winSystem->SetExportedWindow(origRect, m_sourceRect, m_destRect);
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

bool CRendererStarfish::LoadShadersHook()
{
  return true;
}

bool CRendererStarfish::RenderHook(int index)
{
  return true;
}

bool CRendererStarfish::CreateTexture(int index)
{
  return true;
}

void CRendererStarfish::DeleteTexture(int index)
{
}

bool CRendererStarfish::UploadTexture(int index)
{
  return true;
}

bool CRendererStarfish::IsGuiLayer()
{
  return false;
}

void CRendererStarfish::RenderUpdateVideo(bool clear, unsigned int flags, unsigned int alpha)
{
}
