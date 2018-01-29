/*
 *      Copyright (C) 2007-2015 Team Kodi
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

#include "RendererAML.h"

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAmlogic.h"
#include "cores/VideoPlayer/DVDCodecs/Video/AMLCodec.h"
#include "utils/log.h"
#include "utils/SysfsUtils.h"
#include "utils/ScreenshotAML.h"
#include "settings/MediaSettings.h"
#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "settings/AdvancedSettings.h"

CRendererAML::CRendererAML()
 : m_prevVPts(-1)
 , m_bConfigured(false)
{
  CLog::Log(LOGINFO, "Constructing CRendererAML");
}

CRendererAML::~CRendererAML()
{
  Reset();
}

CBaseRenderer* CRendererAML::Create(CVideoBuffer *buffer)
{
  if (buffer && dynamic_cast<CAMLVideoBuffer*>(buffer))
    return new CRendererAML();
  return nullptr;
}

bool CRendererAML::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("amlogic", CRendererAML::Create);
  return true;
}

bool CRendererAML::Configure(const VideoPicture &picture, float fps, unsigned int orientation)
{
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

  m_bConfigured = true;

  return true;
}

CRenderInfo CRendererAML::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = m_numRenderBuffers;
  info.optimal_buffer_size = m_numRenderBuffers;
  info.opaque_pointer = (void *)this;
  return info;
}

bool CRendererAML::RenderCapture(CRenderCapture* capture)
{
  capture->BeginRender();
  capture->EndRender();
  CScreenshotAML::CaptureVideoFrame((unsigned char *)capture->GetRenderBuffer(), capture->GetWidth(), capture->GetHeight());
  return true;
}

void CRendererAML::AddVideoPicture(const VideoPicture &picture, int index, double currentClock)
{
  ReleaseBuffer(index);

  BUFFER &buf(m_buffers[index]);
  if (picture.videoBuffer)
  {
    buf.videoBuffer = picture.videoBuffer;
    buf.videoBuffer->Acquire();
  }
}

void CRendererAML::ReleaseBuffer(int idx)
{
  BUFFER &buf(m_buffers[idx]);
  if (buf.videoBuffer)
  {
    CAMLVideoBuffer *amli(dynamic_cast<CAMLVideoBuffer*>(buf.videoBuffer));
    if (amli)
    {
      if (amli->m_amlCodec)
      {
        amli->m_amlCodec->ReleaseFrame(amli->m_bufferIndex, true);
        amli->m_amlCodec = nullptr; // Released
      }
      amli->Release();
    }
    buf.videoBuffer = nullptr;
  }
}

bool CRendererAML::Supports(ERENDERFEATURE feature)
{
  if (feature == RENDERFEATURE_ZOOM ||
      feature == RENDERFEATURE_CONTRAST ||
      feature == RENDERFEATURE_BRIGHTNESS ||
      feature == RENDERFEATURE_NONLINSTRETCH ||
      feature == RENDERFEATURE_STRETCH ||
      feature == RENDERFEATURE_PIXEL_RATIO ||
      feature == RENDERFEATURE_ROTATION)
    return true;

  return false;
}

void CRendererAML::Reset()
{
  m_prevVPts = -1;
  for (int i = 0 ; i < m_numRenderBuffers ; ++i)
  {
    if (m_buffers[i].videoBuffer)
    {
      m_buffers[i].videoBuffer->Release();
      m_buffers[i].videoBuffer = nullptr;
    }
  }
}

void CRendererAML::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  ManageRenderArea();

  CAMLVideoBuffer *amli = dynamic_cast<CAMLVideoBuffer *>(m_buffers[index].videoBuffer);
  if(amli && amli->m_amlCodec)
  {
    int pts = amli->m_omxPts;
    if (pts != m_prevVPts)
    {
      amli->m_amlCodec->ReleaseFrame(amli->m_bufferIndex);
      amli->m_amlCodec->SetVideoRect(m_sourceRect, m_destRect);
      amli->m_amlCodec = nullptr; //Mark frame as processed
      m_prevVPts = pts;
    }
  }
  CAMLCodec::PollFrame();
}
