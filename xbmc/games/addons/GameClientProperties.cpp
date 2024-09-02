/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientProperties.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GameClient.h"
#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/GameResource.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <cstring>

using namespace KODI;
using namespace ADDON;
using namespace GAME;
using namespace XFILE;

CGameClientProperties::CGameClientProperties(const CGameClient& parent, AddonProps_Game& props)
  : m_parent(parent), m_properties(props)
{
}

void CGameClientProperties::ReleaseResources(void)
{
  for (auto& it : m_proxyDllPaths)
    delete[] it;
  m_proxyDllPaths.clear();

  for (auto& it : m_resourceDirectories)
    delete[] it;
  m_resourceDirectories.clear();

  for (auto& it : m_extensions)
    delete[] it;
  m_extensions.clear();
}

bool CGameClientProperties::InitializeProperties(void)
{
  ReleaseResources();

  ADDON::VECADDONS addons;
  if (!GetProxyAddons(addons))
    return false;

  m_properties.game_client_dll_path = GetLibraryPath();
  m_properties.proxy_dll_paths = GetProxyDllPaths(addons);
  m_properties.proxy_dll_count = GetProxyDllCount();
  m_properties.resource_directories = GetResourceDirectories();
  m_properties.resource_directory_count = GetResourceDirectoryCount();
  m_properties.profile_directory = GetProfileDirectory();
  m_properties.supports_vfs = m_parent.SupportsVFS();
  m_properties.extensions = GetExtensions();
  m_properties.extension_count = GetExtensionCount();

  return true;
}

const char* CGameClientProperties::GetLibraryPath(void)
{
  if (m_strLibraryPath.empty())
  {
    // Get the parent add-on's real path
    std::string strLibPath = m_parent.CAddonDll::LibPath();
    m_strLibraryPath = CSpecialProtocol::TranslatePath(strLibPath);
    URIUtils::RemoveSlashAtEnd(m_strLibraryPath);
  }
  return m_strLibraryPath.c_str();
}

const char** CGameClientProperties::GetProxyDllPaths(const ADDON::VECADDONS& addons)
{
  if (m_proxyDllPaths.empty())
  {
    for (const auto& addon : addons)
      AddProxyDll(std::static_pointer_cast<CGameClient>(addon));
  }

  if (!m_proxyDllPaths.empty())
    return const_cast<const char**>(m_proxyDllPaths.data());

  return nullptr;
}

unsigned int CGameClientProperties::GetProxyDllCount(void) const
{
  return static_cast<unsigned int>(m_proxyDllPaths.size());
}

const char** CGameClientProperties::GetResourceDirectories(void)
{
  if (m_resourceDirectories.empty())
  {
    // Add all other game resources
    const auto& dependencies = m_parent.GetDependencies();
    for (auto it = dependencies.begin(); it != dependencies.end(); ++it)
    {
      const std::string& strAddonId = it->id;
      AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(strAddonId, addon, AddonType::RESOURCE_GAMES,
                                                 OnlyEnabled::CHOICE_YES))
      {
        std::shared_ptr<CGameResource> resource = std::static_pointer_cast<CGameResource>(addon);

        std::string resourcePath = resource->GetFullPath("");
        URIUtils::RemoveSlashAtEnd(resourcePath);

        char* resourceDir = new char[resourcePath.length() + 1];
        std::strcpy(resourceDir, resourcePath.c_str());
        m_resourceDirectories.push_back(resourceDir);
      }
    }

    // Add resource directories for profile and path
    std::string addonProfile = CSpecialProtocol::TranslatePath(m_parent.Profile());
    std::string addonPath = m_parent.Path();

    addonProfile = URIUtils::AddFileToFolder(addonProfile, GAME_CLIENT_RESOURCES_DIRECTORY);
    addonPath = URIUtils::AddFileToFolder(addonPath, GAME_CLIENT_RESOURCES_DIRECTORY);

    if (!CDirectory::Exists(addonProfile))
    {
      CLog::Log(LOGDEBUG, "Creating resource directory: {}", addonProfile);
      CDirectory::Create(addonProfile);
    }

    // Only add user profile directory if non-empty
    CFileItemList items;
    if (CDirectory::GetDirectory(addonProfile, items, "", DIR_FLAG_DEFAULTS))
    {
      if (!items.IsEmpty())
      {
        char* addonProfileDir = new char[addonProfile.length() + 1];
        std::strcpy(addonProfileDir, addonProfile.c_str());
        m_resourceDirectories.push_back(addonProfileDir);
      }
    }

    char* addonPathDir = new char[addonPath.length() + 1];
    std::strcpy(addonPathDir, addonPath.c_str());
    m_resourceDirectories.push_back(addonPathDir);
  }

  if (!m_resourceDirectories.empty())
    return const_cast<const char**>(m_resourceDirectories.data());

  return nullptr;
}

