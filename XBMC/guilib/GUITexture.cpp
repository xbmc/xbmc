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
#include "GUITexture.h"
#include "GraphicContext.h"
#include "TextureManager.h"
#include "GUILargeTextureManager.h"
#include "Util.h" // for mathutils

using namespace std;

CGUITextureBase::CGUITextureBase(float posX, float posY, float width, float height, const CTextureInfo& texture)
{
  m_posX = posX;
  m_posY = posY;
  m_width = width;
  m_height = height;
  m_info = texture;

  // defaults
  m_visible = true;
  m_diffuseColor = 0xffffffff;
  m_alpha = 0xff;

  m_frameWidth = 0;
  m_frameHeight = 0;
  m_diffuseFrameU = 0;
  m_diffuseFrameV = 0;

  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;
  m_diffuseScaleU = 1.0f;
  m_diffuseScaleV = 1.0f;
  m_largeOrientation = 0;

  // anim gifs
  m_currentFrame = 0;
  m_frameCounter = (DWORD) -1;
  m_currentLoop = 0;

  m_allocateDynamically = false;
  m_isAllocated = false;
}

CGUITextureBase::CGUITextureBase(const CGUITextureBase &right)
{
  m_posX = right.m_posX;
  m_posY = right.m_posY;
  m_width = right.m_width;
  m_height = right.m_height;
  m_info = right.m_info;

  m_visible = right.m_visible;
  m_diffuseColor = right.m_diffuseColor;
  m_alpha = right.m_alpha;

  m_allocateDynamically = right.m_allocateDynamically;

  // defaults
  m_frameWidth = 0;
  m_frameHeight = 0;
  m_diffuseFrameU = 0;
  m_diffuseFrameV = 0;

  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;
  m_diffuseScaleU = 1.0f;
  m_diffuseScaleV = 1.0f;

  m_largeOrientation = 0;

  m_currentFrame = 0;
  m_frameCounter = (DWORD) -1;
  m_currentLoop = 0;

  m_isAllocated = false;
}

CGUITextureBase::~CGUITextureBase(void)
{
}

void CGUITextureBase::AllocateOnDemand()
{
  if (m_visible)
  { // visible, so make sure we're allocated
    if (!m_isAllocated || (m_info.useLarge && !m_textures.size()))
      AllocResources();
  }
  else
  { // hidden, so deallocate as applicable
    if (m_allocateDynamically && m_isAllocated)
      FreeResources();
    // reset animated textures (animgifs)
    m_currentLoop = 0;
    m_currentFrame = 0;
    m_frameCounter = 0;
  }
}

