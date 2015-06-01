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

#include "RendererVAAPI.h"

#ifdef HAVE_LIBVA

#include "cores/dvdplayer/DVDCodecs/Video/VAAPI.h"
#include "cores/dvdplayer/DVDCodecs/DVDCodecUtils.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

CRendererVAAPI::CRendererVAAPI()
{

}

CRendererVAAPI::~CRendererVAAPI()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }
}

void CRendererVAAPI::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  VAAPI::CVaapiRenderPicture *vaapi = picture.vaapi;
  YUVBUFFER &buf = m_buffers[index];
  VAAPI::CVaapiRenderPicture *pic = vaapi->Acquire();
  if (buf.hwDec)
    ((VAAPI::CVaapiRenderPicture*)buf.hwDec)->Release();
  buf.hwDec = pic;

  if (m_format == RENDER_FMT_VAAPINV12)
  {
    YV12Image &im = m_buffers[index].image;
    CDVDCodecUtils::CopyNV12Picture(&im, &vaapi->DVDPic);
  }
}

void CRendererVAAPI::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
    ((VAAPI::CVaapiRenderPicture*)buf.hwDec)->Release();
  buf.hwDec = NULL;
}

CRenderInfo CRendererVAAPI::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  if (m_format == RENDER_FMT_VAAPINV12)
    info.optimal_buffer_size = 3;
  else
    info.optimal_buffer_size = 5;
  return info;
}

bool CRendererVAAPI::Supports(ERENDERFEATURE feature)
{
  if (m_format == RENDER_FMT_VAAPINV12)
    return CLinuxRendererGL::Supports(feature);

  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO     ||
      feature == RENDERFEATURE_POSTPROCESS     ||
      feature == RENDERFEATURE_ROTATION        ||
      feature == RENDERFEATURE_NONLINSTRETCH)
    return true;

  return false;
}

bool CRendererVAAPI::Supports(EINTERLACEMETHOD method)
{
  VAAPI::CVaapiRenderPicture *vaapiPic = (VAAPI::CVaapiRenderPicture*)m_buffers[m_iYV12RenderBuffer].hwDec;
  if(vaapiPic && vaapiPic->vaapi)
    return vaapiPic->vaapi->Supports(method);
  return false;
}

bool CRendererVAAPI::Supports(ESCALINGMETHOD method)
{
  if (m_format == RENDER_FMT_VAAPINV12)
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
    int minScale = CSettings::Get().GetInt("videoplayer.hqscalers");
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

bool CRendererVAAPI::LoadShadersHook()
{
  if (m_format == RENDER_FMT_VAAPINV12)
    return false;

  CLog::Log(LOGNOTICE, "GL: Using VAAPI render method");
  m_renderMethod = RENDER_VAAPI;
  return true;
}

bool CRendererVAAPI::RenderHook(int idx)
{
  if (m_format == RENDER_FMT_VAAPINV12)
    return false;

  UpdateVideoFilter();
  RenderRGB(idx, m_currentField);
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    ((VAAPI::CVaapiRenderPicture*)buf.hwDec)->Sync();
  }
  return true;
}

bool CRendererVAAPI::CreateTexture(int index)
{
  if (m_format == RENDER_FMT_VAAPINV12)
  {
    return CreateNV12Texture(index);
  }

  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE  &plane  = fields[0][0];

  DeleteTexture(index);

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

void CRendererVAAPI::DeleteTexture(int index)
{
  if (m_format == RENDER_FMT_VAAPINV12)
  {
    DeleteNV12Texture(index);
    return;
  }

  YUVPLANE &plane = m_buffers[index].fields[FIELD_FULL][0];
  if (m_buffers[index].hwDec)
    ((VAAPI::CVaapiRenderPicture*)m_buffers[index].hwDec)->Release();
  m_buffers[index].hwDec = NULL;
  plane.id = 0;
}

bool CRendererVAAPI::UploadTexture(int index)
{
  if (m_format == RENDER_FMT_VAAPINV12)
  {
    return UploadNV12Texture(index);
  }

  VAAPI::CVaapiRenderPicture *vaapi = (VAAPI::CVaapiRenderPicture*)m_buffers[index].hwDec;

  YUVFIELDS &fields = m_buffers[index].fields;
  YUVPLANE &plane = fields[FIELD_FULL][0];

  if (!vaapi || !vaapi->valid)
  {
    return false;
  }

  if (!vaapi->CopyGlx())
    return false;

  plane.id = vaapi->texture;

  // in stereoscopic mode sourceRect may only
  // be a part of the source video surface
  plane.rect = m_sourceRect;

  // clip rect
  if (vaapi->crop.x1 > plane.rect.x1)
    plane.rect.x1 = vaapi->crop.x1;
  if (vaapi->crop.x2 < plane.rect.x2)
    plane.rect.x2 = vaapi->crop.x2;
  if (vaapi->crop.y1 > plane.rect.y1)
    plane.rect.y1 = vaapi->crop.y1;
  if (vaapi->crop.y2 < plane.rect.y2)
    plane.rect.y2 = vaapi->crop.y2;

  plane.texheight = vaapi->texHeight;
  plane.texwidth  = vaapi->texWidth;

  if (m_textureTarget == GL_TEXTURE_2D)
  {
    plane.rect.y1 /= plane.texheight;
    plane.rect.y2 /= plane.texheight;
    plane.rect.x1 /= plane.texwidth;
    plane.rect.x2 /= plane.texwidth;
  }
  return true;
}


#endif
