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

#if defined(HAS_LIBAMCODEC)
#include "cores/IPlayer.h"
#include "windowing/egl/EGLWrapper.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecAmlogic.h"
#include "cores/VideoPlayer/DVDCodecs/Video/AMLCodec.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "utils/SysfsUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "cores/VideoPlayer/VideoRenderers/RenderCapture.h"

CRendererAML::CRendererAML()
{
  m_prevPts = -1;
}

CRendererAML::~CRendererAML()
{
}

bool CRendererAML::RenderCapture(CRenderCapture* capture)
{
  capture->BeginRender();
  capture->EndRender();
  return true;
}

void CRendererAML::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  if (picture.amlcodec)
    buf.hwDec = picture.amlcodec->Retain();
}

void CRendererAML::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    CDVDAmlogicInfo *amli = static_cast<CDVDAmlogicInfo *>(buf.hwDec);
    SAFE_RELEASE(amli);
    buf.hwDec = NULL;
  }
}

int CRendererAML::GetImageHook(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererAML::IsGuiLayer()
{
  return false;
}

bool CRendererAML::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool CRendererAML::Supports(ESCALINGMETHOD method)
{
  return false;
}

bool CRendererAML::Supports(ERENDERFEATURE feature)
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

EINTERLACEMETHOD CRendererAML::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

bool CRendererAML::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using AML render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_BYPASS;
  return false;
}

bool CRendererAML::RenderHook(int index)
{
  return true;// nothing to be done for aml
}

bool CRendererAML::RenderUpdateVideoHook(bool clear, DWORD flags, DWORD alpha)
{
  ManageRenderArea();

  CDVDAmlogicInfo *amli = static_cast<CDVDAmlogicInfo *>(m_buffers[m_iYV12RenderBuffer].hwDec);
  if (amli && amli->GetOmxPts() != m_prevPts)
  {
    m_prevPts = amli->GetOmxPts();
    SysfsUtils::SetInt("/sys/module/amvideo/parameters/omx_pts", amli->GetOmxPts());

    CAMLCodec *amlcodec = amli->getAmlCodec();
    if (amlcodec)
      amlcodec->SetVideoRect(m_sourceRect, m_destRect);
  }

  usleep(10000);

  return true;
}

#endif
