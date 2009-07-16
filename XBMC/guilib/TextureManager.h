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

/*!
\file TextureManager.h
\brief 
*/

#ifndef GUILIB_TEXTUREMANAGER_H
#define GUILIB_TEXTUREMANAGER_H

#include "TextureBundle.h"
#include <vector>

#pragma once

// This will be subclass to render spec (DX, GL etc.)
/************************************************************************/
/*                                                                      */
/************************************************************************/
class CBaseTexture
{

public:
  
  CBaseTexture(void* surface, bool loadToGPU = true, bool freeSurface = false);
  virtual ~CBaseTexture();

  virtual void LoadToGPU() = 0;
  virtual void Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU) = 0; 
  virtual void Update(void *surface, bool loadToGPU, bool freeSurface) = 0;

  int imageWidth;
  int imageHeight;
  int textureWidth;
  int textureHeight;
  unsigned int id;
  unsigned char* m_pixels;
  bool m_loadedToGPU;
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
class CTextureArray
{
public:
  CTextureArray(int width, int height, int loops, bool texCoordsArePixels = false);
  CTextureArray();

  virtual ~CTextureArray();

  void Reset();

  virtual void Add(void *texture, int delay) = 0;
  virtual void Set(void *texture, int width, int height) = 0;
  virtual void Free() = 0;
  
  unsigned int size() const;

  std::vector<void* > m_textures;
  std::vector<int> m_delays;
  int m_width;
  int m_height;
  int m_loops;
  int m_texWidth;
  int m_texHeight;
  bool m_texCoordsArePixels;
};

/*!
\ingroup textures
\brief 
*/
/************************************************************************/
/*                                                                      */
/************************************************************************/
class CTextureMap
{
public:
  CTextureMap();
  CTextureMap(const CStdString& textureName, int width, int height, int loops);
  virtual ~CTextureMap();

  virtual void Add(void* pTexture, int delay) = 0;

  bool Release();
  const CStdString& GetName() const;
  const CTextureArray* GetTexture();
  void Dump() const;
  DWORD GetMemoryUsage() const;
  void Flush();
  bool IsEmpty() const;

protected:
  void FreeTexture();

  CTextureArray* m_texture;
  CStdString m_textureName;
  unsigned int m_referenceCount;
  DWORD m_memUsage;
};

/*!
\ingroup textures
\brief 
*/
/************************************************************************/
/*                                                                      */
/************************************************************************/
class CGUITextureManager
{
public:
  CGUITextureManager(void);
  virtual ~CGUITextureManager(void);

  virtual int Load(const CStdString& strTextureName, bool checkBundleOnly = false)= 0;

  bool HasTexture(const CStdString &textureName, CStdString *path = NULL, int *bundle = NULL, int *size = NULL);
  bool CanLoad(const CStdString &texturePath) const; ///< Returns true if the texture manager can load this texture
  void ReleaseTexture(const CStdString& strTextureName);
  void Cleanup();
  void Dump() const;
  DWORD GetMemoryUsage() const;
  void Flush();
  CStdString GetTexturePath(const CStdString& textureName, bool directory = false);
  void GetBundledTexturesFromPath(const CStdString& texturePath, std::vector<CStdString> &items);
  void AddTexturePath(const CStdString &texturePath);    ///< Add a new path to the paths to check when loading media
  void SetTexturePath(const CStdString &texturePath);    ///< Set a single path as the path to check when loading media (clear then add)
  void RemoveTexturePath(const CStdString &texturePath); ///< Remove a path from the paths to check when loading media

protected:
  virtual const CTextureArray* GetTexture(const CStdString& strTextureName);
  std::vector<CTextureMap*> m_vecTextures;
  typedef std::vector<CTextureMap*>::iterator ivecTextures;
  // we have 2 texture bundles (one for the base textures, one for the theme)
  CTextureBundle m_TexBundle[2];
  std::vector<CStdString> m_texturePaths;
};

/*!
\ingroup textures
\brief 
*/
#endif
