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

#include "include.h"
#include "TextureGL.h"
#include "WindowingFactory.h"

#ifdef HAS_GL

using namespace std;

/************************************************************************/
/*    CGLTexture                                                       */
/************************************************************************/
CGLTexture::CGLTexture(unsigned int width, unsigned int height, unsigned int BPP)
: CBaseTexture(width, height, BPP)
{
  m_nTextureWidth = 0;
  m_nTextureHeight = 0;

  if(m_imageWidth != 0 && m_imageHeight != 0)
    Allocate(m_imageWidth, m_imageHeight, m_nBPP);
}

CGLTexture::~CGLTexture()
{
  Delete();
}

CGLTexture::CGLTexture(CBaseTexture& texture)
{
  m_nTextureWidth = 0;
  m_nTextureHeight = 0;

  *this = texture;
}

CBaseTexture& CGLTexture::operator = (const CBaseTexture &rhs)
{
  if (this != &rhs) 
  {
    m_pTexture = rhs.GetTextureObject();
    m_imageWidth = rhs.GetWidth();
    m_imageHeight = rhs.GetHeight();
  }

  return *this;
}

void CGLTexture::Delete()
{
  m_imageWidth = 0;
  m_imageHeight = 0;

  if(m_pPixels)
  {
    delete [] m_pPixels;
    m_pPixels = NULL;
  }
}

void CGLTexture::CreateTextureObject()
{
   glGenTextures(1, (GLuint*) &m_pTexture);
}

void CGLTexture::DestroyTextureObject()
{
  glDeleteTextures(1, (GLuint*) &m_pTexture);
}

void CGLTexture::LoadToGPU()
{
  if (!m_pPixels)
  {
    // nothing to load - probably same image (no change)
    return;
  }

  if (m_pTexture == 0)
  {
    // Have OpenGL generate a texture object handle for us
    // this happens only one time - the first time the texture is loaded
    CreateTextureObject();
  }

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, m_pTexture);

  // Set the texture's stretching properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  unsigned int maxSize = g_Windowing.GetMaxTextureSize();
  {
    if (m_nTextureHeight > maxSize)
    {
      CLog::Log(LOGERROR, "GL: Image height %d too big to fit into single texture unit, truncating to %u", m_nTextureHeight, maxSize);
      m_nTextureHeight = maxSize;
    }
    if (m_nTextureWidth > maxSize)
    {
      CLog::Log(LOGERROR, "GL: Image width %d too big to fit into single texture unit, truncating to %u", m_nTextureWidth, maxSize);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, m_nTextureWidth);
      m_nTextureWidth = maxSize;
    }
  }
  //CLog::Log(LOGNOTICE, "Texture width x height: %d x %d", textureWidth, textureHeight);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, m_nTextureWidth, m_nTextureHeight, 0,
    GL_BGRA, GL_UNSIGNED_BYTE, m_pPixels);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  VerifyGLState();

  delete [] m_pPixels;
  m_pPixels = NULL;

  m_loadedToGPU = true;   
}
#endif // HAS_GL
