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

#include "RendererVTB.h"

#if defined(TARGET_DARWIN_IOS)
#include "cores/IPlayer.h"
#include "DVDCodecs/Video/DVDVideoCodecVideoToolBox.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "settings/MediaSettings.h"
#include "windowing/WindowingFactory.h"
#include "osx/DarwinUtils.h"

CRendererVTB::CRendererVTB()
{

}

CRendererVTB::~CRendererVTB()
{

}

void CRendererVTB::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  YUVBUFFER &buf = m_buffers[index];
  if (buf.hwDec)
    CVBufferRelease((struct __CVBuffer *)buf.hwDec);
  buf.hwDec = picture.cvBufferRef;
  // retain another reference, this way VideoPlayer and renderer can issue releases.
  CVBufferRetain(picture.cvBufferRef);
}

void CRendererVTB::ReleaseBuffer(int idx)
{
  YUVBUFFER &buf = m_buffers[idx];
  if (buf.hwDec)
    CVBufferRelease((struct __CVBuffer *)buf.hwDec);
  buf.hwDec = NULL;
}

int CRendererVTB::GetImage(YV12Image *image, int source, bool readonly)
{
  return source;
}

bool CRendererVTB::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool CRendererVTB::Supports(EDEINTERLACEMODE mode)
{
  return false;
}

EINTERLACEMETHOD CRendererVTB::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

bool CRendererVTB::LoadShadersHook()
{
  float ios_version = CDarwinUtils::GetIOSVersion();
  CLog::Log(LOGNOTICE, "GL: Using CVBREF render method");
  m_textureTarget = GL_TEXTURE_2D;

  if (ios_version < 5.0 && m_format == RENDER_FMT_YUV420P)
  {
    CLog::Log(LOGNOTICE, "GL: Using software color conversion/RGBA render method");
    m_renderMethod = RENDER_SW;
  }
  else
  {
    m_renderMethod = RENDER_CVREF;
  }
  ReorderDrawPoints();// cvref needs a reorder because its flipped in y direction

  return false;
}

bool CRendererVTB::RenderHook(int index)
{
  YUVPLANE &plane = m_buffers[index].fields[m_currentField][0];
  
  glDisable(GL_DEPTH_TEST);
  
  glEnable(m_textureTarget);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(m_textureTarget, plane.id);
  
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
  
  // Set texture coordinates (corevideo is flipped in y)
  tex[0][0] = tex[3][0] = plane.rect.x1;
  tex[0][1] = tex[1][1] = plane.rect.y2;
  tex[1][0] = tex[2][0] = plane.rect.x2;
  tex[2][1] = tex[3][1] = plane.rect.y1;
  
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);
  
  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);
  glDisableVertexAttribArray(colLoc);
  
  g_Windowing.DisableGUIShader();
  VerifyGLState();
  
  glDisable(m_textureTarget);
  VerifyGLState();
  return true;
}

bool CRendererVTB::CreateTexture(int index)
{
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
  
  if(m_renderMethod & RENDER_POT)
  {
    plane.texwidth  = NP2(plane.texwidth);
    plane.texheight = NP2(plane.texheight);
  }
  glEnable(m_textureTarget);
  glGenTextures(1, &plane.id);
  VerifyGLState();
  
  glBindTexture(m_textureTarget, plane.id);
#if !TARGET_OS_IPHONE
#ifdef GL_UNPACK_ROW_LENGTH
  // Set row pixels
  glPixelStorei(GL_UNPACK_ROW_LENGTH, m_sourceWidth);
#endif
#ifdef GL_TEXTURE_STORAGE_HINT_APPLE
  // Set storage hint. Can also use GL_STORAGE_SHARED_APPLE see docs.
  glTexParameteri(m_textureTarget, GL_TEXTURE_STORAGE_HINT_APPLE , GL_STORAGE_CACHED_APPLE);
  // Set client storage
#endif
  glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
#endif
  
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // This is necessary for non-power-of-two textures
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexImage2D(m_textureTarget, 0, GL_RGBA, plane.texwidth, plane.texheight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
  
#if !TARGET_OS_IPHONE
  // turn off client storage so it doesn't get picked up for the next texture
  glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
#endif
  glBindTexture(m_textureTarget, 0);
  glDisable(m_textureTarget);

  return true;
}

void CRendererVTB::DeleteTexture(int index)
{
  YUVPLANE &plane = m_buffers[index].fields[0][0];
  
  if (m_buffers[index].hwDec)
    CVBufferRelease((struct __CVBuffer *)m_buffers[index].hwDec);
  m_buffers[index].hwDec = NULL;
  
  if(plane.id && glIsTexture(plane.id))
    glDeleteTextures(1, &plane.id);
  plane.id = 0;
}

bool CRendererVTB::UploadTexture(int index)
{
  bool ret = false;
  CVBufferRef cvBufferRef = (struct __CVBuffer *)m_buffers[index].hwDec;
  
  if (cvBufferRef)
  {
    YUVPLANE &plane = m_buffers[index].fields[0][0];
    
    CVPixelBufferLockBaseAddress(cvBufferRef, kCVPixelBufferLock_ReadOnly);
#if !TARGET_OS_IPHONE
    int rowbytes = CVPixelBufferGetBytesPerRow(cvBufferRef);
#endif
    int bufferWidth = CVPixelBufferGetWidth(cvBufferRef);
    int bufferHeight = CVPixelBufferGetHeight(cvBufferRef);
    unsigned char *bufferBase = (unsigned char *)CVPixelBufferGetBaseAddress(cvBufferRef);
    
    glEnable(m_textureTarget);
    VerifyGLState();
    
    glBindTexture(m_textureTarget, plane.id);
#if !TARGET_OS_IPHONE
#ifdef GL_UNPACK_ROW_LENGTH
    // Set row pixels
    glPixelStorei( GL_UNPACK_ROW_LENGTH, rowbytes);
#endif
#ifdef GL_TEXTURE_STORAGE_HINT_APPLE
    // Set storage hint. Can also use GL_STORAGE_SHARED_APPLE see docs.
    glTexParameteri(m_textureTarget, GL_TEXTURE_STORAGE_HINT_APPLE , GL_STORAGE_CACHED_APPLE);
#endif
#endif
    
    // Using BGRA extension to pull in video frame data directly
    glTexSubImage2D(m_textureTarget, 0, 0, 0, bufferWidth, bufferHeight, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bufferBase);
    
#if !TARGET_OS_IPHONE
#ifdef GL_UNPACK_ROW_LENGTH
    // Unset row pixels
    glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
#endif
#endif
    glBindTexture(m_textureTarget, 0);
    
    glDisable(m_textureTarget);
    VerifyGLState();
    
    CVPixelBufferUnlockBaseAddress(cvBufferRef, kCVPixelBufferLock_ReadOnly);
    CVBufferRelease((struct __CVBuffer *)m_buffers[index].hwDec);
    m_buffers[index].hwDec = NULL;
    
    plane.flipindex = m_buffers[index].flipindex;
    ret = true;
  }
  
  CalculateTextureSourceRects(index, 1);
  return ret;
}

#endif