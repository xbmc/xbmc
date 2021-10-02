/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIComponent.h"
#include "TextureBundle.h"
#include "threads/CriticalSection.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CTexture;

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

  void Add(std::shared_ptr<CTexture> texture, int delay);
  void Set(std::shared_ptr<CTexture> texture, int width, int height);
  void Free();
  unsigned int size() const;

  std::vector<std::shared_ptr<CTexture>> m_textures;
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

  void Add(std::unique_ptr<CTexture> texture, int delay);
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
  std::vector<std::string> GetBundledTexturesFromPath(const std::string& texturePath);

  void AddTexturePath(const std::string &texturePath);    ///< Add a new path to the paths to check when loading media
  void SetTexturePath(const std::string &texturePath);    ///< Set a single path as the path to check when loading media (clear then add)
  void RemoveTexturePath(const std::string &texturePath); ///< Remove a path from the paths to check when loading media

  void FreeUnusedTextures(unsigned int timeDelay = 0); ///< Free textures (called from app thread only)
  void ReleaseHwTexture(unsigned int texture);
protected:
  std::vector<CTextureMap*> m_vecTextures;
  std::list<std::pair<CTextureMap*, std::chrono::time_point<std::chrono::steady_clock>>>
      m_unusedTextures;
  std::vector<unsigned int> m_unusedHwTextures;
  typedef std::vector<CTextureMap*>::iterator ivecTextures;
  // we have 2 texture bundles (one for the base textures, one for the theme)
  CTextureBundle m_TexBundle[2];

  std::vector<std::string> m_texturePaths;
  CCriticalSection m_section;
};

