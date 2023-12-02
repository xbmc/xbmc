/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus

//==============================================================================
/// @defgroup cpp_kodi_gui_helpers_gl OpenGL helpers
/// @ingroup cpp_kodi_gui_helpers
/// @brief **Auxiliary functions for Open GL**\n
/// This group includes help for definitions, functions, and classes for
/// OpenGL.
///
/// To use OpenGL for your system, add the @ref GL.h "#include <kodi/gui/gl/GL.h>".
///
/// The @ref HAS_GL is declared if Open GL is required and @ref HAS_GLES if Open GL
/// Embedded Systems (ES) is required, with ES the version is additionally given
/// in the definition, this can be "2" or "3".
///
///
///-----------------------------------------------------------------------------
///
/// Following @ref GL_TYPE_STRING define can be used, for example, to manage
/// different folders for GL and GLES and make the selection easier.
/// This are on OpenGL <b>"GL"</b> and on Open GL|ES <b>"GLES"</b>.
///
/// **Example:**
/// ~~~~~~~~~~~~~~~~~{.cpp}
/// kodi::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/frag.glsl");
/// ~~~~~~~~~~~~~~~~~
///
///
///----------------------------------------------------------------------------
///
/// In addition, @ref BUFFER_OFFSET is declared in it which can be used to give an
/// offset on the array to GL.
///
/// **Example:**
/// ~~~~~~~~~~~~~~~~~{.cpp}
/// const struct PackedVertex {
///   float position[3]; // Position x, y, z
///   float color[4]; // Color r, g, b, a
/// } vertices[3] = {
///   { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
///   { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
///   { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
/// };
///
/// glVertexAttribPointer(m_aPosition, 3, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, position)));
/// glEnableVertexAttribArray(m_aPosition);
///
/// glVertexAttribPointer(m_aColor, 4, GL_FLOAT, GL_FALSE, sizeof(PackedVertex), BUFFER_OFFSET(offsetof(PackedVertex, color)));
/// glEnableVertexAttribArray(m_aColor);
/// ~~~~~~~~~~~~~~~~~

#if HAS_GL
#define GL_TYPE_STRING "GL"
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
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#elif defined(WIN32)
#error Use of GL under Windows is not possible
#endif
#elif HAS_GLES >= 2
#define GL_TYPE_STRING "GLES"
#if defined(WIN32)
#if defined(HAS_ANGLE)
#include <angle_gl.h>
#else
#error Use of GLES only be available under Windows by the use of angle
#endif
#elif defined(TARGET_DARWIN)
#if HAS_GLES == 3
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#else
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif
#else
#if HAS_GLES == 3
#include <GLES3/gl3.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#endif
#endif

#ifndef BUFFER_OFFSET
/// @ingroup cpp_kodi_gui_helpers_gl
/// @brief To give a offset number as pointer value.
#define BUFFER_OFFSET(i) ((char*)nullptr + (i))
#endif

#endif /* __cplusplus */
