/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererMediaCodec.h"

#include "../RenderFactory.h"
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "ServiceBroker.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "settings/MediaSettings.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#if defined(EGL_KHR_reusable_sync) && !defined(EGL_EGLEXT_PROTOTYPES)
static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
#endif

CRendererMediaCodec::CRendererMediaCodec()
{
  CLog::Log(LOGINFO, "Instancing CRendererMediaCodec");
#if defined(EGL_KHR_reusable_sync) && !defined(EGL_EGLEXT_PROTOTYPES)
  if (!eglCreateSyncKHR) {
    eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC) eglGetProcAddress("eglCreateSyncKHR");
  }
  if (!eglDestroySyncKHR) {
    eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC) eglGetProcAddress("eglDestroySyncKHR");
  }
  if (!eglClientWaitSyncKHR) {
    eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC) eglGetProcAddress("eglClientWaitSyncKHR");
  }
#endif
}

CRendererMediaCodec::~CRendererMediaCodec()
{
  for (int i(0); i < NUM_BUFFERS; ++i)
    ReleaseBuffer(i);
}

CBaseRenderer* CRendererMediaCodec::Create(CVideoBuffer *buffer)
{
  if (buffer && dynamic_cast<CMediaCodecVideoBuffer*>(buffer) && dynamic_cast<CMediaCodecVideoBuffer*>(buffer)->HasSurfaceTexture())
    return new CRendererMediaCodec();
  return nullptr;
}

bool CRendererMediaCodec::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("mediacodec_egl", CRendererMediaCodec::Create);
  return true;
}

void CRendererMediaCodec::AddVideoPicture(const VideoPicture &picture, int index)
{
  CPictureBuffer &buf = m_buffers[index];
  CMediaCodecVideoBuffer *videoBuffer;
  if (picture.videoBuffer && (videoBuffer = dynamic_cast<CMediaCodecVideoBuffer*>(picture.videoBuffer)))
  {
    CPictureBuffer &buf = m_buffers[index];
    buf.videoBuffer = picture.videoBuffer;
    buf.fields[0][0].id = videoBuffer->GetTextureId();
    videoBuffer->Acquire();

    // releaseOutputBuffer must be in same thread as
    // dequeueOutputBuffer. We are in VideoPlayerVideo
    // thread here, so we are safe.
    videoBuffer->ReleaseOutputBuffer(true, 0);
  }
  else
   buf.fields[0][0].id = 0;
}

void CRendererMediaCodec::ReleaseBuffer(int idx)
{
  CPictureBuffer &buf = m_buffers[idx];
  CMediaCodecVideoBuffer* videoBuffer;
  if (buf.videoBuffer && (videoBuffer = dynamic_cast<CMediaCodecVideoBuffer*>(buf.videoBuffer)))
  {
    // The media buffer has been queued to the SurfaceView but we didn't render it
    // We have to do to the updateTexImage or it will get stuck
    videoBuffer->UpdateTexImage();
    videoBuffer->GetTransformMatrix(m_textureMatrix);
    videoBuffer->Release();
    buf.videoBuffer = NULL;
  }
}

CRenderInfo CRendererMediaCodec::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = 4;
  return info;
}

bool CRendererMediaCodec::LoadShadersHook()
{
  CLog::Log(LOGINFO, "GL: Using MediaCodec render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_CUSTOM;
  return true;
}

bool CRendererMediaCodec::RenderHook(int index)
{
  CYuvPlane &plane = m_buffers[index].fields[0][0];
  CYuvPlane &planef = m_buffers[index].fields[m_currentField][0];

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, plane.id);

  CRenderSystemGLES* renderSystem = dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());

  if (m_currentField != FIELD_FULL)
  {
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE_RGBA_BOB_OES);
    GLint   fieldLoc = renderSystem->GUIShaderGetField();
    GLint   stepLoc = renderSystem->GUIShaderGetStep();

    // Y is inverted, so invert fields
    if     (m_currentField == FIELD_TOP)
      glUniform1i(fieldLoc, 0);
    else if(m_currentField == FIELD_BOT)
      glUniform1i(fieldLoc, 1);
    glUniform1f(stepLoc, 1.0f / (float)plane.texheight);
  }
  else
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE_RGBA_OES);

  GLint   contrastLoc = renderSystem->GUIShaderGetContrast();
  glUniform1f(contrastLoc, m_videoSettings.m_Contrast * 0.02f);
  GLint   brightnessLoc = renderSystem->GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, m_videoSettings.m_Brightness * 0.01f - 0.5f);
  GLint depthLoc = renderSystem->GUIShaderGetDepth();
  glUniform1f(depthLoc, -1.0f);

  glUniformMatrix4fv(renderSystem->GUIShaderGetCoord0Matrix(), 1, GL_FALSE, m_textureMatrix);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][4];

  GLint   posLoc = renderSystem->GUIShaderGetPos();
  GLint   texLoc = renderSystem->GUIShaderGetCoord0();


  glVertexAttribPointer(posLoc, 4, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(texLoc, 4, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);

  // Set vertex coordinates
  for(int i = 0; i < 4; i++)
  {
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;        // set z to 0
    ver[i][3] = 1.0f;
  }

  // Set texture coordinates (MediaCodec is flipped in y)
  if (m_currentField == FIELD_FULL)
  {
    tex[0][0] = tex[3][0] = plane.rect.x1;
    tex[0][1] = tex[1][1] = plane.rect.y2;
    tex[1][0] = tex[2][0] = plane.rect.x2;
    tex[2][1] = tex[3][1] = plane.rect.y1;
  }
  else
  {
    tex[0][0] = tex[3][0] = planef.rect.x1;
    tex[0][1] = tex[1][1] = planef.rect.y2 * 2.0f;
    tex[1][0] = tex[2][0] = planef.rect.x2;
    tex[2][1] = tex[3][1] = planef.rect.y1 * 2.0f;
  }

  for(int i = 0; i < 4; i++)
  {
    tex[i][2] = 0.0f;
    tex[i][3] = 1.0f;
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);

  const float identity[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
  };
  glUniformMatrix4fv(renderSystem->GUIShaderGetCoord0Matrix(),  1, GL_FALSE, identity);

  renderSystem->DisableGUIShader();
  VerifyGLState();

  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
  VerifyGLState();

  return true;
}

bool CRendererMediaCodec::CreateTexture(int index)
{
  CPictureBuffer &buf(m_buffers[index]);

  buf.image.height = m_sourceHeight;
  buf.image.width  = m_sourceWidth;

  for (int f=0; f<3; ++f)
  {
    CYuvPlane  &plane  = buf.fields[f][0];

    plane.texwidth  = m_sourceWidth;
    plane.texheight = m_sourceHeight;
    plane.pixpertex_x = 1;
    plane.pixpertex_y = 1;
  }

  return true;
}

void CRendererMediaCodec::DeleteTexture(int index)
{
  ReleaseBuffer(index);
}

bool CRendererMediaCodec::UploadTexture(int index)
{
  ReleaseBuffer(index);
  CalculateTextureSourceRects(index, 1);
  return true;
}