void CGUITextureBase::Render()
{
  // check if we need to allocate our resources
  AllocateOnDemand();

  if (!m_visible || !m_textures.size())
    return;

  if (m_textures.size() > 1)
    UpdateAnimFrame();

  // setup our renderer
  Begin();

  // compute the texture coordinates
  float u1, u2, u3, v1, v2, v3;
  u1 = m_info.border.left;
  u2 = m_frameWidth - m_info.border.right;
  u3 = (float)m_frameWidth;
  v1 = m_info.border.top;
  v2 = m_frameHeight - m_info.border.bottom;
  v3 = (float)m_frameHeight;

  if (!m_textures[m_currentFrame].m_texCoordsArePixels)
  {
    u1 *= m_texCoordsScaleU;
    u2 *= m_texCoordsScaleU;
    u3 *= m_texCoordsScaleU;
    v1 *= m_texCoordsScaleV;
    v2 *= m_texCoordsScaleV;
    v3 *= m_texCoordsScaleV;
  }

  // TODO: The diffuse coloring applies to all vertices, which will
  //       look weird for stuff with borders, as will the -ve height/width
  //       for flipping

  // left segment (0,0,u1,v3)
  if (m_info.border.left)
  {
    if (m_info.border.top)
      Render(m_posX, m_posY, m_posX + m_info.border.left, m_posY + m_info.border.top, 0, 0, u1, v1, u3, v3);
    Render(m_posX, m_posY + m_info.border.top, m_posX + m_info.border.left, m_posY + m_height - m_info.border.bottom, 0, v1, u1, v2, u3, v3);
    if (m_info.border.bottom)
      Render(m_posX, m_posY + m_height - m_info.border.bottom, m_posX + m_info.border.left, m_posY + m_height, 0, v2, u1, v3, u3, v3); 
  }
  // middle segment (u1,0,u2,v3)
  if (m_info.border.top)
    Render(m_posX + m_info.border.left, m_posY, m_posX + m_width - m_info.border.right, m_posY + m_info.border.top, u1, 0, u2, v1, u3, v3);
  Render(m_posX + m_info.border.left, m_posY + m_info.border.top, m_posX + m_width - m_info.border.right, m_posY + m_height - m_info.border.bottom, u1, v1, u2, v2, u3, v3);
  if (m_info.border.bottom)
    Render(m_posX + m_info.border.left, m_posY + m_height - m_info.border.bottom, m_posX + m_width - m_info.border.right, m_posY + m_height, u1, v2, u2, v3, u3, v3); 
  // right segment
  if (m_info.border.right)
  { // have a left border
    if (m_info.border.top)
      Render(m_posX + m_width - m_info.border.right, m_posY, m_posX + m_width, m_posY + m_info.border.top, u2, 0, u3, v1, u3, v3);
    Render(m_posX + m_width - m_info.border.right, m_posY + m_info.border.top, m_posX + m_width, m_posY + m_height - m_info.border.bottom, u2, v1, u3, v2, u3, v3);
    if (m_info.border.bottom)
      Render(m_posX + m_width - m_info.border.right, m_posY + m_height - m_info.border.bottom, m_posX + m_width, m_posY + m_height, u2, v2, u3, v3, u3, v3); 
  } 

  // close off our renderer
  End();
}

void CGUITextureBase::Render(float left, float top, float right, float bottom, float u1, float v1, float u2, float v2, float u3, float v3)
{
  CRect diffuse(u1, v1, u2, v2);
  CRect texture(u1, v1, u2, v2);
  CRect vertex(left, top, right, bottom);
  g_graphicsContext.ClipRect(vertex, texture, m_diffuse.m_texture ? &diffuse : NULL);

  int orientation = GetOrientation();
  OrientateTexture(texture, u3, v3, orientation);

  if (m_diffuse.m_texture)
  {
    // flip the texture as necessary.  Diffuse just gets flipped according to m_info.orientation.
    // Main texture gets flipped according to GetOrientation().
    diffuse.x1 *= m_diffuseFrameU * m_diffuseScaleU / u3; diffuse.x2 *= m_diffuseFrameU * m_diffuseScaleU / u3;
    diffuse.y1 *= m_diffuseFrameV * m_diffuseScaleU / v3; diffuse.y2 *= m_diffuseFrameV * m_diffuseScaleV / v3;
    diffuse += m_diffuseOffset;
    OrientateTexture(diffuse, u3, v3, m_info.orientation);
  }

  if (vertex.IsEmpty())
    return; // nothing to render

  float x[4], y[4], z[4];

#define ROUND_TO_PIXEL(x) (float)(MathUtils::round_int(x))

  x[0] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y1));
  y[0] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y1));
  z[0] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y1));
  x[1] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y1));
  y[1] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y1));
  z[1] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y1));
  x[2] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y2));
  y[2] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y2));
  z[2] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y2));
  x[3] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y2));
  y[3] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y2));
  z[3] = ROUND_TO_PIXEL(g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y2));
  
  if (y[2] == y[0]) y[2] += 1.0f; if (x[2] == x[0]) x[2] += 1.0f;
  if (y[3] == y[1]) y[3] += 1.0f; if (x[3] == x[1]) x[3] += 1.0f;

#define MIX_ALPHA(a,c) (((a * (c >> 24)) / 255) << 24) | (c & 0x00ffffff)

  DWORD color = m_diffuseColor;
  if (m_alpha != 0xFF) color = MIX_ALPHA(m_alpha, m_diffuseColor);
  color = g_graphicsContext.MergeAlpha(color);

  Draw(x, y, z, texture, diffuse, color, orientation);
}

void CGUITextureBase::PreAllocResources()
{
  FreeResources();
  if (!m_info.useLarge)
    g_TextureManager.PreLoad(m_info.filename);
  if (!m_info.diffuse.IsEmpty())
    g_TextureManager.PreLoad(m_info.diffuse);
}

