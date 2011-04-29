/*
*      Copyright (C) 2005-2008 Team XBMC
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
*/

#include "system.h"
#include "TextureGL.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

#if defined(HAS_GL) || defined(HAS_GLES)

using namespace std;

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
CGLTexture::CGLTexture(unsigned int width, unsigned int height, unsigned int format)
: CBaseTexture(width, height, format)
{
}

CGLTexture::~CGLTexture()
{
  DestroyTextureObject();
}

void CGLTexture::CreateTextureObject()
{
  glGenTextures(1, (GLuint*) &m_texture);
}

void CGLTexture::DestroyTextureObject()
{
  if (m_texture)
    glDeleteTextures(1, (GLuint*) &m_texture);
}

void CGLTexture::LoadToGPU()
{
  if (!m_pixels)
  {
    // nothing to load - probably same image (no change)
    return;
  }

  if (m_texture == 0)
  {
    // Have OpenGL generate a texture object handle for us
    // this happens only one time - the first time the texture is loaded
    CreateTextureObject();
  }

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, m_texture);

  // Set the texture's stretching properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  unsigned int maxSize = g_Windowing.GetMaxTextureSize();
  if (m_textureHeight > maxSize)
  {
    CLog::Log(LOGERROR, "GL: Image height %d too big to fit into single texture unit, truncating to %u", m_textureHeight, maxSize);
    m_textureHeight = maxSize;
  }
  if (m_textureWidth > maxSize)
  {
    CLog::Log(LOGERROR, "GL: Image width %d too big to fit into single texture unit, truncating to %u", m_textureWidth, maxSize);
#ifndef HAS_GLES
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_textureWidth);
    m_textureWidth = maxSize;
  }

  GLenum format;

  switch (m_format)
  {
  case XB_FMT_DXT1:
    format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    break;
  case XB_FMT_DXT3:
    format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    break;
  case XB_FMT_DXT5:
  case XB_FMT_DXT5_YCoCg:
    format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    break;
  case XB_FMT_A8R8G8B8:
  default:
    format = GL_BGRA;
    break;
  }

  if ((m_format & XB_FMT_DXT_MASK) == 0)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, 4, m_textureWidth, m_textureHeight, 0,
      format, GL_UNSIGNED_BYTE, m_pixels);
  }
  else
  {
    // changed from glCompressedTexImage2D to support GL < 1.3
    glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, format,
      m_textureWidth, m_textureHeight, 0, GetPitch() * GetRows(), m_pixels);
  }

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#else	// GLES version
    m_textureWidth = maxSize;
  }

  GLenum format;

  switch (m_format)
  {
  case XB_FMT_PVR2:
    format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
    break;
  case XB_FMT_PVR4:
    format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
    break;
  case XB_FMT_A8R8G8B8:
  default:
#if HAS_GLES == 1
    format = GL_BGRA_EXT;
#elif HAS_GLES == 2
    format = GL_RGBA;
#endif
    break;
  }

  if ( ((m_format & XB_FMT_PVR2) == 0)  && ((m_format & XB_FMT_PVR4) == 0) )
  {
    glTexImage2D(GL_TEXTURE_2D, 0, format, m_textureWidth, m_textureHeight, 0,
      format, GL_UNSIGNED_BYTE, m_pixels);
  }
  else
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format,
      m_textureWidth, m_textureHeight, 0, GetPitch() * GetRows(), m_pixels);
  }

#endif
  VerifyGLState();

  delete [] m_pixels;
  m_pixels = NULL;

  m_loadedToGPU = true;
}
#endif // HAS_GL
