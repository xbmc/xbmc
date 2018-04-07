/*
 *      Copyright (C) 2007-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
 : m_bConfigured(false)
{
  CLog::Log(LOGNOTICE, "Instancing CRendererMediaCodecSurface");
}

CRendererMediaCodecSurface::~CRendererMediaCodecSurface()
{
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

void CRendererMediaCodecSurface::AddVideoPicture(const VideoPicture &picture, int index, double currentClock)
{
  if (m_bConfigured && dynamic_cast<CMediaCodecVideoBuffer*>(picture.videoBuffer))
  {
    int64_t nanodiff(static_cast<int64_t>((picture.pts - currentClock) * 1000));
    dynamic_cast<CMediaCodecVideoBuffer*>(picture.videoBuffer)->RenderUpdate(m_surfDestRect,
      CurrentHostCounter() + nanodiff);
  }
}

void CRendererMediaCodecSurface::ReleaseBuffer(int idx)
{
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

void CRendererMediaCodecSurface::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  CXBMCApp::get()->WaitVSync(100);
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