void CGUITextureBase::AllocResources()
{
  if (m_info.filename.IsEmpty())
    return;

  if (m_textures.size())
    FreeResources();

  // reset our animstate
  m_frameCounter = 0;
  m_currentFrame = 0;
  m_currentLoop = 0;

  if (m_info.useLarge)
  { // we want to use the large image loader, but we first check for bundled textures
    int images = g_TextureManager.Load(m_info.filename, 0, true);
    if (images)
    {
      m_isAllocated = true;
      for (int i = 0; i < images; i++)
        m_textures.push_back(g_TextureManager.GetTexture(m_info.filename, i));
      m_frameWidth = m_textures[0].m_width;
      m_frameHeight = m_textures[0].m_height;
    }
    else
    { // use our large image background loader
      CBaseTexture texture = g_largeTextureManager.GetImage(m_info.filename, m_largeOrientation, !m_isAllocated);
      m_isAllocated = true;

      if (!texture.m_texture) // not ready as yet
        return;

      m_frameWidth = texture.m_width;
      m_frameHeight = texture.m_height;

      m_usingLargeTexture = true;
      m_textures.push_back(texture);
    }
  }
  else
  {
    int images = g_TextureManager.Load(m_info.filename, 0);

    // set allocated to true even if we couldn't load the image to save
    // us hitting the disk every frame
    m_isAllocated = true;
    if (!images)
      return;

    for (int i = 0; i < images; i++)
      m_textures.push_back(g_TextureManager.GetTexture(m_info.filename, i));
    
    m_frameWidth = m_textures[0].m_width;
    m_frameHeight = m_textures[0].m_height;
    m_usingLargeTexture = false;
  }

  CalculateSize();

  LoadDiffuseImage();

  // call our implementation
  Allocate();
}

void CGUITextureBase::CalculateSize()
{
  if (m_currentFrame >= m_textures.size())
    return;

  m_texCoordsScaleU = 1.0f/m_textures[m_currentFrame].m_texWidth;
  m_texCoordsScaleV = 1.0f/m_textures[m_currentFrame].m_texHeight;

  if (m_width == 0)
    m_width = (float)m_frameWidth;
  if (m_height == 0)
    m_height = (float)m_frameHeight;
}

void CGUITextureBase::LoadDiffuseImage()
{
  m_diffuseScaleU = m_diffuseScaleV = 1.0f;
  m_diffuseOffset = CPoint(0, 0);
  // load the diffuse texture (if necessary)
  if (!m_info.diffuse.IsEmpty())
  {
    g_TextureManager.Load(m_info.diffuse, 0);
    m_diffuse = g_TextureManager.GetTexture(m_info.diffuse, 0);
    if (m_diffuse.m_texture)
    { // calculate scaling for the texcoords
      if (m_diffuse.m_texCoordsArePixels)
      {
        m_diffuseFrameU = float(m_diffuse.m_width);
        m_diffuseFrameV = float(m_diffuse.m_height);
      }
      else
      {
        m_diffuseFrameU = float(m_diffuse.m_width) / float(m_diffuse.m_texWidth);
        m_diffuseFrameV = float(m_diffuse.m_height) / float(m_diffuse.m_texHeight);
      }
    }
  }
}

void CGUITextureBase::FreeResources(bool immediately /* = false */)
{
  if (m_isAllocated)
  {
    if (m_usingLargeTexture)
      g_largeTextureManager.ReleaseImage(m_info.filename, immediately);
    else
    {
      for (int i = 0; i < (int)m_textures.size(); ++i)
      {
        g_TextureManager.ReleaseTexture(m_info.filename, i);
      }
    }
  }

  if (m_diffuse.m_texture)
    g_TextureManager.ReleaseTexture(m_info.diffuse);
  m_diffuse.Reset();

  m_textures.clear();

  m_currentFrame = 0;
  m_currentLoop = 0;
  m_texCoordsScaleU = 1.0f;
  m_texCoordsScaleV = 1.0f;
  m_largeOrientation = 0;

  // call our implementation
  Free();

  m_isAllocated = false;
}

