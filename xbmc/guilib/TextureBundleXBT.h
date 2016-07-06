#pragma once

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

private:
  bool OpenBundle();
  bool ConvertFrameToTexture(const std::string& name, CXBTFFrame& frame, CBaseTexture** ppTexture);

  time_t m_TimeStamp;

  bool m_themeBundle;
  std::string m_path;
  std::shared_ptr<CXBTFReader> m_XBTFReader;
};


