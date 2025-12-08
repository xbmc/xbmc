/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererVAAPIGLES.h"

#include "../RenderFactory.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/EGLFence.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <memory>

using namespace VAAPI;
using namespace KODI::UTILS::EGL;

IVaapiWinSystem* CRendererVAAPIGLES::m_pWinSystem = nullptr;

CBaseRenderer* CRendererVAAPIGLES::Create(CVideoBuffer* buffer)
{
  CVaapiRenderPicture *vb = dynamic_cast<CVaapiRenderPicture*>(buffer);
  if (vb)
    return new CRendererVAAPIGLES();

  return nullptr;
}

void CRendererVAAPIGLES::Register(IVaapiWinSystem* winSystem,
                                  VADisplay vaDpy,
                                  EGLDisplay eglDisplay,
                                  bool& general,
                                  bool& deepColor)
{
  general = deepColor = false;

  int major_version, minor_version;
  if (vaInitialize(vaDpy, &major_version, &minor_version) != VA_STATUS_SUCCESS)
  {
    vaTerminate(vaDpy);
    return;
  }

  CVaapi2Texture::TestInterop(vaDpy, eglDisplay, general, deepColor);
  CLog::Log(LOGDEBUG, "Vaapi2 EGL interop test results: general {}, deepColor {}",
            general ? "yes" : "no", deepColor ? "yes" : "no");
  if (!general)
  {
    CVaapi1Texture::TestInterop(vaDpy, eglDisplay, general, deepColor);
    CLog::Log(LOGDEBUG, "Vaapi1 EGL interop test results: general {}, deepColor {}",
              general ? "yes" : "no", deepColor ? "yes" : "no");
  }

  vaTerminate(vaDpy);

  if (general)
  {
    VIDEOPLAYER::CRendererFactory::RegisterRenderer("vaapi", CRendererVAAPIGLES::Create);
    m_pWinSystem = winSystem;
  }
}

CRendererVAAPIGLES::CRendererVAAPIGLES() = default;

CRendererVAAPIGLES::~CRendererVAAPIGLES()
{
  for (int i = 0; i < NUM_BUFFERS; ++i)
  {
    DeleteTexture(i);
  }
}

bool CRendererVAAPIGLES::Configure(const VideoPicture& picture, float fps, unsigned int orientation)
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
  interop.eglDisplay = m_pWinSystem->GetEGLDisplay();

  bool useVaapi2 = VAAPI::CVaapi2Texture::TestInteropGeneral(
      pic->vadsp, CRendererVAAPIGLES::m_pWinSystem->GetEGLDisplay());

  for (auto &tex : m_vaapiTextures)
  {
    if (useVaapi2)
    {
      tex = std::make_unique<VAAPI::CVaapi2Texture>();
    }
    else
    {
      tex = std::make_unique<VAAPI::CVaapi1Texture>();
    }
    tex->Init(interop);
  }

  for (auto& fence : m_fences)
  {
    fence = std::make_unique<CEGLFence>(CRendererVAAPIGLES::m_pWinSystem->GetEGLDisplay());
  }

  return CLinuxRendererGLES::Configure(picture, fps, orientation);
}

bool CRendererVAAPIGLES::ConfigChanged(const VideoPicture& picture)
{
  CVaapiRenderPicture *pic = dynamic_cast<CVaapiRenderPicture*>(picture.videoBuffer);
  if (pic->procPic.videoSurface != VA_INVALID_ID && !m_isVAAPIBuffer)
    return true;

  return false;
}

EShaderFormat CRendererVAAPIGLES::GetShaderFormat()
{
  EShaderFormat ret = SHADER_NONE;

  if (m_isVAAPIBuffer)
    ret = SHADER_NV12_RRG;
  else
    ret = SHADER_NV12;

  return ret;
}

bool CRendererVAAPIGLES::LoadShadersHook()
{
  return false;
}

bool CRendererVAAPIGLES::RenderHook(int idx)
{
  return false;
}

bool CRendererVAAPIGLES::CreateTexture(int index)
{
  if (!m_isVAAPIBuffer)
  {
    return CreateNV12Texture(index);
  }

  CPictureBuffer &buf = m_buffers[index];
  YuvImage &im = buf.image;
  CYuvPlane (&planes)[YuvImage::MAX_PLANES] = buf.fields[0];

  DeleteTexture(index);

  im = {};
  std::fill(std::begin(planes), std::end(planes), CYuvPlane{});
  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;
  im.cshift_x = 1;
  im.cshift_y = 1;

  planes[0].id = 1;

  return true;
}

void CRendererVAAPIGLES::DeleteTexture(int index)
{
  ReleaseBuffer(index);

  if (!m_isVAAPIBuffer)
  {
    DeleteNV12Texture(index);
    return;
  }

  CPictureBuffer &buf = m_buffers[index];
  buf.fields[FIELD_FULL][0].id = 0;
  buf.fields[FIELD_FULL][1].id = 0;
  buf.fields[FIELD_FULL][2].id = 0;
}

bool CRendererVAAPIGLES::UploadTexture(int index)
{
  if (!m_isVAAPIBuffer)
  {
    return UploadNV12Texture(index);
  }

  CPictureBuffer &buf = m_buffers[index];

  CVaapiRenderPicture *pic = dynamic_cast<CVaapiRenderPicture*>(buf.videoBuffer);

  if (!pic || !pic->valid)
  {
    return false;
  }

  m_vaapiTextures[index]->Map(pic);

  YuvImage &im = buf.image;
  CYuvPlane (&planes)[3] = buf.fields[0];

  auto size = m_vaapiTextures[index]->GetTextureSize();
  planes[0].texwidth  = size.Width();
  planes[0].texheight = size.Height();

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
  planes[0].id = m_vaapiTextures[index]->GetTextureY();
  planes[1].id = m_vaapiTextures[index]->GetTextureVU();
  planes[2].id = m_vaapiTextures[index]->GetTextureVU();

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
  return true;
}

void CRendererVAAPIGLES::AfterRenderHook(int index)
{
  m_fences[index]->CreateFence();
}

bool CRendererVAAPIGLES::NeedBuffer(int index)
{
  return !m_fences[index]->IsSignaled();
}

void CRendererVAAPIGLES::ReleaseBuffer(int index)
{
  m_fences[index]->DestroyFence();

  if (m_isVAAPIBuffer)
  {
    m_vaapiTextures[index]->Unmap();
  }

  CLinuxRendererGLES::ReleaseBuffer(index);
}