void CGUITextureBase::DynamicResourceAlloc(bool allocateDynamically)
{
  m_allocateDynamically = allocateDynamically;
}

void CGUITextureBase::UpdateAnimFrame()
{
  m_frameCounter++;
  DWORD delay = g_TextureManager.GetDelay(m_info.filename, m_currentFrame);
  int maxLoops = g_TextureManager.GetLoops(m_info.filename, m_currentFrame);
  if (!delay) delay = 100;
  if (m_frameCounter * 40 >= delay)
  {
    m_frameCounter = 0;
    if (m_currentFrame + 1 >= m_textures.size())
    {
      if (maxLoops > 0)
      {
        if (m_currentLoop + 1 < maxLoops)
        {
          m_currentLoop++;
          m_currentFrame = 0;
        }
      }
      else
      {
        // 0 == loop forever
        m_currentFrame = 0;
      }
    }
    else
    {
      m_currentFrame++;
    }
  }
}

void CGUITextureBase::SetAlpha(unsigned char alpha)
{
  m_alpha = alpha;
}

bool CGUITextureBase::IsAllocated() const
{
  return m_textures.size() > 0;
}

void CGUITextureBase::OrientateTexture(CRect &rect, float width, float height, int orientation)
{
  switch (orientation & 3)
  {
  case 0:
    // default
    break;
  case 1:
    // flip in X direction
    rect.x1 = width - rect.x1;
    rect.x2 = width - rect.x2;
    break;
  case 2:
    // rotate 180 degrees
    rect.x1 = width - rect.x1;
    rect.x2 = width - rect.x2;
    rect.y1 = height - rect.y1;
    rect.y2 = height - rect.y2;
    break;
  case 3:
    // flip in Y direction
    rect.y1 = height - rect.y1;
    rect.y2 = height - rect.y2;
    break;
  }
  if (orientation & 4)
  {
    // we need to swap x and y coordinates but only within the width,height block
    float temp = rect.x1;
    rect.x1 = rect.y1 * width/height;
    rect.y1 = temp * height/width;
    temp = rect.x2;
    rect.x2 = rect.y2 * width/height;
    rect.y2 = temp * height/width;
  }
}

void CGUITextureBase::SetWidth(float width)
{
  if (width < m_info.border.left + m_info.border.right)
    width = m_info.border.left + m_info.border.right;
  m_width = width;
}

void CGUITextureBase::SetHeight(float height)
{
  if (height < m_info.border.top + m_info.border.bottom)
    height = m_info.border.top + m_info.border.bottom;
  m_height = height;
}

void CGUITextureBase::SetPosition(float posX, float posY)
{
  m_posX = posX;
  m_posY = posY;
}

// this routine must only be called after allocation.
void CGUITextureBase::SetDiffuseScaling(float scaleU, float scaleV, float offsetU, float offsetV)
{
  // scaleU and scaleV are amount to scale the diffuse by
  m_diffuseScaleU = scaleU;
  m_diffuseScaleV = scaleV;
  m_diffuseOffset = CPoint(offsetU * m_diffuseFrameU * m_diffuseScaleU, offsetV * m_diffuseFrameV * m_diffuseScaleV);
}

void CGUITextureBase::SetFileName(const CStdString& filename)
{
  if (m_info.filename.Equals(filename)) return;
  // Don't completely free resources here - we may be just changing
  // filenames mid-animation
  FreeResources();
  m_info.filename = filename;
  // Don't allocate resources here as this is done at render time
}

int CGUITextureBase::GetOrientation() const
{
  if (!m_usingLargeTexture)
    return m_info.orientation;
  // otherwise multiply our orientations
  static char orient_table[] = { 0, 1, 2, 3, 4, 5, 6, 7,
                                 1, 0, 3, 2, 5, 4, 7, 6,
                                 2, 3, 0, 1, 6, 7, 4, 5,
                                 3, 2, 1, 0, 7, 6, 5, 4,
                                 4, 7, 6, 5, 0, 3, 2, 1,
                                 5, 6, 7, 4, 1, 2, 3, 0,
                                 6, 5, 4, 7, 2, 1, 0, 3,
                                 7, 4, 5, 6, 3, 0, 1, 2 };
  return (int)orient_table[8 * m_info.orientation + m_largeOrientation];
}
