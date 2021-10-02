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
   * \param[out] texture holds the pointer to the texture after successful loading
   * \param[out] width width of the loaded texture
   * \param[out] height height of the loaded texture
   * \return true if texture was loaded
   *
   * \todo With c++17 this should be changed to return a std::optional that's
   *       wrapping a struct containing the output values. Same for
   *       CTextureBundleXBT::LoadTexture.
   */
  bool LoadTexture(const std::string& filename,
                   std::unique_ptr<CTexture>& texture,
                   int& width,
                   int& height);

  /*!
   * \brief Load animation from bundle
   *
   * \param[in] filename name of the animation to load
   * \param[out] texture vector of frames. Each frame is pair of a texture and
   *                     the duration the frame
   * \param[out] width width of the loaded textures
   * \param[out] height height of the loaded textures
   * \return true if animation was loaded
   *
   * \todo With c++17 this should be changed to return a std::optional that's
   *       wrapping a struct containing the output values. Same for
   *       CTextureBundleXBT::LoadAnim.
   */
  bool LoadAnim(const std::string& filename,
                std::vector<std::pair<std::unique_ptr<CTexture>, int>>& textures,
                int& width,
                int& height,
                int& nLoops);
  void Close();
private:
  CTextureBundleXBT m_tbXBT;

  bool m_useXBT;
};


