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

#if defined(TARGET_ANDROID)

#include "RendererMediaCodecSurface.h"

#include "../RenderCapture.h"
#include "guilib/GraphicContext.h"
#include "rendering/RenderSystem.h"
#include "settings/MediaSettings.h"
#include "platform/android/activity/XBMCApp.h"
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "utils/log.h"
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

bool CRendererMediaCodecSurface::Configure(const VideoPicture &picture, float fps, unsigned flags, unsigned int orientation)
{
  CLog::Log(LOGNOTICE, "CRendererMediaCodecSurface::Configure");

  m_sourceWidth = picture.iWidth;
  m_sourceHeight = picture.iHeight;
  m_renderOrientation = orientation;

  // Save the flags.
  m_iFlags = flags;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);

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
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() + nanodiff);
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

void CRendererMediaCodecSurface::Reset()
{
}

void CRendererMediaCodecSurface::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  CXBMCApp::get()->WaitVSync(100);
  ManageRenderArea();
  m_bConfigured = true;
}

void CRendererMediaCodecSurface::ReorderDrawPoints()
{
  CBaseRenderer::ReorderDrawPoints();

  // this hack is needed to get the 2D mode of a 3D movie going
  RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
  if (stereo_mode)
    g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_LEFT);

  if (stereo_mode)
    g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_OFF);

  m_surfDestRect = m_destRect;
  CRect srcRect(m_sourceRect);
  switch (stereo_mode)
  {
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
      m_surfDestRect.y2 *= 2.0;
      srcRect.y2 *= 2.0;
      break;
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:
      m_surfDestRect.x2 *= 2.0;
      srcRect.x2 *= 2.0;
      break;
    default:
      break;
  }

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

#endif
