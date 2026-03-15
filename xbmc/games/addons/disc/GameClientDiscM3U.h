/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/disc/GameClientDiscPlaylist.h"

#include <string>

namespace KODI
{
namespace GAME
{

class CGameClientDiscModel;
struct GameClientDiscEntry;

class CGameClientDiscM3U : protected CGameClientDiscPlaylist
{
public:
  static std::string GetM3UPath(const std::string& gamePath);
  static bool Save(const std::string& gamePath, const CGameClientDiscModel& model);

private:
  static std::string BuildM3U(const CGameClientDiscModel& model);
  static void AppendDiscToM3U(std::string& m3u, const GameClientDiscEntry& disc);
  static std::string NormalizeDiscPath(const std::string& discPath);
};

} // namespace GAME
} // namespace KODI
