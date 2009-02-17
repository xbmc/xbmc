/*!
\file GUITexture.h
\brief 
*/

#ifndef GUILIB_GUITEXTURE_H
#define GUILIB_GUITEXTURE_H

#pragma once

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

#include "TextureManager.h"
#include "Geometry.h"

struct FRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

class CTextureInfo
{
public:
  CTextureInfo()
  {
    memset(&border, 0, sizeof(FRECT));
    orientation = 0;
    useLarge = false;
  };

  void operator=(const CTextureInfo &right)
  {
    memcpy(&border, &right.border, sizeof(FRECT));
    orientation = right.orientation;
    diffuse = right.diffuse;
    filename = right.filename;
    useLarge = right.useLarge;
  };
  bool       useLarge;
  FRECT      border;      // scaled  - unneeded if we get rid of scale on load
  int        orientation; // orientation of the texture (0 - 7 == EXIForientation - 1)
  CStdString diffuse;     // diffuse overlay texture
  CStdString filename;    // main texture file
};

class CGUITextureBase
{
public:
  CGUITextureBase(float posX, float posY, float width, float height, const CTextureInfo& texture);
  CGUITextureBase(const CGUITextureBase &left);
  virtual ~CGUITextureBase(void);

  void Render();

  void DynamicResourceAlloc(bool bOnOff);
  void PreAllocResources();
  void AllocResources();
  void FreeResources(bool immediately = false);

  void SetAlpha(unsigned char alpha);
  void SetPosition(float x, float y);
  void SetWidth(float width);
  void SetHeight(float height);
  void SetFileName(const CStdString &filename);
  void SetDiffuseScaling(float scaleU, float scaleV, float offsetU, float offsetV);
  
  const CStdString& GetFileName() const { return m_info.filename; };
  int GetTextureWidth() const { return m_frameWidth; };
  int GetTextureHeight() const { return m_frameHeight; };
  float GetWidth() const { return m_width; };
  float GetHeight() const { return m_height; };
  int GetOrientation() const;

  bool IsAllocated() const;
protected:
  void CalculateSize();
  void LoadDiffuseImage();
  void AllocateOnDemand();
  void UpdateAnimFrame();
  void Render(float left, float top, float bottom, float right, float u1, float v1, float u2, float v2, float u3, float v3);
  void OrientateTexture(CRect &rect, float width, float height, int orientation);

  // functions that our implementation classes handle
  virtual void Allocate() {}; ///< called after our textures have been allocated
  virtual void Free() {};     ///< called after our textures have been freed
  virtual void Begin() {};
  virtual void Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, DWORD color, int orientation)=0;
  virtual void End() {};

  bool m_visible;
  DWORD m_diffuseColor;

  float m_posX;
  float m_posY;
  float m_width;
  float m_height;

  unsigned char m_alpha;

  int m_frameWidth;       // size in pixels of the actual frame within the texture
  int m_frameHeight;
  float m_texCoordsScaleU, m_texCoordsScaleV;

  // animations
  int m_currentLoop;
  unsigned int m_currentFrame;
  DWORD m_frameCounter;

  float m_diffuseFrameU, m_diffuseFrameV; // size of the diffuse frame (in tex coords)
  float m_diffuseScaleU, m_diffuseScaleV; // scale factor of the diffuse frame (it's not always 1:1)
  CPoint m_diffuseOffset;                 // offset into the diffuse frame (it's not always the origin)

  bool m_allocateDynamically;
  bool m_isAllocated;

  CTextureInfo m_info;

  bool m_usingLargeTexture; // true if we're using a large texture
  int m_largeOrientation;   // orientation for large textures

  CBaseTexture m_diffuse;
  std::vector<CBaseTexture> m_textures;
};

#ifndef HAS_SDL
#include "GUITextureD3D.h"
#elif defined(HAS_SDL_2D)
#include "GUITextureSDL.h"
#elif defined(HAS_SDL_OPENGL)
#include "GUITextureGL.h"
#endif

#endif
