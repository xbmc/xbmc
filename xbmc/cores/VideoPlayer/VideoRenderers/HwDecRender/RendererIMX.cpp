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

#include "RendererIMX.h"

#if defined(HAS_IMXVPU)
#include "cores/IPlayer.h"
#include "windowing/egl/EGLWrapper.h"
#include "DVDCodecs/Video/DVDVideoCodecIMX.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"

#define RENDER_FLAG_FIELDS (RENDER_FLAG_FIELD0 | RENDER_FLAG_FIELD1)

CRendererIMX::CRendererIMX()
{

}

CRendererIMX::~CRendererIMX()
{
  UnInit();
}

bool CRendererIMX::RenderCapture(CRenderCapture* capture)
{
  CRect rect(0, 0, capture->GetWidth(), capture->GetHeight());

  CDVDVideoCodecIMXBuffer *buffer = static_cast<CDVDVideoCodecIMXBuffer*>(m_buffers[m_iYV12RenderBuffer].hwDec);
  capture->BeginRender();
  g_IMXContext.PushCaptureTask(buffer, &rect);
  capture->EndRender();
  return true;
}

void CRendererIMX::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];

  buf.hwDec = picture.IMXBuffer;

  if (picture.IMXBuffer)
    picture.IMXBuffer->Lock();
}

void CRendererIMX::ReleaseBuffer(int idx)
{
  CDVDVideoCodecIMXBuffer *buffer =  static_cast<CDVDVideoCodecIMXBuffer*>(m_buffers[idx].hwDec);
  SAFE_RELEASE(buffer);
  m_buffers[idx].hwDec = NULL;
}

int CRendererIMX::GetImageHook(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererIMX::IsGuiLayer()
{
  return false;
}

bool CRendererIMX::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if(method == VS_INTERLACEMETHOD_IMX_FASTMOTION
  || method == VS_INTERLACEMETHOD_IMX_FASTMOTION_DOUBLE)
    return true;
  else
    return false;
}

bool CRendererIMX::Supports(EDEINTERLACEMODE mode)
{
  if(mode == VS_DEINTERLACEMODE_AUTO
  || mode == VS_DEINTERLACEMODE_FORCE
  || mode == VS_DEINTERLACEMODE_OFF)
    return true;

  return false;
}

bool CRendererIMX::Supports(ESCALINGMETHOD method)
{
  return false;
}

EINTERLACEMETHOD CRendererIMX::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_IMX_FASTMOTION;
}

CRenderInfo CRendererIMX::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  // Let the codec control the buffer size
  info.optimal_buffer_size = info.max_buffer_size;
  return info;
}

bool CRendererIMX::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using IMXMAP render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_IMXMAP;
  return true;
}

bool CRendererIMX::RenderHook(int index)
{
  return true;// nothing to be done for imx
}

bool CRendererIMX::RenderUpdateVideoHook(bool clear, DWORD flags, DWORD alpha)
{
#if 0
  static unsigned long long previous = 0;
  unsigned long long current = XbmcThreads::SystemClockMillis();
  printf("r->r: %d\n", (int)(current-previous));
  previous = current;
#endif
  CDVDVideoCodecIMXBuffer *buffer = static_cast<CDVDVideoCodecIMXBuffer*>(m_buffers[m_iYV12RenderBuffer].hwDec);
  if (buffer != NULL && buffer->IsValid())
  {
    // this hack is needed to get the 2D mode of a 3D movie going
    RENDER_STEREO_MODE stereo_mode = g_graphicsContext.GetStereoMode();
    if (stereo_mode)
      g_graphicsContext.SetStereoView(RENDER_STEREO_VIEW_LEFT);

    ManageRenderArea();

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

    //CLog::Log(LOGDEBUG, "BLIT RECTS: source x1 %f x2 %f y1 %f y2 %f dest x1 %f x2 %f y1 %f y2 %f", srcRect.x1, srcRect.x2, srcRect.y1, srcRect.y2, dstRect.x1, dstRect.x2, dstRect.y1, dstRect.y2);
    g_IMXContext.SetBlitRects(srcRect, dstRect);

    uint8_t fieldFmt = 0;
    if (g_graphicsContext.IsFullScreenVideo())
    {
      fieldFmt |= flags & RENDER_FLAG_FIELDMASK;
      if (flags & RENDER_FLAG_FIELDS)
      {
        fieldFmt |= IPU_DEINTERLACE_RATE_EN;
        if (flags & RENDER_FLAG_FIELD1)
        {
          fieldFmt |= IPU_DEINTERLACE_RATE_FRAME1;
          // CXBMCRenderManager::PresentFields() is swapping field flag for frame1
          // this makes IPU render same picture as before, just shifted one line.
          // let's correct this
          fieldFmt ^= RENDER_FLAG_FIELDMASK;
        }
      }
    }

    g_IMXContext.BlitAsync(NULL, buffer, fieldFmt);
  }

#if 0
  unsigned long long current2 = XbmcThreads::SystemClockMillis();
  printf("r: %d  %d\n", m_iYV12RenderBuffer, (int)(current2-current));
#endif

  return true;
}

bool CRendererIMX::CreateTexture(int index)
{
  return true;
}

void CRendererIMX::DeleteTexture(int index)
{
  ReleaseBuffer(index);
}

bool CRendererIMX::UploadTexture(int index)
{
  return true;// nothing todo for IMX
}
#endif

