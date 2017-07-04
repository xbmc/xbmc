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

#include "RendererMediaCodec.h"

#if defined(TARGET_ANDROID)
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "../RenderFactory.h"

#if defined(EGL_KHR_reusable_sync) && !defined(EGL_EGLEXT_PROTOTYPES)
static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
#endif

CRendererMediaCodec::CRendererMediaCodec()
{
  CLog::Log(LOGNOTICE, "Instancing CRendererMediaCodec");
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
  YUVBUFFER &buf = m_buffers[index];
  CMediaCodecVideoBuffer *videoBuffer;
  if (picture.videoBuffer && (videoBuffer = dynamic_cast<CMediaCodecVideoBuffer*>(picture.videoBuffer)))
  {
    YUVBUFFER &buf = m_buffers[index];
    buf.videoBuffer = picture.videoBuffer;
    buf.fields[0][0].id = videoBuffer->GetTextureId();
    videoBuffer->Acquire();

    // releaseOutputBuffer must be in same thread as
    // dequeueOutputBuffer. We are in VideoPlayerVideo
    // thread here, so we are safe.
    videoBuffer->ReleaseOutputBuffer(true);
  }
  else
   buf.fields[0][0].id = 0;
}

void CRendererMediaCodec::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
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
  info.optimal_buffer_size = 3;
  return info;
}

bool CRendererMediaCodec::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using MediaCodec render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_MEDIACODEC;
  return true;
}

bool CRendererMediaCodec::RenderHook(int index)
{
  YUVPLANE &plane = m_buffers[index].fields[0][0];
  YUVPLANE &planef = m_buffers[index].fields[m_currentField][0];

  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, plane.id);

  if (m_currentField != FIELD_FULL)
  {
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA_BOB_OES);
    GLint   fieldLoc = g_Windowing.GUIShaderGetField();
    GLint   stepLoc = g_Windowing.GUIShaderGetStep();

    // Y is inverted, so invert fields
    if     (m_currentField == FIELD_TOP)
      glUniform1i(fieldLoc, 0);
    else if(m_currentField == FIELD_BOT)
      glUniform1i(fieldLoc, 1);
    glUniform1f(stepLoc, 1.0f / (float)plane.texheight);
  }
  else
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA_OES);

  GLint   contrastLoc = g_Windowing.GUIShaderGetContrast();
  glUniform1f(contrastLoc, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast * 0.02f);
  GLint   brightnessLoc = g_Windowing.GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);

  glUniformMatrix4fv(g_Windowing.GUIShaderGetCoord0Matrix(), 1, GL_FALSE, m_textureMatrix);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][4];

  GLint   posLoc = g_Windowing.GUIShaderGetPos();
  GLint   texLoc = g_Windowing.GUIShaderGetCoord0();


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
  glUniformMatrix4fv(g_Windowing.GUIShaderGetCoord0Matrix(),  1, GL_FALSE, identity);

  g_Windowing.DisableGUIShader();
  VerifyGLState();

  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
  VerifyGLState();

  return true;
}

bool CRendererMediaCodec::CreateTexture(int index)
{
  YUVBUFFER &buf(m_buffers[index]);

  buf.image.height = m_sourceHeight;
  buf.image.width  = m_sourceWidth;

  for (int f=0; f<3; ++f)
  {
    YUVPLANE  &plane  = buf.fields[f][0];

    plane.texwidth  = m_sourceWidth;
    plane.texheight = m_sourceHeight;
    plane.pixpertex_x = 1;
    plane.pixpertex_y = 1;

    if(m_renderMethod & RENDER_POT)
    {
      plane.texwidth  = NP2(plane.texwidth);
      plane.texheight = NP2(plane.texheight);
    }
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

#endif
