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

#include "RendererVAAPIGL.h"
#include "../RenderFactory.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "windowing/WindowingFactory.h"

using namespace VAAPI;

CBaseRenderer* CRendererVAAPI::Create(CVideoBuffer *buffer)
{
  CVaapiRenderPicture *vb = dynamic_cast<CVaapiRenderPicture*>(buffer);
  if (vb)
    return new CRendererVAAPI();

  return nullptr;
}

void CRendererVAAPI::Register(VADisplay vaDpy, EGLDisplay eglDisplay, bool &general, bool &hevc)
{
  CVaapiTexture::TestInterop(vaDpy, eglDisplay, general, hevc);
  if (general)
    VIDEOPLAYER::CRendererFactory::RegisterRenderer("vaapi", CRendererVAAPI::Create);
}

CRendererVAAPI::CRendererVAAPI() = default;

CRendererVAAPI::~CRendererVAAPI()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }
}

bool CRendererVAAPI::Configure(const VideoPicture &picture, float fps, unsigned flags, unsigned int orientation)
{
  CVaapiRenderPicture *pic = dynamic_cast<CVaapiRenderPicture*>(picture.videoBuffer);
  if (pic->procPic.videoSurface != VA_INVALID_ID)
    m_isVAAPIBuffer = true;
  else
    m_isVAAPIBuffer = false;

  InteropInfo interop;
  interop.textureTarget = GL_TEXTURE_2D;
  interop.eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  interop.eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
  interop.glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
  interop.eglDisplay = g_Windowing.GetEGLDisplay();

  for (auto &tex : m_vaapiTextures)
  {
    tex.Init(interop);
  }
  for (auto &fence : m_fences)
  {
    fence = GL_NONE;
  }

  return CLinuxRendererGL::Configure(picture, fps, flags, orientation);
}

bool CRendererVAAPI::ConfigChanged(const VideoPicture &picture)
{
  CVaapiRenderPicture *pic = dynamic_cast<CVaapiRenderPicture*>(picture.videoBuffer);
  if (pic->procPic.videoSurface != VA_INVALID_ID && !m_isVAAPIBuffer)
    return true;

  return false;
}

bool CRendererVAAPI::Supports(ERENDERFEATURE feature)
{
  return CLinuxRendererGL::Supports(feature);
}

bool CRendererVAAPI::Supports(ESCALINGMETHOD method)
{
  return CLinuxRendererGL::Supports(method);
}

EShaderFormat CRendererVAAPI::GetShaderFormat()
{
  EShaderFormat ret = SHADER_NONE;

  if (m_isVAAPIBuffer)
    ret = SHADER_NV12_RRG;
  else
    ret = SHADER_NV12;

  return ret;
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
  if (!m_isVAAPIBuffer)
  {
    return CreateNV12Texture(index);
  }

  YUVBUFFER &buf = m_buffers[index];
  YuvImage &im = buf.image;
  YUVPLANE (&planes)[YuvImage::MAX_PLANES] = buf.fields[0];

  DeleteTexture(index);

  memset(&im, 0, sizeof(im));
  memset(&planes, 0, sizeof(YUVPLANE[YuvImage::MAX_PLANES]));
  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  planes[0].id = 1;

  return true;
}

void CRendererVAAPI::DeleteTexture(int index)
{
  ReleaseBuffer(index);

  if (!m_isVAAPIBuffer)
  {
    DeleteNV12Texture(index);
    return;
  }

  YUVBUFFER &buf = m_buffers[index];
  buf.fields[FIELD_FULL][0].id = 0;
  buf.fields[FIELD_FULL][1].id = 0;
  buf.fields[FIELD_FULL][2].id = 0;
}

bool CRendererVAAPI::UploadTexture(int index)
{
  if (!m_isVAAPIBuffer)
  {
    return UploadNV12Texture(index);
  }

  YUVBUFFER &buf = m_buffers[index];

  CVaapiRenderPicture *pic = dynamic_cast<CVaapiRenderPicture*>(buf.videoBuffer);

  if (!pic || !pic->valid)
  {
    return false;
  }

  m_vaapiTextures[index].Map(pic);

  YuvImage &im = buf.image;
  YUVPLANE (&planes)[3] = buf.fields[0];

  planes[0].texwidth  = m_vaapiTextures[index].m_texWidth;
  planes[0].texheight = m_vaapiTextures[index].m_texHeight;

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
  planes[0].id = m_vaapiTextures[index].m_textureY;
  planes[1].id = m_vaapiTextures[index].m_textureVU;
  planes[2].id = m_vaapiTextures[index].m_textureVU;

  glEnable(m_textureTarget);

  for (int p=0; p<2; p++)
  {
    glBindTexture(m_textureTarget, planes[p].id);
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
  if (glIsSync(m_fences[idx]))
  {
    glDeleteSync(m_fences[idx]);
    m_fences[idx] = None;
  }
  m_fences[idx] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

bool CRendererVAAPI::NeedBuffer(int idx)
{
  if (glIsSync(m_fences[idx]))
  {
    GLint state;
    GLsizei length;
    glGetSynciv(m_fences[idx], GL_SYNC_STATUS, 1, &length, &state);
    if (state == GL_SIGNALED)
    {
      glDeleteSync(m_fences[idx]);
      m_fences[idx] = GL_NONE;
    }
    else
    {
      return true;
    }
  }

  return false;
}

void CRendererVAAPI::ReleaseBuffer(int idx)
{
  if (glIsSync(m_fences[idx]))
  {
    glDeleteSync(m_fences[idx]);
    m_fences[idx] = GL_NONE;
  }
  m_vaapiTextures[idx].Unmap();
  CLinuxRendererGL::ReleaseBuffer(idx);
}
