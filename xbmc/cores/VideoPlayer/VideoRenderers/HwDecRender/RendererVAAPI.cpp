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

#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

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
    info.optimal_buffer_size = 4;
  else
    info.optimal_buffer_size = 5;
  return info;
}

bool CRendererVAAPI::Supports(ERENDERFEATURE feature)
{
  return CLinuxRendererGL::Supports(feature);
}

bool CRendererVAAPI::Supports(ESCALINGMETHOD method)
{
  return CLinuxRendererGL::Supports(method);
}

bool CRendererVAAPI::LoadShadersHook()
{
  return false;
}

bool CRendererVAAPI::RenderHook(int idx)
{
  return false;
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
  im.cshift_x = 1;
  im.cshift_y = 1;

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

  if (m_buffers[index].hwDec)
    ((VAAPI::CVaapiRenderPicture*)m_buffers[index].hwDec)->Release();
  m_buffers[index].hwDec = NULL;

  YUVFIELDS &fields = m_buffers[index].fields;
  fields[FIELD_FULL][0].id = None;
  fields[FIELD_FULL][1].id = None;
  fields[FIELD_FULL][2].id = None;
}

bool CRendererVAAPI::UploadTexture(int index)
{
  if (m_format == RENDER_FMT_VAAPINV12)
  {
    return UploadNV12Texture(index);
  }

  VAAPI::CVaapiRenderPicture *vaapi = (VAAPI::CVaapiRenderPicture*)m_buffers[index].hwDec;

  YV12Image &im = m_buffers[index].image;

  YUVFIELDS &fields = m_buffers[index].fields;

  if (!vaapi || !vaapi->valid)
  {
    return false;
  }

  YUVPLANES &planes = fields[0];

  planes[0].texwidth  = vaapi->texWidth;
  planes[0].texheight = vaapi->texHeight;

  planes[1].texwidth  = planes[0].texwidth  >> im.cshift_x;
  planes[1].texheight = planes[0].texheight >> im.cshift_y;
  planes[2].texwidth  = planes[1].texwidth;
  planes[2].texheight = planes[1].texheight;

  for (int p = 0; p < 3; p++)
  {
    planes[p].pixpertex_x = 1;
    planes[p].pixpertex_y = 1;
  }

  // set textures
  fields[0][0].id = vaapi->textureY;
  fields[0][1].id = vaapi->textureVU;
  fields[0][2].id = vaapi->textureVU;

  glEnable(m_textureTarget);

  for (int p=0; p<2; p++)
  {
    glBindTexture(m_textureTarget,fields[0][p].id);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(m_textureTarget, 0);
    VerifyGLState();
  }

  CalculateTextureSourceRects(index, 3);
  glDisable(m_textureTarget);
  return true;
}

void CRendererVAAPI::AfterRenderHook(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    ((VAAPI::CVaapiRenderPicture*)buf.hwDec)->Sync();
  }
}

#endif
