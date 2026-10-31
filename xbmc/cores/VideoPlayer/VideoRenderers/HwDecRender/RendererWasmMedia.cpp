/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererWasmMedia.h"

#include "cores/VideoPlayer/Buffers/VideoBufferWasmMedia.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"

#include "platform/wasm/WasmMediaLayer.h"

#include <cmath>

CRendererWasmMedia::~CRendererWasmMedia()
{
  wasm_media_set_display_rect(0, 0, 0, 0);
}

CBaseRenderer* CRendererWasmMedia::Create(CVideoBuffer* buffer)
{
  if (dynamic_cast<CVideoBufferWasmMedia*>(buffer))
    return new CRendererWasmMedia();
  return nullptr;
}

bool CRendererWasmMedia::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("wasmmedia", CRendererWasmMedia::Create);
  return true;
}

bool CRendererWasmMedia::Configure(const VideoPicture& picture, float fps, unsigned int orientation)
{
  m_format = picture.videoBuffer->GetFormat();
  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;
  m_iFlags = GetFlagsStereoMode(picture.stereoMode);

  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  m_configured = true;
  return true;
}

bool CRendererWasmMedia::ConfigChanged(const VideoPicture& picture)
{
  return picture.videoBuffer->GetFormat() != m_format;
}

CRenderInfo CRendererWasmMedia::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = 4;
  return info;
}

bool CRendererWasmMedia::Supports(ERENDERFEATURE feature) const
{
  return feature == RENDERFEATURE_ZOOM || feature == RENDERFEATURE_STRETCH ||
         feature == RENDERFEATURE_PIXEL_RATIO || feature == RENDERFEATURE_VERTICAL_SHIFT;
}

void CRendererWasmMedia::UnInit()
{
  m_configured = false;
}

void CRendererWasmMedia::RenderUpdate(
    int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  if (m_configured)
    ManageRenderArea();
}

void CRendererWasmMedia::ManageRenderArea()
{
  CBaseRenderer::ManageRenderArea();

  if (m_destRect.IsEmpty() || m_destRect == m_placedRect)
    return;

  wasm_media_set_display_rect(static_cast<int>(std::lround(m_destRect.x1)),
                              static_cast<int>(std::lround(m_destRect.y1)),
                              static_cast<int>(std::lround(m_destRect.Width())),
                              static_cast<int>(std::lround(m_destRect.Height())));
  m_placedRect = m_destRect;
}
