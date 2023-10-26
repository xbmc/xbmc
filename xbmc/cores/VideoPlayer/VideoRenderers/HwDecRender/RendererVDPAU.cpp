/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererVDPAU.h"

#include "../RenderFactory.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VDPAU.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

using namespace VDPAU;

CBaseRenderer* CRendererVDPAU::Create(CVideoBuffer *buffer)
{
  CVdpauRenderPicture *vb = dynamic_cast<CVdpauRenderPicture*>(buffer);
  if (vb)
    return new CRendererVDPAU();

  return nullptr;
}

bool CRendererVDPAU::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("vdpau", CRendererVDPAU::Create);
  return true;
}

CRendererVDPAU::CRendererVDPAU() = default;

CRendererVDPAU::~CRendererVDPAU()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }
  m_interopState.Finish();
}

bool CRendererVDPAU::Configure(const VideoPicture &picture, float fps, unsigned int orientation)
{
  CVdpauRenderPicture *pic = dynamic_cast<CVdpauRenderPicture*>(picture.videoBuffer);
  if (pic->procPic.isYuv)
    m_isYuv = true;
  else
    m_isYuv = false;

  if (!m_interopState.Init(pic->device, pic->procFunc, pic->ident))
    return false;

  for (auto &tex : m_vdpauTextures)
  {
    tex.Init(m_interopState.GetInterop());
  }
  for (auto &fence : m_fences)
  {
    fence = {};
  }

  return CLinuxRendererGL::Configure(picture, fps, orientation);
}

bool CRendererVDPAU::ConfigChanged(const VideoPicture &picture)
{
  CVdpauRenderPicture *pic = dynamic_cast<CVdpauRenderPicture*>(picture.videoBuffer);
  if (pic->procPic.isYuv && !m_isYuv)
    return true;

  if (m_interopState.NeedInit(pic->device, pic->procFunc, pic->ident))
    return true;

  return false;
}

bool CRendererVDPAU::NeedBuffer(int idx)
{
  if (glIsSync(m_fences[idx]))
  {
    GLint state;
    GLsizei length;
    glGetSynciv(m_fences[idx], GL_SYNC_STATUS, 1, &length, &state);
    if (state == GL_SIGNALED)
    {
      glDeleteSync(m_fences[idx]);
      m_fences[idx] = {};
    }
    else
    {
      return true;
    }
  }

  return false;
}

bool CRendererVDPAU::Flush(bool saveBuffers)
{
  for (int i = 0; i < NUM_BUFFERS; i++)
      m_vdpauTextures[i].Unmap();

  return CLinuxRendererGL::Flush(saveBuffers);
}


void CRendererVDPAU::ReleaseBuffer(int idx)
{
  if (glIsSync(m_fences[idx]))
  {
    glDeleteSync(m_fences[idx]);
    m_fences[idx] = {};
  }
  m_vdpauTextures[idx].Unmap();
  CLinuxRendererGL::ReleaseBuffer(idx);
}

