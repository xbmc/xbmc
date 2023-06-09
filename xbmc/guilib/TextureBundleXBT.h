/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Texture.h"

#include <cstdint>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class CXBTFReader;
class CXBTFFrame;

class CTextureBundleXBT
{
public:
  CTextureBundleXBT();
  explicit CTextureBundleXBT(bool themeBundle);
  ~CTextureBundleXBT();

  void SetThemeBundle(bool themeBundle);
  bool HasFile(const std::string& Filename);
  std::vector<std::string> GetTexturesFromPath(const std::string& path);
  static std::string Normalize(std::string name);

  struct Texture
  {
    std::unique_ptr<CTexture> texture;
    int width;
    int height;
  };

  /*!
   * \brief See CTextureBundle::LoadTexture
   */
  std::optional<Texture> LoadTexture(const std::string& filename);

  struct Animation
  {
    std::vector<std::pair<std::unique_ptr<CTexture>, int>> textures;
    int width;
    int height;
    int loops;
  };

  /*!
   * \brief See CTextureBundle::LoadAnim
   */
  std::optional<Animation> LoadAnim(const std::string& filename);

  //! @todo Change return to std::optional<std::vector<uint8_t>>> when c++17 is allowed
  static std::vector<uint8_t> UnpackFrame(const CXBTFReader& reader, const CXBTFFrame& frame);

  void CloseBundle();

private:
  bool OpenBundle();
  std::unique_ptr<CTexture> ConvertFrameToTexture(const std::string& name, const CXBTFFrame& frame);

  time_t m_TimeStamp;

  bool m_themeBundle;
  std::string m_path;
  std::shared_ptr<CXBTFReader> m_XBTFReader;
};


