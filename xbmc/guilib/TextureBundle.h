/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureBundleXBT.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CTexture;

class CTextureBundle
{
public:
  CTextureBundle();
  explicit CTextureBundle(bool useXBT);
  ~CTextureBundle() = default;

  void SetThemeBundle(bool themeBundle);
  bool HasFile(const std::string& Filename);
  std::vector<std::string> GetTexturesFromPath(const std::string& path);
  static std::string Normalize(std::string name);

  /*!
   * \brief Load texture from bundle
   *
   * \param[in] filename name of the texture to load
   * \return std::optional<CTextureBundleXBT::Texture> if texture was loaded
   */
  std::optional<CTextureBundleXBT::Texture> LoadTexture(const std::string& filename);

  /*!
   * \brief Load animation from bundle
   *
   * \param[in] filename name of the animation to load
   * \return std::optional<CTextureBundleXBT::Animation> if animation was loaded
   */
  std::optional<CTextureBundleXBT::Animation> LoadAnim(const std::string& filename);

  void Close();
private:
  CTextureBundleXBT m_tbXBT;

  bool m_useXBT;
};


