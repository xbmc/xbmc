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

CRendererMediaCodecSurface::CRendererMediaCodecSurface()
  : m_iRenderBuffer(0)
  , m_prevTime(std::chrono::system_clock::now())
  , m_bConfigured(false)
  , m_updateCount(10)
{
  CLog::Log(LOGNOTICE, "Instancing CRendererMediaCodecSurface");
}

CRendererMediaCodecSurface::~CRendererMediaCodecSurface()
{
  for (int i(0); i < m_numRenderBuffers; ++i)
    ReleaseBuffer(i);
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

  m_bConfigured = true;

  return true;
}

CRenderInfo CRendererMediaCodecSurface::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = m_numRenderBuffers;
  info.optimal_buffer_size = m_numRenderBuffers;
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
  ReleaseBuffer(index);

  BUFFER &buf(m_buffers[index]);
  if (picture.videoBuffer && (buf.videoBuffer = dynamic_cast<CMediaCodecVideoBuffer*>(picture.videoBuffer)))
    buf.videoBuffer->Acquire();
}

void CRendererMediaCodecSurface::ReleaseBuffer(int idx)
{
  BUFFER &buf(m_buffers[idx]);
  if (buf.videoBuffer)
  {
    buf.videoBuffer->ReleaseOutputBuffer(false);
    buf.videoBuffer->Release();
    buf.videoBuffer = nullptr;
  }
}

void CRendererMediaCodecSurface::FlipPage(int source)
{
  if (source >= 0 && source < m_numRenderBuffers)
    m_iRenderBuffer = source;
  else
    m_iRenderBuffer = (m_iRenderBuffer + 1) % m_numRenderBuffers;

  // Android SurfaceFlinger has its own clock, so we can release frames early.
  // Benefit of this place is that it is called from render-thread and not
  // affected by gui stalls when opening overlay dialogs
  BUFFER &buf(m_buffers[m_iRenderBuffer]);
  if (buf.videoBuffer)
    buf.videoBuffer->RenderUpdate(m_surfDestRect);
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
  m_updateCount = 10;
}

void CRendererMediaCodecSurface::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  {
    std::chrono::milliseconds elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_prevTime).count());
    if (elapsed < std::chrono::milliseconds(10))
      std::this_thread::sleep_for(std::chrono::milliseconds(10) - elapsed);

    // ManageRenderArea every 100ms.
    m_updateCount += (elapsed.count() / 10) + 1;
    if (m_updateCount > 10)
    {
      ManageRenderArea();
      m_updateCount = 0;
    }
    m_prevTime = std::chrono::system_clock::now();
  }
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