unsigned int CGameClientProperties::GetResourceDirectoryCount(void) const
{
  return static_cast<unsigned int>(m_resourceDirectories.size());
}

const char* CGameClientProperties::GetProfileDirectory(void)
{
  if (m_strProfileDirectory.empty())
  {
    m_strProfileDirectory = CSpecialProtocol::TranslatePath(m_parent.Profile());
    URIUtils::RemoveSlashAtEnd(m_strProfileDirectory);
  }

  return m_strProfileDirectory.c_str();
}

const char** CGameClientProperties::GetExtensions(void)
{
  for (auto& extension : m_parent.GetExtensions())
  {
    char* ext = new char[extension.length() + 1];
    std::strcpy(ext, extension.c_str());
    m_extensions.push_back(ext);
  }

  return !m_extensions.empty() ? const_cast<const char**>(m_extensions.data()) : nullptr;
}

unsigned int CGameClientProperties::GetExtensionCount(void) const
{
  return static_cast<unsigned int>(m_extensions.size());
}

bool CGameClientProperties::GetProxyAddons(ADDON::VECADDONS& addons)
{
  ADDON::VECADDONS ret;
  std::vector<std::string> disabledDependencies; // ID or name of disabled dependencies
  std::vector<std::string> uninstalledDependencies; // ID or name of uninstalled dependencies

  for (const auto& dependency : m_parent.GetDependencies())
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(dependency.id, addon, OnlyEnabled::CHOICE_NO))
    {
      // If add-on is disabled, ask the user to enable it
      if (CServiceBroker::GetAddonMgr().IsAddonDisabled(dependency.id))
      {
        // "Failed to play game"
        // "This game depends on a disabled add-on. Would you like to enable it?"
        if (CGUIDialogYesNo::ShowAndGetInput(CVariant{35210}, CVariant{35215}))
        {
          if (!CServiceBroker::GetAddonMgr().EnableAddon(dependency.id))
          {
            CLog::Log(LOGERROR, "Failed to enable add-on {}", dependency.id);
            disabledDependencies.emplace_back(addon->Name());
            addon.reset();
          }
        }
        else
        {
          CLog::Log(LOGERROR, "User chose to not enable add-on {}", dependency.id);
          disabledDependencies.emplace_back(addon->Name());
          addon.reset();
        }
      }

      if (addon && addon->Type() == AddonType::GAMEDLL)
        ret.emplace_back(std::move(addon));
    }
    else
    {
      if (dependency.optional)
      {
        CLog::Log(LOGDEBUG, "Missing optional dependency {}", dependency.id);
      }
      else
      {
        CLog::Log(LOGERROR, "Missing mandatory dependency {}", dependency.id);
        uninstalledDependencies.emplace_back(dependency.id);
      }
    }
  }

  // Attempt to install missing dependencies
  if (disabledDependencies.empty() && !uninstalledDependencies.empty())
  {
    if (InstallDependencies(uninstalledDependencies))
      uninstalledDependencies.clear();
  }

  // IDs of all missing dependencies
  std::vector<std::string> missingDependencies;

  // Reserve enough space to avoid multiple allocations
  missingDependencies.reserve(disabledDependencies.size() + uninstalledDependencies.size());

  // Move elements from the first vector
  missingDependencies.insert(missingDependencies.end(),
                             std::make_move_iterator(disabledDependencies.begin()),
                             std::make_move_iterator(disabledDependencies.end()));

  // Move elements from the second vector
  missingDependencies.insert(missingDependencies.end(),
                             std::make_move_iterator(uninstalledDependencies.begin()),
                             std::make_move_iterator(uninstalledDependencies.end()));

  // Show an error dialog if any dependencies are still missing
  if (!missingDependencies.empty())
  {
    std::string strDependencies = StringUtils::Join(missingDependencies, ", ");
    std::string dialogText = StringUtils::Format(g_localizeStrings.Get(35223), strDependencies);

    // "Failed to play game"
    // "Add-on is incompatible due to unmet dependencies."
    // ""
    // "Missing: {0:s}"
    MESSAGING::HELPERS::ShowOKDialogLines(CVariant{35210}, CVariant{24104}, CVariant{""},
                                          CVariant{dialogText});

    return false;
  }

  addons = std::move(ret);
  return true;
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
  for (const auto& it : m_proxyDllPaths)
  {
    if (strLibPath == it)
      return true;
  }
  return false;
}

