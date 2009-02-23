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

#ifdef HAS_SDL_OPENGL
class CGLTexture
{
public:
  CGLTexture(SDL_Surface* surface, bool loadToGPU = true, bool freeSurface = false);  
  ~CGLTexture();

  void LoadToGPU();
  void Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU); 
  void Update(SDL_Surface *surface, bool loadToGPU, bool freeSurface);

  int imageWidth;
  int imageHeight;
  int textureWidth;
  int textureHeight;
  GLuint id;
private:
  unsigned char* m_pixels;
  bool m_loadedToGPU;
};
#endif

// currently just used as a transport from texture manager to rest of app
class CBaseTexture
{
public:
  CBaseTexture()
  {
    Reset();
  };
  void Reset()
  {
    m_texture = NULL;
    m_palette = NULL;
    m_width = 0;
    m_height = 0;
    m_texWidth = 0;
    m_texHeight = 0;
    m_texCoordsArePixels = false;
  };
#if !defined(HAS_SDL)
  CBaseTexture(LPDIRECT3DTEXTURE8 texture, int width, int height, LPDIRECT3DPALETTE8 palette = NULL, bool texCoordsArePixels = false);
#elif defined(HAS_SDL_2D)
  CBaseTexture(SDL_Surface* texture, int width, int height, SDL_Palette* palette = NULL, bool texCoordsArePixels = false);
#else
  CBaseTexture(CGLTexture* texture, int width, int height, SDL_Palette* palette = NULL, bool texCoordsArePixels = false);
#endif

#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 m_texture;
  LPDIRECT3DPALETTE8 m_palette;
#elif defined(HAS_SDL_2D)
  SDL_Surface* m_pexture;
  SDL_Palette* m_palette;
#elif defined(HAS_SDL_OPENGL)
  CGLTexture* m_texture;
  SDL_Palette* m_palette;  
#endif
  int m_width;
  int m_height;
  int m_texWidth;
  int m_texHeight;
  bool m_texCoordsArePixels;
};

/*!
 \ingroup textures
 \brief 
 */
class CTexture
{
public:
  CTexture();
#ifndef HAS_SDL
  CTexture(LPDIRECT3DTEXTURE8 pTexture, int iWidth, int iHeight, bool bPacked, int iDelay = 100, LPDIRECT3DPALETTE8 pPalette = NULL);
#else
  CTexture(SDL_Surface* pTexture, int iWidth, int iHeight, bool bPacked, int iDelay = 100, SDL_Palette* pPalette = NULL);
#endif
  virtual ~CTexture();
  bool Release();
  CBaseTexture GetTexture();
  int GetDelay() const;
  int GetRef() const;
  void Dump() const;
  void ReadTextureInfo();
  DWORD GetMemoryUsage() const;
  void SetDelay(int iDelay);
  void Flush();
  void SetLoops(int iLoops);
  int GetLoops() const;
protected:
  void FreeTexture();

#ifndef HAS_SDL
  LPDIRECT3DTEXTURE8 m_pTexture;
  LPDIRECT3DPALETTE8 m_pPalette;
#elif defined(HAS_SDL_2D)
  SDL_Surface* m_pTexture;
  SDL_Palette* m_pPalette;
#elif defined(HAS_SDL_OPENGL)
  CGLTexture* m_pTexture;
  SDL_Palette* m_pPalette;  
#endif
  int m_iReferenceCount;
  int m_iDelay;
  int m_iWidth;
  int m_iHeight;
  int m_iLoops;
  bool m_bPacked;
  D3DFORMAT m_format;
  DWORD m_memUsage;
};

/*!
 \ingroup textures
 \brief 
 */
class CTextureMap
{
public:
  CTextureMap();
  CTextureMap(const CStdString& strTextureName);
  virtual ~CTextureMap();
  const CStdString& GetName() const;
  int size() const;
  CBaseTexture GetTexture(int iPicture);
  int GetDelay(int iPicture = 0) const;
  int GetLoops(int iPicture = 0) const;
  void Add(CTexture* pTexture);
  bool Release(int iPicture = 0);
  bool IsEmpty() const;
  void Dump() const;
  DWORD GetMemoryUsage() const;
  void Flush();
protected:
  CStdString m_strTextureName;
  std::vector<CTexture*> m_vecTexures;
  typedef std::vector<CTexture*>::iterator ivecTextures;
};

/*!
 \ingroup textures
 \brief 
 */
class CGUITextureManager
{
public:
  CGUITextureManager(void);
  virtual ~CGUITextureManager(void);

  void StartPreLoad();
  void PreLoad(const CStdString& strTextureName);
  void EndPreLoad();
  void FlushPreLoad();
  bool HasTexture(const CStdString &textureName, CStdString *path = NULL, int *bundle = NULL, int *size = NULL);
  bool CanLoad(const CStdString &texturePath) const; ///< Returns true if the texture manager can load this texture
  int Load(const CStdString& strTextureName, DWORD dwColorKey = 0, bool checkBundleOnly = false);
  CBaseTexture GetTexture(const CStdString& strTextureName, int iItem);
  int GetDelay(const CStdString& strTextureName, int iPicture = 0) const;
  int GetLoops(const CStdString& strTextureName, int iPicture = 0) const;
  void ReleaseTexture(const CStdString& strTextureName, int iPicture = 0);
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
  std::vector<CTextureMap*> m_vecTextures;
  typedef std::vector<CTextureMap*>::iterator ivecTextures;
  // we have 2 texture bundles (one for the base textures, one for the theme)
  CTextureBundle m_TexBundle[2];
  std::list<CStdString> m_PreLoadNames[2];
  std::list<CStdString>::iterator m_iNextPreload[2];

  std::vector<CStdString> m_texturePaths;
};

/*!
 \ingroup textures
 \brief 
 */
extern CGUITextureManager g_TextureManager;
#endif
