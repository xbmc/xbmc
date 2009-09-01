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
#include "../xbmc/Picture.h"

#ifdef HAS_GL

#define MAX_PICTURE_WIDTH  2048
#define MAX_PICTURE_HEIGHT 2048

using namespace std;

extern "C" void dllprintf( const char *format, ... );

DWORD PadPow2(DWORD x) 
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

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

void CGLTexture::Allocate(unsigned int width, unsigned int height, unsigned int BPP)
{
  if(BPP != 0)
    m_nBPP = BPP;

  if (g_Windowing.NeedPower2Texture())
  {
    m_nTextureWidth = PadPow2(m_imageWidth);
    m_nTextureHeight = PadPow2(m_imageHeight);
  }
  else
  {
    m_nTextureWidth = m_imageWidth;
    m_nTextureHeight = m_imageHeight;
  }

  m_pPixels = new unsigned char[m_nTextureWidth * m_nTextureHeight * m_nBPP / 8];
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

bool CGLTexture::LoadFromFile(const CStdString& texturePath)
{
  CPicture pic;
  CBaseTexture* original = pic.Load(texturePath, MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT);
  if (!original)
  {
    CLog::Log(LOGERROR, "Texture manager unable to load file: %s", texturePath.c_str());
    return 0;
  }
  m_imageWidth = original->GetWidth();
  m_imageHeight = original->GetHeight();
  m_nBPP = original->GetBPP();

  Update(original->GetWidth(), original->GetHeight(), original->GetPitch(), original->GetPixels(), false);

  delete original;

  return true;
}


bool CGLTexture::LoadFromMemory(unsigned int width, unsigned int height, unsigned int pitch, unsigned int BPP, unsigned char* pPixels)
{
  m_imageWidth = width;
  m_imageHeight = height;
  m_nBPP = BPP;
  Update(width, height, pitch, pPixels, false);

  return TRUE;
}

void CGLTexture::LoadToGPU()
{
  if (!m_pPixels) {
    // nothing to load - probably same image (no change)
    return;
  }

  //g_graphicsContext.BeginPaint();
  if (m_pTexture == 0) {
    // Have OpenGL generate a texture object handle for us
    // this happens only one time - the first time the texture is loaded
    glGenTextures(1, (GLuint*) &m_pTexture);
  }

  // Bind the texture object
  glBindTexture(GL_TEXTURE_2D, m_pTexture);

  // Set the texture's stretching properties
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  static unsigned int maxSize = MAX_PICTURE_WIDTH;
  {
    if (m_nTextureHeight > maxSize)
    {
      CLog::Log(LOGERROR, "GL: Image height %d too big to fit into single texture unit, truncating to %d", m_nTextureHeight, (int) maxSize);
      m_nTextureHeight = maxSize;
    }
    if (m_nTextureWidth > maxSize)
    {
      CLog::Log(LOGERROR, "GL: Image width %d too big to fit into single texture unit, truncating to %d", m_nTextureWidth, (int) maxSize);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, m_nTextureWidth);
      m_nTextureWidth = maxSize;
    }
  }
  //CLog::Log(LOGNOTICE, "Texture width x height: %d x %d", textureWidth, textureHeight);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, m_nTextureWidth, m_nTextureHeight, 0,
    GL_BGRA, GL_UNSIGNED_BYTE, m_pPixels);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  VerifyGLState();

  //g_graphicsContext.EndPaint();
  delete [] m_pPixels;
  m_pPixels = NULL;

  m_loadedToGPU = true;   
}

unsigned int CGLTexture::GetPitch() const
{
  return m_nTextureWidth * (m_nBPP / 8);
}

unsigned int CGLTexture::GetTextureWidth() const
{
  return m_nTextureWidth;
}

unsigned int CGLTexture::GetTextureHeight() const
{
  return m_nTextureHeight;
}

unsigned char* CGLTexture::GetPixels() const
{
  return m_pPixels;
}

void CGLTexture::Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU) 
{
  int tpitch;

  if (m_pPixels)
  {
    delete [] m_pPixels;
    m_pPixels = NULL;
  }

  Allocate(w, h, 0);

  // Resize texture to POT
  const unsigned char *src = pixels;
  tpitch = pitch;
  unsigned char* resized = m_pPixels;

  for (int y = 0; y < h; y++)
  {
    memcpy(resized, src, tpitch); // make sure pitch is not bigger than our width
    src += pitch;

    // repeat last column to simulate clamp_to_edge
    for(unsigned int i = tpitch; i < m_nTextureWidth*4; i+=4)
      memcpy(resized+i, src-4, 4);

    resized += (m_nTextureWidth * 4);
  }

  // repeat last row to simulate clamp_to_edge
  for(unsigned int y = h; y < m_nTextureHeight; y++) 
  {
    memcpy(resized, src - tpitch, tpitch);

    // repeat last column to simulate clamp_to_edge
    for(unsigned int i = tpitch; i < m_nTextureWidth*4; i+=4) 
      memcpy(resized+i, src-4, 4);

    resized += (m_nTextureWidth * 4);
  }

  if (loadToGPU)
    LoadToGPU();
}

#endif // HAS_GL
