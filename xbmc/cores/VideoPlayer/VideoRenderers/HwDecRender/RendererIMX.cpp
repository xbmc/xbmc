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
#include "osx/DarwinUtils.h"
#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"

CRendererIMX::CRendererIMX()
{

}

CRendererIMX::~CRendererIMX()
{

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
  CDVDVideoCodecIMXBuffer *buffer = static_cast<CDVDVideoCodecIMXBuffer*>(buf.hwDec);

  SAFE_RELEASE(buffer);
  buf.hwDec = picture.IMXBuffer;

  if (picture.IMXBuffer)
    picture.IMXBuffer->Lock();
}

void CRendererIMX::ReleaseBuffer(int idx)
{
  CDVDVideoCodecIMXBuffer *buffer =  static_cast<CDVDVideoCodecIMXBuffer*>(m_buffers[idx].hwDec);
  SAFE_RELEASE(buffer);
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
  m_renderMethod = RENDER_FMT_IMXMAP;
  return false;
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

    ManageDisplay();

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

    bool topFieldFirst = true;

    // Deinterlacing requested
    if (flags & RENDER_FLAG_FIELDMASK)
    {
      if ((buffer->GetFieldType() == VPU_FIELD_BOTTOM)
      ||  (buffer->GetFieldType() == VPU_FIELD_BT) )
        topFieldFirst = false;

      if (flags & RENDER_FLAG_FIELD0)
      {
        // Double rate first frame
        g_IMXContext.SetDeInterlacing(true);
        g_IMXContext.SetDoubleRate(true);
        g_IMXContext.SetInterpolatedFrame(true);
      }
      else if (flags & RENDER_FLAG_FIELD1)
      {
        // Double rate second frame
        g_IMXContext.SetDeInterlacing(true);
        g_IMXContext.SetDoubleRate(true);
        g_IMXContext.SetInterpolatedFrame(false);
      }
      else
      {
        // Fast motion
        g_IMXContext.SetDeInterlacing(true);
        g_IMXContext.SetDoubleRate(false);
      }
    }
    // Progressive
    else
      g_IMXContext.SetDeInterlacing(false);

    g_IMXContext.BlitAsync(NULL, buffer, topFieldFirst);
  }

#if 0
  unsigned long long current2 = XbmcThreads::SystemClockMillis();
  printf("r: %d  %d\n", m_iYV12RenderBuffer, (int)(current2-current));
#endif

  return true;
}

bool CRendererIMX::CreateTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[0][0];

  DeleteTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  plane.texwidth  = 0; // Must be actual frame width for pseudo-cropping
  plane.texheight = 0; // Must be actual frame height for pseudo-cropping
  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  VerifyGLState();

  glBindTexture(m_textureTarget, plane.id);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glDisable(m_textureTarget);
  return true;
}

void CRendererIMX::DeleteTexture(int index)
{
  YUVBUFFER &buf = m_buffers[index];
  YUVPLANE &plane = buf.fields[0][0];
  CDVDVideoCodecIMXBuffer* buffer = static_cast<CDVDVideoCodecIMXBuffer*>(buf.hwDec);

  if(plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id);
  plane.id = 0;

  SAFE_RELEASE(buffer);
}

bool CRendererIMX::UploadTexture(int index)
{
  return true;// nothing todo for IMX
}

#endif