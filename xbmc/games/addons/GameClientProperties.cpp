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

#include "GameClientProperties.h"
#include "GameClient.h"
#include "ServiceBroker.h"
#include "addons/IAddon.h"
#include "addons/AddonManager.h"
#include "addons/GameResource.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/Variant.h"

#include <cstring>

using namespace KODI;
using namespace ADDON;
using namespace GAME;
using namespace XFILE;

#define GAME_CLIENT_RESOURCES_DIRECTORY  "resources"

CGameClientProperties::CGameClientProperties(const CGameClient* parent, AddonProps_Game& props)
  : m_parent(parent),
    m_properties(props)
{
}

void CGameClientProperties::ReleaseResources(void)
{
  for (std::vector<char*>::const_iterator it = m_proxyDllPaths.begin(); it != m_proxyDllPaths.end(); ++it)
    delete[] *it;
  m_proxyDllPaths.clear();

  for (std::vector<char*>::const_iterator it = m_resourceDirectories.begin(); it != m_resourceDirectories.end(); ++it)
    delete[] *it;
  m_resourceDirectories.clear();

  for (std::vector<char*>::const_iterator it = m_extensions.begin(); it != m_extensions.end(); ++it)
    delete[] *it;
  m_extensions.clear();
}

void CGameClientProperties::InitializeProperties(void)
{
  ReleaseResources();

  m_properties.game_client_dll_path     = GetLibraryPath();
  m_properties.proxy_dll_paths          = GetProxyDllPaths();
  m_properties.proxy_dll_count          = GetProxyDllCount();
  m_properties.resource_directories     = GetResourceDirectories();
  m_properties.resource_directory_count = GetResourceDirectoryCount();
  m_properties.profile_directory        = GetProfileDirectory();
  m_properties.supports_vfs             = m_parent->SupportsVFS();
  m_properties.extensions               = GetExtensions();
  m_properties.extension_count          = GetExtensionCount();
}

const char* CGameClientProperties::GetLibraryPath(void)
{
  if (m_strLibraryPath.empty())
  {
    // Get the parent add-on's real path
    std::string strLibPath = m_parent->CAddonDll::LibPath();
    m_strLibraryPath = CSpecialProtocol::TranslatePath(strLibPath);
  }
  return m_strLibraryPath.c_str();
}

const char** CGameClientProperties::GetProxyDllPaths(void)
{
  if (m_proxyDllPaths.empty())
  {
    // Add all game client dependencies
    //! @todo Compare helper version with required dependency
    const ADDONDEPS& dependencies = m_parent->GetDeps();
    for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
    {
      const std::string& strAddonId = it->first;
      AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(strAddonId, addon, ADDON_GAMEDLL, false))
      {
        // If add-on is disabled, ask the user to enable it
        if (CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID()))
        {
          // Failed to play game
          // This game depends on a disabled add-on. Would you like to enable it?
          if (CGUIDialogYesNo::ShowAndGetInput(CVariant{ 35210 }, CVariant{ 35215 }))
            CServiceBroker::GetAddonMgr().EnableAddon(addon->ID());
          else
            addon.reset();
        }
      }

      if (addon)
        AddProxyDll(std::static_pointer_cast<CGameClient>(addon));
    }
  }

  if (!m_proxyDllPaths.empty())
    return const_cast<const char**>(m_proxyDllPaths.data());

  return nullptr;
}

const char** CGameClientProperties::GetResourceDirectories(void)
{
  if (m_resourceDirectories.empty())
  {
    // Add all other game resources
    const ADDONDEPS& dependencies = m_parent->GetDeps();
    for (ADDONDEPS::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
    {
      const std::string& strAddonId = it->first;
      AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(strAddonId, addon, ADDON_RESOURCE_GAMES))
      {
        std::shared_ptr<CGameResource> resource = std::static_pointer_cast<CGameResource>(addon);

        std::string resourcePath = resource->GetFullPath("");

        char* resourceDir = new char[resourcePath.length() + 1];
        std::strcpy(resourceDir, resourcePath.c_str());
        m_resourceDirectories.push_back(resourceDir);
      }
    }

    // Add resource directories for profile and path
    std::string addonProfile = CSpecialProtocol::TranslatePath(m_parent->Profile());
    std::string addonPath = m_parent->Path();

    addonProfile = URIUtils::AddFileToFolder(addonProfile, GAME_CLIENT_RESOURCES_DIRECTORY);
    addonPath = URIUtils::AddFileToFolder(addonPath, GAME_CLIENT_RESOURCES_DIRECTORY);

    if (!CDirectory::Exists(addonProfile))
    {
      CLog::Log(LOGDEBUG, "Creating resource directory: %s", addonProfile.c_str());
      CDirectory::Create(addonProfile);
    }

    char* addonProfileDir = new char[addonProfile.length() + 1];
    std::strcpy(addonProfileDir, addonProfile.c_str());
    m_resourceDirectories.push_back(addonProfileDir);

    char* addonPathDir = new char[addonPath.length() + 1];
    std::strcpy(addonPathDir, addonPath.c_str());
    m_resourceDirectories.push_back(addonPathDir);
  }

  if (!m_resourceDirectories.empty())
    return const_cast<const char**>(m_resourceDirectories.data());

  return nullptr;
}

const char* CGameClientProperties::GetProfileDirectory(void)
{
  if (m_strProfileDirectory.empty())
    m_strProfileDirectory = CSpecialProtocol::TranslatePath(m_parent->Profile());

  return m_strProfileDirectory.c_str();
}

const char** CGameClientProperties::GetExtensions(void)
{
  for (auto& extension : m_parent->GetExtensions())
  {
    char* ext = new char[extension.length() + 1];
    std::strcpy(ext, extension.c_str());
    m_extensions.push_back(ext);
  }

  return !m_extensions.empty() ? const_cast<const char**>(m_extensions.data()) : nullptr;
}

void CGameClientProperties::AddProxyDll(const GameClientPtr& gameClient)
{
  // Get the add-on's real path
  std::string strLibPath = gameClient->CAddon::LibPath();

  // Ignore add-on if it is already added
  if (!HasProxyDll(strLibPath))
  {
    char* libPath = new char[strLibPath.length() + 1];
    std::strcpy(libPath, strLibPath.c_str());
    m_proxyDllPaths.push_back(libPath);
  }
}

bool CGameClientProperties::HasProxyDll(const std::string& strLibPath) const
{
  for (std::vector<char*>::const_iterator it = m_proxyDllPaths.begin(); it != m_proxyDllPaths.end(); ++it)
  {
    if (strLibPath == *it)
      return true;
  }
  return false;
}
