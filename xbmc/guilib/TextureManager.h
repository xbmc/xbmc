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
#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "Texture.h"
#include "TextureBundleXBT.h"
#include "threads/CriticalSection.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/
class CTextureArray
{
public:
  CTextureArray(int width, int height, int loops, bool texCoordsArePixels = false);
  CTextureArray();

  virtual ~CTextureArray() = default;

  void Reset();

  void Add(CBaseTexture *texture, int delay);
  void Set(CBaseTexture *texture, int width, int height);
  void Free();
  std::size_t size() const;

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
  void SetHeight(int height);
  void SetWidth(int height);
protected:
  void FreeTexture();

  CTextureArray m_texture;
  std::string m_textureName;
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
  CGUITextureManager();
  ~CGUITextureManager();

  bool HasTexture(const std::string &textureName, std::string *path = nullptr, int *bundle = nullptr, int *size = nullptr);
  static bool CanLoad(const std::string &texturePath);
  ///< Returns true if the texture manager can load this texture
  const CTextureArray& Load(const std::string& strTextureName, bool checkBundleOnly = false);
  void ReleaseTexture(const std::string& strTextureName, bool immediately = false);
  void Cleanup();
  void Dump() const;
  uint32_t GetMemoryUsage() const;
  void Flush();
  std::string GetTexturePath(const std::string& textureName, bool directory = false);
  void GetBundledTexturesFromPath(const std::string& texturePath, std::vector<std::string> &items) const;

  void AddTexturePath(const std::string &texturePath);    ///< Add a new path to the paths to check when loading media
  void SetTexturePath(const std::string &texturePath);    ///< Set a single path as the path to check when loading media (clear then add)
  void RemoveTexturePath(const std::string &texturePath); ///< Remove a path from the paths to check when loading media

  void FreeUnusedTextures(unsigned int timeDelay = 0); ///< Free textures (called from app thread only)
  void ReleaseHwTexture(unsigned int texture);
protected:
  const CTextureArray* GetTextureGif(CTextureBundleXBT& bundle, const std::string& strTextureName);
  const CTextureArray* GetTextureGifOrPng(CTextureBundleXBT& bundle, const std::string& strTextureName, std::string strPath);
  const CTextureArray* GetTexture(CTextureBundleXBT& bundle, const std::string& strTextureName, std::string strPath);
  const CTextureArray* LoadInternal(CTextureBundleXBT& bundle, std::string textureName, std::string path);
  void LoadAll(CTextureBundleXBT& bundle);

  std::unordered_map<std::string, std::unique_ptr<CTextureMap>> m_vecTextures;
  // we have 2 texture bundles (one for the base textures, one for the theme)
  CTextureBundleXBT m_TexBundle[2];
  bool m_initialized{ false };

  std::vector<std::string> m_texturePaths;
  CCriticalSection m_section;
};