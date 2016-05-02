/*
 *      Copyright (C) 2007-2015 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RendererVDPAU.h"

#ifdef HAVE_LIBVDPAU

#include "cores/VideoPlayer/DVDCodecs/Video/VDPAU.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "windowing/WindowingFactory.h"

CRendererVDPAU::CRendererVDPAU()
{

}

CRendererVDPAU::~CRendererVDPAU()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }
}

void CRendererVDPAU::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  VDPAU::CVdpauRenderPicture *vdpau = picture.vdpau;
  YUVBUFFER &buf = m_buffers[index];
  VDPAU::CVdpauRenderPicture *pic = vdpau->Acquire();
  if (buf.hwDec)
    ((VDPAU::CVdpauRenderPicture*)buf.hwDec)->Release();
  buf.hwDec = pic;
}

void CRendererVDPAU::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
    ((VDPAU::CVdpauRenderPicture*)buf.hwDec)->Release();
  buf.hwDec = NULL;
}

CRenderInfo CRendererVDPAU::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  info.optimal_buffer_size = 5;
  return info;
}

bool CRendererVDPAU::Supports(ERENDERFEATURE feature)
{
  if(feature == RENDERFEATURE_BRIGHTNESS ||
     feature == RENDERFEATURE_CONTRAST)
  {
    if ((m_renderMethod & RENDER_VDPAU) && !CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE))
      return true;

    return (m_renderMethod & RENDER_GLSL)
        || (m_renderMethod & RENDER_ARB);
  }
  else if (feature == RENDERFEATURE_NOISE ||
           feature == RENDERFEATURE_SHARPNESS)
  {
    if (m_format == RENDER_FMT_VDPAU)
      return true;
  }
  else if (feature == RENDERFEATURE_STRETCH         ||
           feature == RENDERFEATURE_ZOOM            ||
           feature == RENDERFEATURE_VERTICAL_SHIFT  ||
           feature == RENDERFEATURE_PIXEL_RATIO     ||
           feature == RENDERFEATURE_POSTPROCESS     ||
           feature == RENDERFEATURE_ROTATION        ||
           feature == RENDERFEATURE_NONLINSTRETCH)
    return true;

  return false;
}

bool CRendererVDPAU::Supports(EINTERLACEMETHOD method)
{
  VDPAU::CVdpauRenderPicture *VDPAUPic = (VDPAU::CVdpauRenderPicture*)m_buffers[m_iYV12RenderBuffer].hwDec;
  if(VDPAUPic && VDPAUPic->vdpau)
    return VDPAUPic->vdpau->Supports(method);
  return false;
}

bool CRendererVDPAU::Supports(ESCALINGMETHOD method)
{
  if (m_format == RENDER_FMT_VDPAU_420)
    return CLinuxRendererGL::Supports(method);

  //nearest neighbor doesn't work on YUY2 and UYVY
  if (method == VS_SCALINGMETHOD_NEAREST &&
      m_format != RENDER_FMT_YUYV422 &&
      m_format != RENDER_FMT_UYVY422)
    return true;

  if(method == VS_SCALINGMETHOD_LINEAR
  || method == VS_SCALINGMETHOD_AUTO)
    return true;

  if(method == VS_SCALINGMETHOD_CUBIC
  || method == VS_SCALINGMETHOD_LANCZOS2
  || method == VS_SCALINGMETHOD_SPLINE36_FAST
  || method == VS_SCALINGMETHOD_LANCZOS3_FAST
  || method == VS_SCALINGMETHOD_SPLINE36
  || method == VS_SCALINGMETHOD_LANCZOS3)
  {
    // if scaling is below level, avoid hq scaling
    float scaleX = fabs(((float)m_sourceWidth - m_destRect.Width())/m_sourceWidth)*100;
    float scaleY = fabs(((float)m_sourceHeight - m_destRect.Height())/m_sourceHeight)*100;
    int minScale = CSettings::GetInstance().GetInt("videoplayer.hqscalers");
    if (scaleX < minScale && scaleY < minScale)
      return false;

    // spline36 and lanczos3 are only allowed through advancedsettings.xml
    if(method != VS_SCALINGMETHOD_SPLINE36
        && method != VS_SCALINGMETHOD_LANCZOS3)
      return true;
    else
      return g_advancedSettings.m_videoEnableHighQualityHwScalers;
  }

  return false;
}

bool CRendererVDPAU::LoadShadersHook()
{
  if (m_format == RENDER_FMT_VDPAU)
  {
    CLog::Log(LOGNOTICE, "GL: Using VDPAU render method");
    m_renderMethod = RENDER_VDPAU;
    m_fullRange = false;
    return true;
  }
  return false;
}

bool CRendererVDPAU::RenderHook(int idx)
{
  UpdateVideoFilter();

  if (m_format == RENDER_FMT_VDPAU_420)
  {
    switch(m_renderQuality)
    {
    case RQ_LOW:
    case RQ_SINGLEPASS:
      if (m_currentField == FIELD_FULL)
        RenderProgressiveWeave(idx, m_currentField);
      else
        RenderSinglePass(idx, m_currentField);
      VerifyGLState();
      break;

    case RQ_MULTIPASS:
      if (m_currentField == FIELD_FULL)
        RenderProgressiveWeave(idx, m_currentField);
      else
      {
        RenderToFBO(idx, m_currentField);
        RenderFromFBO();
      }
      VerifyGLState();
      break;
    }
  }
  else
  {
    RenderRGB(idx, m_currentField);
  }

  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    ((VDPAU::CVdpauRenderPicture*)buf.hwDec)->Sync();
  }
  return true;
}

bool CRendererVDPAU::CreateTexture(int index)
{
  if (m_format == RENDER_FMT_VDPAU)
    return CreateVDPAUTexture(index);
  else if (m_format == RENDER_FMT_VDPAU_420)
    return CreateVDPAUTexture420(index);
  else
    return false;
}

void CRendererVDPAU::DeleteTexture(int index)
{
  if (m_format == RENDER_FMT_VDPAU)
    DeleteVDPAUTexture(index);
  else if (m_format == RENDER_FMT_VDPAU_420)
    DeleteVDPAUTexture420(index);
}

bool CRendererVDPAU::UploadTexture(int index)
{
  if (m_format == RENDER_FMT_VDPAU)
    return UploadVDPAUTexture(index);
  else if (m_format == RENDER_FMT_VDPAU_420)
    return UploadVDPAUTexture420(index);
  else
    return false;
}

bool CRendererVDPAU::CreateVDPAUTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[FIELD_FULL][0];

  DeleteVDPAUTexture(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));
  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  plane.texwidth  = im.width;
  plane.texheight = im.height;

  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  plane.id = 1;
  return true;
}

void CRendererVDPAU::DeleteVDPAUTexture(int index)
{
  YUVPLANE &plane = m_buffers[index].fields[FIELD_FULL][0];

  if (m_buffers[index].hwDec)
    ((VDPAU::CVdpauRenderPicture*)m_buffers[index].hwDec)->Release();
  m_buffers[index].hwDec = NULL;

  plane.id = 0;
}

bool CRendererVDPAU::UploadVDPAUTexture(int index)
{
  VDPAU::CVdpauRenderPicture *vdpau = (VDPAU::CVdpauRenderPicture*)m_buffers[index].hwDec;

  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE &plane = fields[FIELD_FULL][0];

  if (!vdpau || !vdpau->valid)
  {
    return false;
  }

  plane.id = vdpau->texture[0];

  // in stereoscopic mode sourceRect may only
  // be a part of the source video surface
  plane.rect = m_sourceRect;

  // clip rect
  if (vdpau->crop.x1 > plane.rect.x1)
    plane.rect.x1 = vdpau->crop.x1;
  if (vdpau->crop.x2 < plane.rect.x2)
    plane.rect.x2 = vdpau->crop.x2;
  if (vdpau->crop.y1 > plane.rect.y1)
    plane.rect.y1 = vdpau->crop.y1;
  if (vdpau->crop.y2 < plane.rect.y2)
    plane.rect.y2 = vdpau->crop.y2;

  plane.texheight = vdpau->texHeight;
  plane.texwidth  = vdpau->texWidth;

  if (m_textureTarget == GL_TEXTURE_2D)
  {
    plane.rect.y1 /= plane.texheight;
    plane.rect.y2 /= plane.texheight;
    plane.rect.x1 /= plane.texwidth;
    plane.rect.x2 /= plane.texwidth;
  }

  return true;
}

bool CRendererVDPAU::CreateVDPAUTexture420(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE &plane = fields[0][0];
  GLuint    *pbo    = m_buffers[index].pbo;

  DeleteVDPAUTexture420(index);

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.cshift_x = 1;
  im.cshift_y = 1;

  im.plane[0] = NULL;
  im.plane[1] = NULL;
  im.plane[2] = NULL;

  for(int p=0; p<3; p++)
  {
    pbo[p] = None;
  }

  plane.id = 1;

  return true;
}

void CRendererVDPAU::DeleteVDPAUTexture420(int index)
{
  YUVFIELDS &fields = m_buffers[index].fields;

  if (m_buffers[index].hwDec)
    ((VDPAU::CVdpauRenderPicture*)m_buffers[index].hwDec)->Release();
  m_buffers[index].hwDec = NULL;

  fields[0][0].id = 0;
  fields[1][0].id = 0;
  fields[1][1].id = 0;
  fields[2][0].id = 0;
  fields[2][1].id = 0;
}

bool CRendererVDPAU::UploadVDPAUTexture420(int index)
{
  VDPAU::CVdpauRenderPicture *vdpau = (VDPAU::CVdpauRenderPicture*)m_buffers[index].hwDec;
  YV12Image &im = m_buffers[index].image;

  YUVFIELDS &fields = m_buffers[index].fields;

  if (!vdpau || !vdpau->valid)
  {
    return false;
  }

  im.height = vdpau->texHeight;
  im.width  = vdpau->texWidth;

  // YUV
  for (int f = FIELD_TOP; f<=FIELD_BOT ; f++)
  {
    YUVPLANES &planes = fields[f];

    planes[0].texwidth  = im.width;
    planes[0].texheight = im.height >> 1;

    planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
    planes[1].texheight = planes[0].texheight >> im.cshift_y;
    planes[2].texwidth  = planes[1].texwidth;
    planes[2].texheight = planes[1].texheight;

    for (int p = 0; p < 3; p++)
    {
      planes[p].pixpertex_x = 1;
      planes[p].pixpertex_y = 1;
    }
  }
  // crop
//  m_sourceRect.x1 += vdpau->crop.x1;
//  m_sourceRect.x2 -= vdpau->crop.x2;
//  m_sourceRect.y1 += vdpau->crop.y1;
//  m_sourceRect.y2 -= vdpau->crop.y2;

  // set textures
  fields[1][0].id = vdpau->texture[0];
  fields[1][1].id = vdpau->texture[2];
  fields[1][2].id = vdpau->texture[2];
  fields[2][0].id = vdpau->texture[1];
  fields[2][1].id = vdpau->texture[3];
  fields[2][2].id = vdpau->texture[3];

  glEnable(m_textureTarget);
  for (int f = FIELD_TOP; f <= FIELD_BOT; f++)
  {
    for (int p=0; p<2; p++)
    {
      glBindTexture(m_textureTarget,fields[f][p].id);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glBindTexture(m_textureTarget,0);
      VerifyGLState();
    }
  }
  CalculateTextureSourceRects(index, 3);
  glDisable(m_textureTarget);
  return true;
}

#endif
