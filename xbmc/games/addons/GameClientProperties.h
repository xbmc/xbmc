/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/GameTypes.h"

#include <string>
#include <vector>

struct AddonProps_Game;

namespace KODI
{
namespace GAME
{

class CGameClient;

/**
 * \ingroup games
 * \brief C++ wrapper for game client properties declared in kodi_game_types.h
 */
class CGameClientProperties
{
public:
  CGameClientProperties(const CGameClient* parent, AddonProps_Game& props);
  ~CGameClientProperties(void) { ReleaseResources(); }

  void InitializeProperties(void);

private:
  // Release mutable resources
  void ReleaseResources(void);

  // Equal to parent's real library path
  const char* GetLibraryPath(void);

  // List of proxy DLLs needed to load the game client
  const char** GetProxyDllPaths(void);

  // Number of proxy DLLs needed to load the game client
  unsigned int GetProxyDllCount(void) const { return m_proxyDllPaths.size(); }

  // Paths to game resources
  const char** GetResourceDirectories(void);

  // Number of resource directories
  unsigned int GetResourceDirectoryCount(void) const { return m_resourceDirectories.size(); }

  // Equal to special://profile/addon_data/<parent's id>
  const char* GetProfileDirectory(void);

  // List of extensions from addon.xml
  const char** GetExtensions(void);

  // Number of extensions
  unsigned int GetExtensionCount(void) const { return m_extensions.size(); }

  // Helper functions
  void AddProxyDll(const GameClientPtr& gameClient);
  bool HasProxyDll(const std::string& strLibPath) const;

  const CGameClient* const  m_parent;
  AddonProps_Game&          m_properties;

  // Buffers to hold the strings
  std::string        m_strLibraryPath;
  std::vector<char*> m_proxyDllPaths;
  std::vector<char*> m_resourceDirectories;
  std::string        m_strProfileDirectory;
  std::vector<char*> m_extensions;
};

} // namespace GAME
} // namespace KODI
