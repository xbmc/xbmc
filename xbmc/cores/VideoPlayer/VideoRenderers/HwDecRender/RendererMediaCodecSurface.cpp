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
#include <thread>

CRendererMediaCodecSurface::CRendererMediaCodecSurface()
  : m_iRenderBuffer(0)
  , m_prevTime(std::chrono::system_clock::now())
  , m_bConfigured(false)
  , m_renderingStarted(false)
  , m_updateCount(10)
{
}

CRendererMediaCodecSurface::~CRendererMediaCodecSurface()
{
  for (int i(0); i < m_numRenderBuffers; ++i)
    ReleaseBuffer(i);
}

bool CRendererMediaCodecSurface::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, void *hwPic, unsigned int orientation)
{
  CLog::Log(LOGNOTICE, "CRendererMediaCodecSurface::Configure");

  m_sourceWidth = width;
  m_sourceHeight = height;
  m_renderOrientation = orientation;

  // Save the flags.
  m_iFlags = flags;
  m_format = format;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);

  m_bConfigured = true;

  for (int i = 0; i < m_numRenderBuffers; ++i)
    m_buffers[i].hwPic = 0;

  return true;
}

CRenderInfo CRendererMediaCodecSurface::GetRenderInfo()
{
  CRenderInfo info;
  info.formats.push_back(RENDER_FMT_MEDIACODECSURFACE);
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

int CRendererMediaCodecSurface::GetImage(YV12Image *image, int source, bool readonly)
{
  if (image == nullptr)
    return -1;

  /* take next available buffer */
  if (source == -1)
    source = (m_iRenderBuffer + 1) % m_numRenderBuffers;

  return source;
}

void CRendererMediaCodecSurface::AddVideoPictureHW(VideoPicture &picture, int index)
{
  ReleaseBuffer(index);
  BUFFER &buf = m_buffers[index];
  buf.hwPic = picture.hwPic ? static_cast<CDVDMediaCodecInfo*>(picture.hwPic)->Retain() : nullptr;
}

void CRendererMediaCodecSurface::ReleaseBuffer(int idx)
{
  BUFFER &buf = m_buffers[idx];
  if (buf.hwPic)
  {
    CDVDMediaCodecInfo *mci = static_cast<CDVDMediaCodecInfo *>(buf.hwPic);
    SAFE_RELEASE(mci);
    buf.hwPic = NULL;
  }
}

void CRendererMediaCodecSurface::FlipPage(int source)
{
  if (source >= 0 && source < m_numRenderBuffers)
    m_iRenderBuffer = source;
  else
    m_iRenderBuffer = (m_iRenderBuffer + 1) % m_numRenderBuffers;

  CDVDMediaCodecInfo *mci = static_cast<CDVDMediaCodecInfo *>(m_buffers[m_iRenderBuffer].hwPic);

  if (mci && m_renderingStarted)
    mci->ReleaseOutputBuffer(true);
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
  m_renderingStarted = false;
}

void CRendererMediaCodecSurface::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  {
    std::chrono::milliseconds elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_prevTime).count());
    if (elapsed < std::chrono::milliseconds(10))
      std::this_thread::sleep_for(std::chrono::milliseconds(10) - elapsed);

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

  CRect dstRect(m_destRect);
  CRect srcRect(m_sourceRect);
  switch (stereo_mode)
  {
    case RENDER_STEREO_MODE_SPLIT_HORIZONTAL:
      dstRect.y2 *= 2.0;
      srcRect.y2 *= 2.0;
      break;
    case RENDER_STEREO_MODE_SPLIT_VERTICAL:
      dstRect.x2 *= 2.0;
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
        double scale = (double)dstRect.Height() / dstRect.Width();
        int diff = (int) ((dstRect.Height()*scale - dstRect.Width()) / 2);
        dstRect = CRect(dstRect.x1 - diff, dstRect.y1, dstRect.x2 + diff, dstRect.y2);
    }
    default:
      break;
  }

  CRect adjRect = CXBMCApp::MapRenderToDroid(dstRect);
  CXBMCApp::get()->setVideoViewSurfaceRect(adjRect.x1, adjRect.y1, adjRect.x2, adjRect.y2);

  CLog::Log(LOGDEBUG, "CRendererMediaCodecSurface::ReorderDrawPoints: dst: %0.1f+%0.1f-%0.1fx%0.1f, adj: %0.1f+%0.1f-%0.1fx%0.1f",
    dstRect.x1, dstRect.y1, dstRect.Width(), dstRect.Height(),
    adjRect.x1, adjRect.y1, adjRect.Width(), adjRect.Height());

  if (!m_renderingStarted)
  {
    m_renderingStarted = true;
    FlipPage(m_iRenderBuffer);
  }


}

#endif
