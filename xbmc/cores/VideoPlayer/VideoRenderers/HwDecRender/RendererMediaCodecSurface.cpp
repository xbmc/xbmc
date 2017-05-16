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
#include "settings/MediaSettings.h"
#include "platform/android/activity/XBMCApp.h"
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "utils/log.h"
#include <thread>

CRendererMediaCodecSurface::CRendererMediaCodecSurface()
  : m_bConfigured(false)
  , m_iRenderBuffer(0)
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
  ManageRenderArea();

  m_bConfigured = true;

  for (int i = 0; i < m_numRenderBuffers; ++i)
    m_buffers[i].hwPic = 0;

  return true;
}

CRenderInfo CRendererMediaCodecSurface::GetRenderInfo()
{
  CRenderInfo info;
  info.formats.push_back(RENDER_FMT_BYPASS);
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

  if (mci)
  {
    ManageRenderArea();
    mci->ReleaseOutputBuffer(true);
    m_buffers[m_iRenderBuffer].hwPic = nullptr;
  }
}

bool CRendererMediaCodecSurface::Supports(ERENDERFEATURE feature)
{
  if (feature == RENDERFEATURE_ZOOM ||
    feature == RENDERFEATURE_CONTRAST ||
    feature == RENDERFEATURE_BRIGHTNESS ||
    feature == RENDERFEATURE_STRETCH ||
    feature == RENDERFEATURE_PIXEL_RATIO ||
    feature == RENDERFEATURE_ROTATION)
    return true;

  return false;
}

void CRendererMediaCodecSurface::Reset()
{
}

void CRendererMediaCodecSurface::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  {
    std::chrono::milliseconds elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_prevTime).count());
    if (elapsed < std::chrono::milliseconds(10))
      std::this_thread::sleep_for(std::chrono::milliseconds(10) - elapsed);
    m_prevTime = std::chrono::system_clock::now();
  }
}

#endif