bool CRendererVDPAU::Supports(ERENDERFEATURE feature) const
{
  if(feature == RENDERFEATURE_BRIGHTNESS ||
     feature == RENDERFEATURE_CONTRAST)
  {
    if (!m_isYuv && !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOSCREEN_LIMITEDRANGE))
      return true;

    return (m_renderMethod & RENDER_GLSL);
  }
  else if (feature == RENDERFEATURE_NOISE ||
           feature == RENDERFEATURE_SHARPNESS)
  {
    if (!m_isYuv)
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

bool CRendererVDPAU::Supports(ESCALINGMETHOD method) const
{
  if (m_isYuv)
    return CLinuxRendererGL::Supports(method);

  if (method == VS_SCALINGMETHOD_NEAREST)
    return true;

  if (method == VS_SCALINGMETHOD_LINEAR ||
      method == VS_SCALINGMETHOD_AUTO)
    return true;

  if(method == VS_SCALINGMETHOD_CUBIC_B_SPLINE
  || method == VS_SCALINGMETHOD_CUBIC_MITCHELL
  || method == VS_SCALINGMETHOD_CUBIC_CATMULL
  || method == VS_SCALINGMETHOD_CUBIC_0_075
  || method == VS_SCALINGMETHOD_CUBIC_0_1
  || method == VS_SCALINGMETHOD_LANCZOS2
  || method == VS_SCALINGMETHOD_SPLINE36_FAST
  || method == VS_SCALINGMETHOD_LANCZOS3_FAST
  || method == VS_SCALINGMETHOD_SPLINE36
  || method == VS_SCALINGMETHOD_LANCZOS3)
  {
    // if scaling is below level, avoid hq scaling
    float scaleX = fabs(((float)m_sourceWidth - m_destRect.Width())/m_sourceWidth)*100;
    float scaleY = fabs(((float)m_sourceHeight - m_destRect.Height())/m_sourceHeight)*100;
    int minScale = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("videoplayer.hqscalers");
    return !(scaleX < minScale && scaleY < minScale);
  }

  return false;
}

EShaderFormat CRendererVDPAU::GetShaderFormat()
{
  EShaderFormat ret = SHADER_NONE;

  if (m_isYuv)
    ret = SHADER_NV12;

  return ret;
}

bool CRendererVDPAU::LoadShadersHook()
{
  if (!m_isYuv)
  {
    CLog::Log(LOGINFO, "GL: Using VDPAU render method");
    m_renderMethod = RENDER_CUSTOM;
    m_fullRange = false;
    return true;
  }
  return false;
}

bool CRendererVDPAU::RenderHook(int idx)
{
  UpdateVideoFilter();

  if (m_isYuv)
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

  return true;
}

void CRendererVDPAU::AfterRenderHook(int idx)
{
  if (glIsSync(m_fences[idx]))
  {
    glDeleteSync(m_fences[idx]);
    m_fences[idx] = None;
  }
  m_fences[idx] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

bool CRendererVDPAU::CreateTexture(int index)
{
  if (!m_isYuv)
    return CreateVDPAUTexture(index);
  else
    return CreateVDPAUTexture420(index);
}

void CRendererVDPAU::DeleteTexture(int index)
{
  ReleaseBuffer(index);

  if (!m_isYuv)
    DeleteVDPAUTexture(index);
  else
    DeleteVDPAUTexture420(index);
}

bool CRendererVDPAU::UploadTexture(int index)
{
  if (!m_isYuv)
    return UploadVDPAUTexture(index);
  else
    return UploadVDPAUTexture420(index);
}

bool CRendererVDPAU::CreateVDPAUTexture(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = buf.image;
  CYuvPlane &plane = buf.fields[FIELD_FULL][0];

  DeleteVDPAUTexture(index);

  memset(&im, 0, sizeof(im));
  plane = {};
  im.height = m_sourceHeight;
  im.width = m_sourceWidth;

  plane.texwidth = im.width;
  plane.texheight = im.height;

  plane.pixpertex_x = 1;
  plane.pixpertex_y = 1;

  plane.id = 1;
  return true;
}

void CRendererVDPAU::DeleteVDPAUTexture(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  CYuvPlane &plane = buf.fields[FIELD_FULL][0];

  plane.id = 0;
}

bool CRendererVDPAU::UploadVDPAUTexture(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  VDPAU::CVdpauRenderPicture *pic = dynamic_cast<VDPAU::CVdpauRenderPicture*>(buf.videoBuffer);

  CYuvPlane &plane = buf.fields[FIELD_FULL][0];

  if (!pic)
  {
    return false;
  }

  if (!m_vdpauTextures[index].Map(pic))
    return false;

  // in stereoscopic mode sourceRect may only
  // be a part of the source video surface
  plane.rect = m_sourceRect;

  // clip rect
  if (pic->crop.x1 > plane.rect.x1)
    plane.rect.x1 = pic->crop.x1;
  if (pic->crop.x2 < plane.rect.x2)
    plane.rect.x2 = pic->crop.x2;
  if (pic->crop.y1 > plane.rect.y1)
    plane.rect.y1 = pic->crop.y1;
  if (pic->crop.y2 < plane.rect.y2)
    plane.rect.y2 = pic->crop.y2;

  plane.texheight = m_vdpauTextures[index].m_texHeight;
  plane.texwidth  = m_vdpauTextures[index].m_texWidth;

  if (m_textureTarget == GL_TEXTURE_2D)
  {
    plane.rect.y1 /= plane.texheight;
    plane.rect.y2 /= plane.texheight;
    plane.rect.x1 /= plane.texwidth;
    plane.rect.x2 /= plane.texwidth;
  }

  // set texture
  plane.id = m_vdpauTextures[index].m_texture;

  return true;
}

bool CRendererVDPAU::CreateVDPAUTexture420(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = buf.image;
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = buf.fields[0];
  GLuint *pbo = buf.pbo;

  DeleteVDPAUTexture420(index);

  memset(&im, 0, sizeof(im));
  memset(&planes, 0, sizeof(CYuvPlane[YuvImage::MAX_PLANES]));

  im.cshift_x = 1;
  im.cshift_y = 1;

  im.plane[0] = nullptr;
  im.plane[1] = nullptr;
  im.plane[2] = nullptr;

  for(int p=0; p<3; p++)
  {
    pbo[p] = None;
  }

  planes[0].id = 1;

  return true;
}

void CRendererVDPAU::DeleteVDPAUTexture420(int index)
{
  CPictureBuffer &buf = m_buffers[index];

  buf.fields[0][0].id = 0;
  buf.fields[1][0].id = 0;
  buf.fields[1][1].id = 0;
  buf.fields[2][0].id = 0;
  buf.fields[2][1].id = 0;
}

bool CRendererVDPAU::UploadVDPAUTexture420(int index)
{
  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = buf.image;

  VDPAU::CVdpauRenderPicture *pic = dynamic_cast<VDPAU::CVdpauRenderPicture*>(buf.videoBuffer);

  if (!pic)
  {
    return false;
  }

  if (!m_vdpauTextures[index].Map(pic))
    return false;

  im.height = m_vdpauTextures[index].m_texHeight;
  im.width = m_vdpauTextures[index].m_texWidth;

  // YUV
  for (int f = FIELD_TOP; f<=FIELD_BOT ; f++)
  {
    CYuvPlane (&planes)[YuvImage::MAX_PLANES] = buf.fields[f];

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
  buf.fields[1][0].id = m_vdpauTextures[index].m_textureTopY;
  buf.fields[1][1].id = m_vdpauTextures[index].m_textureTopUV;
  buf.fields[1][2].id = m_vdpauTextures[index].m_textureTopUV;
  buf.fields[2][0].id = m_vdpauTextures[index].m_textureBotY;
  buf.fields[2][1].id = m_vdpauTextures[index].m_textureBotUV;
  buf.fields[2][2].id = m_vdpauTextures[index].m_textureBotUV;

  for (int f = FIELD_TOP; f <= FIELD_BOT; f++)
  {
    for (int p=0; p<2; p++)
    {
      glBindTexture(m_textureTarget, buf.fields[f][p].id);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      glBindTexture(m_textureTarget,0);
      VerifyGLState();
    }
  }
  CalculateTextureSourceRects(index, 3);
  return true;
}
