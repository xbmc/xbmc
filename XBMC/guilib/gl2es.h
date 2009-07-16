/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 *
 * gl2es.h
 *
 *      This file bridges the naming conventions of many OpenGL names with OpenGL ES names.
 *      Some functionality of GL is provided by extensions, but in ES, these are included already.
 *      As a result, several functions and enums require renaming for ES.
 *      Some aspects are not supported at all. For now, these are replaced with NULL.
 *      Some aspects are supported. For now, these are forced to be recognised as true.
 *
 *  Created on: 18-Jun-2009
 *      Author: mcgeagh
 */

#ifndef GL2ES_H_
#define GL2ES_H_

#ifdef HAS_SDL_GLES1
// INCLUDES
#include <GLES/gl.h>
#include <GLES/glext.h>

// FUNCTION NAMES
#define glOrtho                         glOrthof
#define glFrustum                       glFrustumf
#define glActiveTextureARB              glActiveTexture

// ENUM NAMES
#define GL_FRAMEBUFFER_EXT              GL_FRAMEBUFFER_OES
#define GL_FRAMEBUFFER_COMPLETE_EXT     GL_FRAMEBUFFER_COMPLETE_OES
#define GL_COLOR_ATTACHMENT0_EXT        GL_COLOR_ATTACHMENT0_OES
#define GL_TEXTURE0_ARB                 GL_TEXTURE0
#define GL_TEXTURE1_ARB                 GL_TEXTURE1
#define GL_SOURCE0_RGB                  GL_SRC0_RGB
#define GL_SOURCE1_RGB                  GL_SRC1_RGB
#define GL_SOURCE0_ALPHA                GL_SRC0_ALPHA
#define GL_SOURCE1_ALPHA                GL_SRC1_ALPHA

// INVALID
#define GLEW_ARB_texture_non_power_of_two       0
#define GLEW_ARB_shading_language_100           0
#define GL_LINE                                 NULL
#define GL_FILL                                 NULL

// MACROS
#define glColor3f(r, g, b)              glColor4f(r, g, b, 1.0f)

#endif


#ifdef HAS_SDL_GLES2
// INCLUDES
#include <GLES2/gl2extimg.h>

// FUNCTION NAMES
#define glBindFramebufferEXT            glBindFramebuffer
#define glGenFramebuffersEXT            glGenFramebuffers
#define glDeleteFramebuffersEXT         glDeleteFramebuffers
#define glFramebufferTexture2DEXT       glFramebufferTexture2D
#define glCheckFramebufferStatusEXT     glCheckFramebufferStatus
#define glActiveTextureARB              glActiveTexture

// ENUM NAMES
#define GL_FRAMEBUFFER_EXT              GL_FRAMEBUFFER
#define GL_COLOR_ATTACHMENT0_EXT        GL_COLOR_ATTACHMENT0
#define GL_FRAMEBUFFER_COMPLETE_EXT     GL_FRAMEBUFFER_COMPLETE
#define GL_TEXTURE0_ARB                 GL_TEXTURE0

// INVALID
#define GLEW_ARB_texture_non_power_of_two       GL_OES_texture_npot
#define GLEW_ARB_shading_language_100           1                       // Does support shaders!
#define GL_BGRA                                 GL_RGBA                 // need to fix!!!
#define GL_RGBA16F_ARB                          GL_RGBA                 // need to fix!!!
#define GL_LINE                                 NULL
#define GL_FILL                                 NULL

typedef char GLchar;

#endif

#endif /* GL2ES_H_ */
