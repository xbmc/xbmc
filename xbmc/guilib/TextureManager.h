/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/*!
\file TextureManager.h
\brief
*/

#ifndef GUILIB_TEXTUREMANAGER_H
#define GUILIB_TEXTUREMANAGER_H

#include <vector>
#include <list>
#include "TextureBundle.h"
#include "threads/CriticalSection.h"

#pragma once

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

  void Add(CBaseTexture *texture, int delay);
  void Set(CBaseTexture *texture, int width, int height);
  void Free();
  unsigned int size() const;

  std::vector<CBaseTexture* > m_textures;
  std::vector<int> m_delays;
  int m_width;
  int m_height;
  int m_orientation;
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
  CTextureMap(const std::string& textureName, int width, int height, int loops);
  virtual ~CTextureMap();

  void Add(CBaseTexture* texture, int delay);
  bool Release();

  const std::string& GetName() const;
  const CTextureArray& GetTexture();
  void Dump() const;
  uint32_t GetMemoryUsage() const;
  void Flush();
  bool IsEmpty() const;
protected:
  void FreeTexture();

  std::string m_textureName;
  CTextureArray m_texture;
  unsigned int m_referenceCount;
  uint32_t m_memUsage;
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

  bool HasTexture(const std::string &textureName, std::string *path = NULL, int *bundle = NULL, int *size = NULL);
  static bool CanLoad(const std::string &texturePath); ///< Returns true if the texture manager can load this texture
  const CTextureArray& Load(const std::string& strTextureName, bool checkBundleOnly = false);
  void ReleaseTexture(const std::string& strTextureName, bool immediately = false);
  void Cleanup();
  void Dump() const;
  uint32_t GetMemoryUsage() const;
  void Flush();
  std::string GetTexturePath(const std::string& textureName, bool directory = false);
  void GetBundledTexturesFromPath(const std::string& texturePath, std::vector<std::string> &items);

  void AddTexturePath(const std::string &texturePath);    ///< Add a new path to the paths to check when loading media
  void SetTexturePath(const std::string &texturePath);    ///< Set a single path as the path to check when loading media (clear then add)
  void RemoveTexturePath(const std::string &texturePath); ///< Remove a path from the paths to check when loading media

  void FreeUnusedTextures(unsigned int timeDelay = 0); ///< Free textures (called from app thread only)
  void ReleaseHwTexture(unsigned int texture);
protected:
  std::vector<CTextureMap*> m_vecTextures;
  std::list<std::pair<CTextureMap*, unsigned int> > m_unusedTextures;
  std::vector<unsigned int> m_unusedHwTextures;
  typedef std::vector<CTextureMap*>::iterator ivecTextures;
  typedef std::list<std::pair<CTextureMap*, unsigned int> >::iterator ilistUnused;
  // we have 2 texture bundles (one for the base textures, one for the theme)
  CTextureBundle m_TexBundle[2];

  std::vector<std::string> m_texturePaths;
  CCriticalSection m_section;
};

/*!
 \ingroup textures
 \brief
 */
extern CGUITextureManager g_TextureManager;
#endif
