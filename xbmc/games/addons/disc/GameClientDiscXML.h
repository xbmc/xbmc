/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscPlaylist.h"

#include <string>
#include <vector>

class CXBMCTinyXML2;

namespace tinyxml2
{
class XMLElement;
}

namespace KODI
{
namespace GAME
{

class CGameClientDiscXML : public CGameClientDiscPlaylist
{
public:
  static std::string GetXMLPath(const std::string& gamePath);

  static bool Load(const std::string& gamePath, CGameClientDiscModel& model);
  static bool Save(const std::string& gamePath, const CGameClientDiscModel& model);

private:
  static std::vector<GameClientDiscEntry> ReadSlotsFromXML(const tinyxml2::XMLElement* rootElement);
  static void WriteSlotsToXML(CXBMCTinyXML2& xmlDoc,
                              tinyxml2::XMLElement* rootElement,
                              const CGameClientDiscModel& model);

  static void ReadTrayFromXML(const tinyxml2::XMLElement* rootElement, CGameClientDiscModel& model);
  static void WriteTrayToXML(CXBMCTinyXML2& xmlDoc,
                             tinyxml2::XMLElement* rootElement,
                             const CGameClientDiscModel& model);

  static void ReadSelectedFromXML(const tinyxml2::XMLElement* rootElement,
                                  CGameClientDiscModel& model);
  static void WriteSelectedToXML(CXBMCTinyXML2& xmlDoc,
                                 tinyxml2::XMLElement* rootElement,
                                 const CGameClientDiscModel& model);
};

} // namespace GAME
} // namespace KODI
