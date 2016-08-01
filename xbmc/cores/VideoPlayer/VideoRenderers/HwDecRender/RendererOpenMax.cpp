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

#include "RendererOpenMax.h"

#if defined(HAVE_LIBOPENMAX)
#include "cores/IPlayer.h"
#include "../VideoPlayer/DVDCodecs/Video/OpenMaxVideo.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"

CRendererOMX::CRendererOMX()
{

}

CRendererOMX::~CRendererOMX()
{

}

bool CRendererOMX::SkipUploadYV12(int index)
{
  // skip if its already set
  return m_buffers[index].hwDec != 0 ? true : false;
}

void CRendererOMX::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  if (buf.hwDec) {
    ((OpenMaxVideoBufferHolder *)buf.hwDec)->Release();
  }
  buf.hwDec = picture.openMaxBufferHolder;
  if (buf.hwDec) {
    ((OpenMaxVideoBufferHolder *)buf.hwDec)->Acquire();
  }
  
  buf = m_buffers[index];

  OpenMaxVideoBufferHolder *buffer = static_cast<OpenMaxVideoBufferHolder *>(buf.hwDec);
  SAFE_RELEASE(buffer);
}

bool CRendererOMX::RenderUpdateCheckForEmptyField()
{
  return false;
}

bool CRendererOMX::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool CRendererOMX::Supports(ESCALINGMETHOD mode)
{
  return false;
}

EINTERLACEMETHOD CRendererOMX::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

CRenderInfo CRendererOMX::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  info.optimal_buffer_size = 2;
  return info;
}

bool CRendererOMX::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using OMXEGL RGBA render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_OMXEGL;
  return false;
}

bool CRendererOMX::RenderHook(int index)
{
  OpenMaxVideoBufferHolder *bufferHolder = static_cast<OpenMaxVideoBufferHolder *>(m_buffers[index].hwDec);
  if (!bufferHolder)
    return false;
  OpenMaxVideoBuffer *buffer = bufferHolder->m_openMaxVideoBuffer;

  GLuint textureId = buffer->texture_id;

  glDisable(GL_DEPTH_TEST);

  // Y
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, textureId);

  g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA);

  GLint   contrastLoc = g_Windowing.GUIShaderGetContrast();
  glUniform1f(contrastLoc, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast * 0.02f);
  GLint   brightnessLoc = g_Windowing.GUIShaderGetBrightness();
  glUniform1f(brightnessLoc, CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness * 0.01f - 0.5f);

  GLubyte idx[4] = {0, 1, 3, 2};        //determines order of triangle strip
  GLfloat ver[4][4];
  GLfloat tex[4][2];
  GLfloat col[3] = {1.0f, 1.0f, 1.0f};

  GLint   posLoc = g_Windowing.GUIShaderGetPos();
  GLint   texLoc = g_Windowing.GUIShaderGetCoord0();
  GLint   colLoc = g_Windowing.GUIShaderGetCol();

  glVertexAttribPointer(posLoc, 4, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, 0, 0, tex);
  glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, col);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(texLoc);
  glEnableVertexAttribArray(colLoc);

  // Set vertex coordinates
  for(int i = 0; i < 4; i++)
  {
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;// set z to 0
    ver[i][3] = 1.0f;
  }

  // Set texture coordinates
  tex[0][0] = tex[3][0] = 0;
  tex[0][1] = tex[1][1] = 1;
  tex[1][0] = tex[2][0] = 1;
  tex[2][1] = tex[3][1] = 0;

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDisableVertexAttribArray(colLoc);

  g_Windowing.DisableGUIShader();

  VerifyGLState();

  glDisable(m_textureTarget);
  VerifyGLState();

#if defined(EGL_KHR_reusable_sync)
  if (buffer->eglSync) {
    eglDestroySyncKHR(buffer->eglDisplay, buffer->eglSync);
  }
  buffer->eglSync = eglCreateSyncKHR(buffer->eglDisplay, EGL_SYNC_FENCE_KHR, NULL);
  glFlush(); // flush to ensure the sync later will succeed
#endif
  return true;
}

bool CRendererOMX::CreateTexture(int index)
{
  m_buffers[index].hwDec = NULL;
  return true;
}

void CRendererOMX::DeleteTexture(int index)
{
  if (m_buffers[index].hwDec) {
    ((OpenMaxVideoBufferHolder *)m_buffers[index].hwDec)->Release();
    m_buffers[index].hwDec = NULL;
  }
}

bool CRendererOMX::UploadTexture(int index)
{
  return true;// nothing todo for OMX
}

#endif
