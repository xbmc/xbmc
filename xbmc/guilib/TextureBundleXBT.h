/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ctime>
#include <map>
#include <memory>
#include <string>
#include <vector>

class CBaseTexture;
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
  void GetTexturesFromPath(const std::string &path, std::vector<std::string> &textures);
  static std::string Normalize(const std::string &name);

  bool LoadTexture(const std::string& Filename, CBaseTexture** ppTexture,
                       int &width, int &height);

  int LoadAnim(const std::string& Filename, CBaseTexture*** ppTextures,
                int &width, int &height, int& nLoops, int** ppDelays);

  static uint8_t* UnpackFrame(const CXBTFReader& reader, const CXBTFFrame& frame);
  
  void CloseBundle();

private:
  bool OpenBundle();
  bool ConvertFrameToTexture(const std::string& name, CXBTFFrame& frame, CBaseTexture** ppTexture);

  time_t m_TimeStamp;

  bool m_themeBundle;
  std::string m_path;
  std::shared_ptr<CXBTFReader> m_XBTFReader;
};


