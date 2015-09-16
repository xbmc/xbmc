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

#include "RendererStagefright.h"

#if defined(HAS_LIBSTAGEFRIGHT)
#include "cores/IPlayer.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "windowing/egl/EGLWrapper.h"
#include "android/activity/XBMCApp.h"
#include "DVDCodecs/Video/DVDVideoCodecStageFright.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"

// EGL extension functions
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

CRendererStagefright::StagefrightContext::StagefrightContext()
{
  stf = NULL;
  eglimg = EGL_NO_IMAGE_KHR;
}

CRendererStagefright::StagefrightContext::~StagefrightContext()
{
}

CRendererStagefright::CRendererStagefright()
{
  if (!eglCreateImageKHR)
    eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglCreateImageKHR");
  if (!eglDestroyImageKHR)
    eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) CEGLWrapper::GetProcAddress("eglDestroyImageKHR");
  if (!glEGLImageTargetTexture2DOES)
    glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) CEGLWrapper::GetProcAddress("glEGLImageTargetTexture2DOES");
}

CRendererStagefright::~CRendererStagefright()
{

}

void CRendererStagefright::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif

  YUVBUFFER &buf = m_buffers[index];
  StagefrightContext *ctx = (StagefrightContext *)buf.hwDec;
  if (ctx)
  {
    if (ctx->eglimg != EGL_NO_IMAGE_KHR)
      picture.stf->ReleaseBuffer(ctx->eglimg);
    stf->LockBuffer(picture.eglimg);

    ctx->stf = picture.stf;
    ctx->eglimg = picture.eglimg;
  }

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "AddProcessor %d: img:%p: tm:%d\n", index, picture.eglimg, XbmcThreads::SystemClockMillis() - time);
#endif
}

bool CRendererStagefright::RenderUpdateCheckForEmptyField()
{
  return false;
}

bool CRendererStagefright::Supports(EINTERLACEMETHOD method)
{
  if (method == VS_INTERLACEMETHOD_RENDER_BOB || method == VS_INTERLACEMETHOD_RENDER_BOB_INVERTED)
    return true;
  else
    return false;
}

EINTERLACEMETHOD CRendererStagefright::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_RENDER_BOB_INVERTED;
}

CRenderInfo CRendererStagefright::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  info.optimal_buffer_size = 2;
  return info;
}

bool CRendererStagefright::LoadShadersHook()
{
  CLog::Log(LOGNOTICE, "GL: Using EGL Image render method");
  m_textureTarget = GL_TEXTURE_2D;
  m_renderMethod = RENDER_EGLIMG;
  return false;
}

bool CRendererStagefright::RenderHook(int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif

  YUVPLANE &plane = m_buffers[index].fields[0][0];
  YUVPLANE &planef = m_buffers[index].fields[field][0];

  glDisable(GL_DEPTH_TEST);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, plane.id);

  if (field != FIELD_FULL)
  {
    g_Windowing.EnableGUIShader(SM_TEXTURE_RGBA_BOB);
    GLint   fieldLoc = g_Windowing.GUIShaderGetField();
    GLint   stepLoc = g_Windowing.GUIShaderGetStep();

    if     (field == FIELD_TOP)
      glUniform1i(fieldLoc, 1);
    else if(field == FIELD_BOT)
      glUniform1i(fieldLoc, 0);
    glUniform1f(stepLoc, 1.0f / (float)plane.texheight);
  }
  else
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

  if (field == FIELD_FULL)
  {
    tex[0][0] = tex[3][0] = plane.rect.x1;
    tex[0][1] = tex[1][1] = plane.rect.y1;
    tex[1][0] = tex[2][0] = plane.rect.x2;
    tex[2][1] = tex[3][1] = plane.rect.y2;
  }
  else
  {
    tex[0][0] = tex[3][0] = planef.rect.x1;
    tex[0][1] = tex[1][1] = planef.rect.y1 * 2.0f;
    tex[1][0] = tex[2][0] = planef.rect.x2;
    tex[2][1] = tex[3][1] = planef.rect.y2 * 2.0f;
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDisableVertexAttribArray(colLoc);

  g_Windowing.DisableGUIShader();
  VerifyGLState();

  glBindTexture(m_textureTarget, 0);
  VerifyGLState();

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "RenderEglImage %d: tm:%d\n", index, XbmcThreads::SystemClockMillis() - time);
#endif
  return true;
}

int CRendererStagefright::GetImageHook(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererStagefright::CreateTexture(int index)
{
  YV12Image &im     = m_buffers[index].image;
  YUVFIELDS &fields = m_buffers[index].fields;

  DeleteTexture(index);

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

  YUVPLANE  &plane  = fields[0][0];
  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  VerifyGLState();

  glBindTexture(m_textureTarget, plane.id);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// This is necessary for non-power-of-two textures
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glDisable(m_textureTarget);
  return true;
}

void CRendererStagefright::DeleteTexture(int index)
{
  YUVBUFFER &buf = m_buffers[index];
  YUVPLANE &plane = buf.fields[0][0];

  if(plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id);
  plane.id = 0;

  ((StagefrightContext *)buf.hwDec)->stf = NULL;
  ((StagefrightContext *)buf.hwDec)->eglimg = EGL_NO_IMAGE_KHR;
}

bool CRendererStagefright::UploadTexture(int index)
{
#ifdef DEBUG_VERBOSE
  unsigned int time = XbmcThreads::SystemClockMillis();
#endif
  YUVBUFFER &buf = m_buffers[index];
  StagefrightContext *ctx = (StagefrightContext *)buf.hwDec;
  if(ctx && ctx->eglimg != EGL_NO_IMAGE_KHR)
  {
    YUVPLANE &plane = m_buffers[index].fields[0][0];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(m_textureTarget, plane.id);
    glEGLImageTargetTexture2DOES(m_textureTarget, (EGLImageKHR)m_buffers[index].eglimg);
    glBindTexture(m_textureTarget, 0);

    plane.flipindex = m_buffers[index].flipindex;
  }

  CalculateTextureSourceRects(index, 1);

#ifdef DEBUG_VERBOSE
  CLog::Log(LOGDEBUG, "UploadEGLIMGTexture %d: img:%p, tm:%d\n", index, ctx?ctx->eglimg:NULL, XbmcThreads::SystemClockMillis() - time);
#endif
  return true;
}

#endif