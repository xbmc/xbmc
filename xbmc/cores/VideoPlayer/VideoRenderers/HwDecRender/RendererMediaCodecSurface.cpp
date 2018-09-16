/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererMediaCodecSurface.h"

#include "../RenderCapture.h"
#include "../RenderFlags.h"
#include "windowing/GraphicContext.h"
#include "rendering/RenderSystem.h"
#include "settings/MediaSettings.h"
#include "platform/android/activity/XBMCApp.h"
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "../RenderFactory.h"

#include <thread>
#include <chrono>

CRendererMediaCodecSurface::CRendererMediaCodecSurface()
{
  CLog::Log(LOGNOTICE, "Instancing CRendererMediaCodecSurface");
}

CRendererMediaCodecSurface::~CRendererMediaCodecSurface()
{
  Reset();
}

CBaseRenderer* CRendererMediaCodecSurface::Create(CVideoBuffer *buffer)
{
  if (buffer && dynamic_cast<CMediaCodecVideoBuffer*>(buffer) && !dynamic_cast<CMediaCodecVideoBuffer*>(buffer)->HasSurfaceTexture())
    return new CRendererMediaCodecSurface();
  return nullptr;
}

bool CRendererMediaCodecSurface::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("mediacodec_surface", CRendererMediaCodecSurface::Create);
  return true;
}

bool CRendererMediaCodecSurface::Configure(const VideoPicture &picture, float fps, unsigned int orientation)
{
  CLog::Log(LOGNOTICE, "CRendererMediaCodecSurface::Configure");

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

  return true;
}

CRenderInfo CRendererMediaCodecSurface::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = info.optimal_buffer_size = 4;
  return info;
}

bool CRendererMediaCodecSurface::RenderCapture(CRenderCapture* capture)
{
  capture->BeginRender();
  capture->EndRender();
  return true;
}

void CRendererMediaCodecSurface::AddVideoPicture(const VideoPicture &picture, int index)
{
  ReleaseBuffer(index);

  BUFFER &buf(m_buffers[index]);
  if (picture.videoBuffer)
  {
    buf.videoBuffer = picture.videoBuffer;
    buf.videoBuffer->Acquire();
  }
}

void CRendererMediaCodecSurface::ReleaseVideoBuffer(int idx, bool render)
{
  BUFFER &buf(m_buffers[idx]);
  if (buf.videoBuffer)
  {
    CMediaCodecVideoBuffer *mcvb(dynamic_cast<CMediaCodecVideoBuffer*>(buf.videoBuffer));
    if (mcvb)
    {
      if (render && m_bConfigured)
        mcvb->RenderUpdate(m_surfDestRect, CXBMCApp::GetNextFrameTime());
      else
        mcvb->ReleaseOutputBuffer(render, 0);
    }
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

void CRendererMediaCodecSurface::ReleaseBuffer(int idx)
{
  ReleaseVideoBuffer(idx, false);
}

bool CRendererMediaCodecSurface::Supports(ERENDERFEATURE feature)
{
  if (feature == RENDERFEATURE_ZOOM ||
    feature == RENDERFEATURE_STRETCH ||
    feature == RENDERFEATURE_PIXEL_RATIO ||
    feature == RENDERFEATURE_ROTATION)
    return true;

  return false;
}

void CRendererMediaCodecSurface::Reset()
{
  for (int i = 0 ; i < 4 ; ++i)
    ReleaseVideoBuffer(i, false);
  m_lastIndex = -1;
}

void CRendererMediaCodecSurface::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  m_bConfigured = true;

  // this hack is needed to get the 2D mode of a 3D movie going
  RENDER_STEREO_MODE stereo_mode = CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
  if (stereo_mode)
    CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoView(RENDER_STEREO_VIEW_LEFT);

  ManageRenderArea();

  if (stereo_mode)
    CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoView(RENDER_STEREO_VIEW_OFF);

  m_surfDestRect = m_destRect;
  switch (stereo_mode)
  {
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
      m_surfDestRect.y2 *= 2.0;
      break;
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:
      m_surfDestRect.x2 *= 2.0;
      break;
    case RENDER_STEREO_MODE_MONO:
      m_surfDestRect.y2 = m_surfDestRect.y2 * (m_surfDestRect.y2 / m_sourceRect.y2);
      m_surfDestRect.x2 = m_surfDestRect.x2 * (m_surfDestRect.x2 / m_sourceRect.x2);
      break;
    default:
      break;
  }

  if (index != m_lastIndex)
  {
    ReleaseVideoBuffer(index, true);
    m_lastIndex = index;
  }
}

void CRendererMediaCodecSurface::ReorderDrawPoints()
{
  CBaseRenderer::ReorderDrawPoints();

  // Handle orientation
  switch (m_renderOrientation)
  {
    case 90:
    case 270:
    {
      double scale = (double)m_surfDestRect.Height() / m_surfDestRect.Width();
      int diff = (int) ((m_surfDestRect.Height()*scale - m_surfDestRect.Width()) / 2);
      m_surfDestRect = CRect(m_surfDestRect.x1 - diff, m_surfDestRect.y1, m_surfDestRect.x2 + diff, m_surfDestRect.y2);
    }
    default:
      break;
  }
}
