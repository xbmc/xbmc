/*
 *      Copyright (C) 2007-2017 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VdpauGL.h"
#include <GL/glx.h>
#include "cores/VideoPlayer/DVDCodecs/Video/VDPAU.h"
#include "utils/log.h"

using namespace VDPAU;

//-----------------------------------------------------------------------------
// interop state
//-----------------------------------------------------------------------------

bool CInteropState::Init(void *device, void *procFunc, int64_t ident)
{
  m_device = device;
  m_procFunc = procFunc;
  m_ident = ident;

  m_interop.glVDPAUInitNV = (PFNGLVDPAUINITNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUInitNV");
  m_interop.glVDPAUFiniNV = (PFNGLVDPAUFININVPROC)glXGetProcAddress((GLubyte *) "glVDPAUFiniNV");
  m_interop.glVDPAURegisterOutputSurfaceNV = (PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAURegisterOutputSurfaceNV");
  m_interop.glVDPAURegisterVideoSurfaceNV = (PFNGLVDPAUREGISTERVIDEOSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAURegisterVideoSurfaceNV");
  m_interop.glVDPAUIsSurfaceNV = (PFNGLVDPAUISSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAUIsSurfaceNV");
  m_interop.glVDPAUUnregisterSurfaceNV = (PFNGLVDPAUUNREGISTERSURFACENVPROC)glXGetProcAddress((GLubyte *) "glVDPAUUnregisterSurfaceNV");
  m_interop.glVDPAUSurfaceAccessNV = (PFNGLVDPAUSURFACEACCESSNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUSurfaceAccessNV");
  m_interop.glVDPAUMapSurfacesNV = (PFNGLVDPAUMAPSURFACESNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUMapSurfacesNV");
  m_interop.glVDPAUUnmapSurfacesNV = (PFNGLVDPAUUNMAPSURFACESNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUUnmapSurfacesNV");
  m_interop.glVDPAUGetSurfaceivNV = (PFNGLVDPAUGETSURFACEIVNVPROC)glXGetProcAddress((GLubyte *) "glVDPAUGetSurfaceivNV");

  while (glGetError() != GL_NO_ERROR);
  m_interop.glVDPAUInitNV(m_device, m_procFunc);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CInteropState::Init - GLInitInterop glVDPAUInitNV failed");
    return false;
  }
  CLog::Log(LOGNOTICE, "CInteropState::Init: vdpau gl interop initialized");

  m_interop.textureTarget = GL_TEXTURE_2D;

  return true;
}

void CInteropState::Finish()
{
  m_interop.glVDPAUFiniNV();
  m_device = nullptr;
  m_procFunc = nullptr;
}

InteropInfo &CInteropState::GetInterop()
{
  return m_interop;
}

bool CInteropState::NeedInit(void *device, void *procFunc, int64_t ident)
{
  if (m_device != device)
    return true;
  if (m_procFunc != procFunc)
    return true;
  if (m_ident != ident)
    return true;

  return false;
}

//-----------------------------------------------------------------------------
// textures
//-----------------------------------------------------------------------------

void CVdpauTexture::Init(InteropInfo &interop)
{
  m_interop = interop;
}

bool CVdpauTexture::Map(CVdpauRenderPicture *pic)
{

  if (m_vdpauPic)
    return true;

  m_vdpauPic = pic;
  m_vdpauPic->Acquire();

  m_texWidth = pic->width;
  m_texHeight = pic->height;

  bool success = false;
  if (m_vdpauPic->procPic.isYuv)
    success = MapNV12();
  else
    success = MapRGB();

  if (!success)
  {
    m_vdpauPic->Release();
    m_vdpauPic = nullptr;
  }

  return success;
}

void CVdpauTexture::Unmap()
{
  if (!m_vdpauPic)
    return;

  if (m_vdpauPic->procPic.isYuv)
    UnmapNV12();
  else
    UnmapRGB();

  m_vdpauPic->Release();
  m_vdpauPic = nullptr;
}

bool CVdpauTexture::MapNV12()
{
  GLuint textures[4];
  while (glGetError() != GL_NO_ERROR) ;
  glGenTextures(4, textures);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CVdpauTexture::MapNV12 error creating texture");
    return false;
  }

  const void *videoSurface = reinterpret_cast<void*>(m_vdpauPic->procPic.videoSurface);
  m_glSurface.glVdpauSurface = m_interop.glVDPAURegisterVideoSurfaceNV(videoSurface,
                                                                       m_interop.textureTarget, 4, textures);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CVdpauTexture::MapNV12 error register video surface");
    glDeleteTextures(4, textures);
    return false;
  }

  m_interop.glVDPAUSurfaceAccessNV(m_glSurface.glVdpauSurface, GL_READ_ONLY);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CVdpauTexture::MapNV12 error setting access");
    glDeleteTextures(4, textures);
    return false;
  }

  m_interop.glVDPAUMapSurfacesNV(1, &m_glSurface.glVdpauSurface);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CVdpauTexture::MapNV12 error mapping surface");
    glDeleteTextures(4, textures);
    return false;
  }

  m_interop.glVDPAUUnregisterSurfaceNV(m_glSurface.glVdpauSurface);

  m_textureTopY = textures[0];
  m_textureTopUV = textures[2];
  m_textureBotY = textures[1];
  m_textureBotUV = textures[3];

  return true;
}

void CVdpauTexture::UnmapNV12()
{
  m_interop.glVDPAUUnmapSurfacesNV(1, &m_glSurface.glVdpauSurface);
  glDeleteTextures(1, &m_textureTopY);
  glDeleteTextures(1, &m_textureTopUV);
  glDeleteTextures(1, &m_textureBotY);
  glDeleteTextures(1, &m_textureBotUV);
}

bool CVdpauTexture::MapRGB()
{
  glGenTextures(1, &m_texture);
  const void *outSurface = reinterpret_cast<void*>(m_vdpauPic->procPic.outputSurface);
  m_glSurface.glVdpauSurface = m_interop.glVDPAURegisterOutputSurfaceNV(outSurface,
                                                                        m_interop.textureTarget, 1, &m_texture);
  GLenum err = glGetError();
  if (err != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CVdpauTexture::MapRGB error register output surface: %d", err);
    return false;
  }

  m_interop.glVDPAUSurfaceAccessNV(m_glSurface.glVdpauSurface, GL_READ_ONLY);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "CVdpauTexture::MapRGB error setting access");
    glDeleteTextures(1, &m_texture);
    return false;
  }

  while (glGetError() != GL_NO_ERROR) ;
  m_interop.glVDPAUMapSurfacesNV(1, &m_glSurface.glVdpauSurface);
  if (glGetError() != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "VDPAU::COutput error mapping surface");
    glDeleteTextures(1, &m_texture);
    return false;
  }

  return true;
}

void CVdpauTexture::UnmapRGB()
{
  m_interop.glVDPAUUnmapSurfacesNV(1, &m_glSurface.glVdpauSurface);
  m_interop.glVDPAUUnregisterSurfaceNV(m_glSurface.glVdpauSurface);
  glDeleteTextures(1, &m_texture);
}

