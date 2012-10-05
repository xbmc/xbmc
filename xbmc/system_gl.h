#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "system.h"

#ifdef HAS_GL
  // always define GL_GLEXT_PROTOTYPES before include gl headers
  #if !defined(GL_GLEXT_PROTOTYPES)
    #define GL_GLEXT_PROTOTYPES
  #endif
  #if defined(TARGET_WINDOWS)
    #include <GL/glew.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
  #elif defined(TARGET_LINUX)
    #include <GL/glew.h>
    #include <GL/gl.h>
    #include <GL/glext.h>
  #elif defined(TARGET_FREEBSD)
    #include <GL/glew.h>
    #include <GL/gl.h>
  #elif defined(TARGET_DARWIN)
    #include <GL/glew.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
  #endif
#elif HAS_GLES == 2
  #if defined(TARGET_DARWIN)
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
  #else
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
  #endif
#endif