bool CGameClientProperties::InstallDependencies(const std::vector<std::string>& addons)
{
  // Only proceed if we have a GUI
  CGUIComponent* const gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return false;

  const CGUIWindowManager& windowManager = gui->GetWindowManager();

  // Get GUI dialogs
  auto* selectDialog = windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  auto* progressDialog = windowManager.GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);

  // We can only install add-ons if the dialogs are present
  if (selectDialog == nullptr || progressDialog == nullptr)
    return false;

  // Get installable add-ons
  VECADDONS installableAddons;
  for (const std::string& dependency : addons)
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().FindInstallableById(dependency, addon) && addon)
      installableAddons.emplace_back(std::move(addon));
    else
      CLog::Log(LOGERROR, "Failed to find installable add-on for {}", dependency);
  }

  // Verify all add-ons are installable
  if (addons.size() != installableAddons.size())
    return false;

  // Setup select dialog
  selectDialog->Reset();
  selectDialog->SetHeading(39020); // "The following additional add-ons will be installed"
  selectDialog->SetUseDetails(true);
  selectDialog->EnableButton(true, 186); // "OK""
  selectDialog->SetButtonFocus(true);
  for (const auto& addon : installableAddons)
  {
    CFileItem item{addon->Name()};
    item.SetArt("icon", addon->Icon());
    selectDialog->Add(item);
  }

  // Show select dialog
  selectDialog->Open();
  if (!selectDialog->IsButtonPressed())
  {
    CLog::Log(LOGDEBUG, "Dependency installer: User cancelled installation dialog");
    return false;
  }

  CLog::Log(LOGDEBUG, "Dependency installer: Installing {} add-ons", installableAddons.size());

  progressDialog->SetHeading(CVariant{24086}); // "Installing add-on..."
  progressDialog->SetLine(0, CVariant{""});
  progressDialog->SetLine(1, CVariant{""});
  progressDialog->SetLine(2, CVariant{""});

  progressDialog->Open();

  size_t installedCount = 0;
  while (installedCount < installableAddons.size())
  {
    const AddonPtr& addon = installableAddons.at(installedCount);

    // Set dialog text
    const std::string& progressTemplate = g_localizeStrings.Get(24057); // "Installing {0:s}..."
    const std::string progressText = StringUtils::Format(progressTemplate, addon->Name());
    progressDialog->SetLine(0, CVariant{progressText});

    // Set dialog percentage
    const unsigned int percentage =
        100 * (installedCount + 1) / static_cast<unsigned int>(installableAddons.size());
    progressDialog->SetPercentage(percentage);

    if (!ADDON::CAddonInstaller::GetInstance().InstallOrUpdate(
            addon->ID(), ADDON::BackgroundJob::CHOICE_NO, ADDON::ModalJob::CHOICE_NO))
    {
      CLog::Log(LOGERROR, "Controller installer: Failed to install {}", addon->ID());
      // "Error"
      // "Failed to install add-on."
      MESSAGING::HELPERS::ShowOKDialogText(257, 35256);
      return false;
    }

    if (progressDialog->IsCanceled())
    {
      CLog::Log(LOGDEBUG, "Controller installer: User cancelled add-on installation");
      return false;
    }

    if (windowManager.GetActiveWindowOrDialog() != WINDOW_DIALOG_PROGRESS)
    {
      CLog::Log(LOGDEBUG, "Controller installer: Progress dialog is hidden, canceling");
      return false;
    }

    installedCount++;
  }

  CLog::Log(LOGDEBUG, "Controller window: Installed {} controller add-ons", installedCount);
  progressDialog->Close();

  return true;
}
