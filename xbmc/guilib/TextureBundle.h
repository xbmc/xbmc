/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureBundleXBT.h"

#include <string>
#include <vector>

class CTextureBundle
{
public:
  CTextureBundle();
  explicit CTextureBundle(bool useXBT);
  ~CTextureBundle() = default;

  void SetThemeBundle(bool themeBundle);
  bool HasFile(const std::string& Filename);
  void GetTexturesFromPath(const std::string &path, std::vector<std::string> &textures);
  static std::string Normalize(const std::string &name);

  bool LoadTexture(const std::string& Filename, CBaseTexture** ppTexture, int &width, int &height);

  int LoadAnim(const std::string& Filename, CBaseTexture*** ppTextures, int &width, int &height, int& nLoops, int** ppDelays);
  void Close();
private:
  CTextureBundleXBT m_tbXBT;

  bool m_useXBT;
};


