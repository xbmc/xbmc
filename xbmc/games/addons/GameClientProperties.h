/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/GameTypes.h"

#include <string>
#include <vector>

struct AddonProps_Game;

namespace ADDON
{
class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;
using VECADDONS = std::vector<AddonPtr>;
} // namespace ADDON

namespace KODI
{
namespace GAME
{

class CGameClient;

/**
 * \ingroup games
 *
 * \brief C++ wrapper for properties to pass to the DLL
 *
 * Game client properties declared in addon-instance/Game.h.
 */
class CGameClientProperties
{
public:
  CGameClientProperties(const CGameClient& parent, AddonProps_Game& props);
  ~CGameClientProperties(void) { ReleaseResources(); }

  bool InitializeProperties(void);

private:
  // Release mutable resources
  void ReleaseResources(void);

  // Equal to parent's real library path
  const char* GetLibraryPath(void);

  // List of proxy DLLs needed to load the game client
  const char** GetProxyDllPaths(const ADDON::VECADDONS& addons);

  // Number of proxy DLLs needed to load the game client
  unsigned int GetProxyDllCount(void) const;

  // Paths to game resources
  const char** GetResourceDirectories(void);

  // Number of resource directories
  unsigned int GetResourceDirectoryCount(void) const;

  // Equal to special://profile/addon_data/<parent's id>
  const char* GetProfileDirectory(void);

  // List of extensions from addon.xml
  const char** GetExtensions(void);

  // Number of extensions
  unsigned int GetExtensionCount(void) const;

  // Helper functions
  bool GetProxyAddons(ADDON::VECADDONS& addons);
  void AddProxyDll(const GameClientPtr& gameClient);
  bool HasProxyDll(const std::string& strLibPath) const;

  // Construction parameters
  const CGameClient& m_parent;
  AddonProps_Game& m_properties;

  // Buffers to hold the strings
  std::string m_strLibraryPath;
  std::vector<char*> m_proxyDllPaths;
  std::vector<char*> m_resourceDirectories;
  std::string m_strProfileDirectory;
  std::vector<char*> m_extensions;
};

} // namespace GAME
} // namespace KODI
