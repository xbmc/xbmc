/*
 *      Copyright (C) 2007-2017 Team XBMC
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

#pragma once

#if defined(HAVE_LIBGL)
#include <GL/gl.h>
#elif defined(HAVE_LIBGLESV2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <va/va.h>

namespace VAAPI
{

class CVaapiRenderPicture;

struct InteropInfo
{
  EGLDisplay eglDisplay = nullptr;
  PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
  PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR;
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
  GLenum textureTarget;
};

class CVaapiTexture
{
public:
  bool Map(CVaapiRenderPicture *pic);
  void Unmap();
  void Init(InteropInfo &interop);
  static void TestInterop(VADisplay vaDpy, EGLDisplay eglDisplay, bool &general, bool &hevc);

  GLuint m_texture = 0;
  GLuint m_textureY = 0;
  GLuint m_textureVU = 0;
  int m_texWidth = 0;
  int m_texHeight = 0;

protected:
  static bool TestInteropHevc(VADisplay vaDpy, EGLDisplay eglDisplay);

  InteropInfo m_interop;
  CVaapiRenderPicture *m_vaapiPic = nullptr;
  struct GLSurface
  {
    VAImage vaImage;
    VABufferInfo vBufInfo;
    EGLImageKHR eglImage;
    EGLImageKHR eglImageY, eglImageVU;
  } m_glSurface;
};
}

