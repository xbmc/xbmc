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
#include "cores/IPlayer.h"
#include "DVDCodecs/Video/DVDVideoCodecAndroidMediaCodec.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"

#if defined(EGL_KHR_reusable_sync) && !defined(EGL_EGLEXT_PROTOTYPES)
static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
#endif


CRendererMediaCodec::CRendererMediaCodec()
{
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

}

void CRendererMediaCodec::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
  int mindex = -1;
#endif

  YUVBUFFER &buf = m_buffers[index];
  if (picture.mediacodec)
  {
    buf.hwDec = picture.mediacodec->Retain();
#ifdef DEBUG_VERBOSE
    mindex = ((CDVDMediaCodecInfo *)buf.hwDec)->GetIndex();
#endif
    // releaseOutputBuffer must be in same thread as
    // dequeueOutputBuffer. We are in VideoPlayerVideo
    // thread here, so we are safe.
    ((CDVDMediaCodecInfo *)buf.hwDec)->ReleaseOutputBuffer(true);
  }

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "AddProcessor %d: img:%d tm:%d", index, mindex, XbmcThreads::SystemClockMillis() - time);
#endif
}

bool CRendererMediaCodec::RenderUpdateCheckForEmptyField()
{
  return false;
}

void CRendererMediaCodec::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
  {
    // The media buffer has been queued to the SurfaceView but we didn't render it
    // We have to do to the updateTexImage or it will get stuck
    CDVDMediaCodecInfo *mci = static_cast<CDVDMediaCodecInfo *>(buf.hwDec);
    mci->UpdateTexImage();
    SAFE_RELEASE(mci);
    buf.hwDec = NULL;
  }
}

bool CRendererMediaCodec::Supports(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_RENDER_BOB || method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
    return true;
  else
    return false;
}

EINTERLACEMETHOD CRendererMediaCodec::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_RENDER_BOB_INVERTED;
}

CRenderInfo CRendererMediaCodec::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = 4;
  info.optimal_buffer_size = 3;
  return info;
}

bool CRendererMediaCodec::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using MediaCodec render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_MEDIACODEC;
  return false;
}

bool CRendererMediaCodec::RenderHook(int index)
{
  #ifdef DEBUG_VERBOSE
    unsigned int time = XbmcThreads::SystemClockMillis();
  #endif

  YUVPLANE &plane = m_buffers[index].fields[0][0];
  YUVPLANE &planef = m_buffers[index].fields[index][0];

  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, plane.id);

  if (index != FIELD_FULL)
  {
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA_BOB_OES);
    GLint   fieldLoc = g_Windowing.GUIShaderGetField();
    GLint   stepLoc = g_Windowing.GUIShaderGetStep();

    // Y is inverted, so invert fields
    if     (index == FIELD_TOP)
      glUniform1i(fieldLoc, 0);
    else if(index == FIELD_BOT)
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
  if (index == FIELD_FULL)
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

  #ifdef DEBUG_VERBOSE
    CLog::Log(LOGDEBUG, "RenderMediaCodecImage %d: tm:%d", index, XbmcThreads::SystemClockMillis() - time);
  #endif
  return true;
}

int CRendererMediaCodec::GetImageHook(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererMediaCodec::CreateTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  memset(&im    , 0, sizeof(im));
  memset(&fields, 0, sizeof(fields));

  im.height = m_sourceHeight;
  im.width  = m_sourceWidth;

  for (int f=0; f<3; ++f)
  {
    YUVPLANE  &plane  = fields[f][0];

    plane.texwidth  = im.width;
    plane.texheight = im.height;
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
  CDVDMediaCodecInfo *mci = static_cast<CDVDMediaCodecInfo *>(m_buffers[index].hwDec);
  SAFE_RELEASE(mci);
  m_buffers[index].hwDec = NULL;
}

bool CRendererMediaCodec::UploadTexture(int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
  int mindex = -1;
#endif

  YUVBUFFER &buf = m_buffers[index];

  if (buf.hwDec)
  {
    CDVDMediaCodecInfo *mci = static_cast<CDVDMediaCodecInfo *>(buf.hwDec);
#ifdef DEBUG_VERBOSE
    mindex = mci->GetIndex();
#endif
    buf.fields[0][0].id = mci->GetTextureID();
    mci->UpdateTexImage();
    mci->GetTransformMatrix(m_textureMatrix);
    SAFE_RELEASE(mci);
    buf.hwDec = NULL;
  }

  CalculateTextureSourceRects(index, 1);

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "UploadSurfaceTexture %d: img: %d tm:%d", index, mindex, XbmcThreads::SystemClockMillis() - time);
#endif
  return true;
}

#endif
