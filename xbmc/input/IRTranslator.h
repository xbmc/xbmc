/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <map>
#include <memory>
#include <string>

class TiXmlNode;

class CIRTranslator
{
public:
  CIRTranslator();

  /*!
   * \brief Loads Lircmap.xml/IRSSmap.xml
   */
  void Load(const std::string &irMapName);

  /*!
   * \brief Clears the map
   */
  void Clear();

  unsigned int TranslateButton(const std::string &szDevice, const std::string &szButton);

  static uint32_t TranslateString(std::string strButton);
  static uint32_t TranslateUniversalRemoteString(const std::string &szButton);

private:
  bool LoadIRMap(const std::string &irMapPath);
  void MapRemote(TiXmlNode *pRemote, const std::string &szDevice);

  using IRButtonMap = std::map<std::string, std::string>;

  std::map<std::string, std::shared_ptr<IRButtonMap>> m_irRemotesMap;
};
