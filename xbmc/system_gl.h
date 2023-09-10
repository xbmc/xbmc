/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef HAS_GL
// always define GL_GLEXT_PROTOTYPES before include gl headers
#if !defined(GL_GLEXT_PROTOTYPES)
#define GL_GLEXT_PROTOTYPES
#endif
#if defined(TARGET_LINUX)
#include <GL/gl.h>
#include <GL/glext.h>
#elif defined(TARGET_FREEBSD)
#include <GL/gl.h>
#elif defined(TARGET_DARWIN)
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#endif
#elif HAS_GLES >= 2
#if defined(TARGET_DARWIN)
// ios/tvos GLES3 headers include GLES2 definitions, so we can only include one.
#if HAS_GLES == 3
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#else
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#if HAS_GLES == 3
#include <GLES3/gl3.h>
#endif
#endif
#endif
