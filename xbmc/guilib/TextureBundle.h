#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include "TextureBundleXBT.h"

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

private:
  CTextureBundleXBT m_tbXBT;

  bool m_useXBT;
};


