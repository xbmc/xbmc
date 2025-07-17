/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetFactory.h"

#include "IShaderPresetLoader.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/ShaderPreset.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <string>

using namespace KODI::SHADER;

CShaderPresetFactory::CShaderPresetFactory(ADDON::CAddonMgr& addons) : m_addons(addons)
{
  UpdateAddons();

  m_addons.Events().Subscribe(this,
                              [this](const ADDON::AddonEvent& event)
                              {
                                if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
                                    typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
                                    typeid(event) == typeid(ADDON::AddonEvents::UnInstalled) ||
                                    typeid(event) == typeid(ADDON::AddonEvents::ReInstalled))
                                {
                                  UpdateAddons();
                                }
                              });
}

CShaderPresetFactory::~CShaderPresetFactory()
{
  m_addons.Events().Unsubscribe(this);
}

void CShaderPresetFactory::RegisterLoader(IShaderPresetLoader* loader, const std::string& extension)
{
  if (!extension.empty())
  {
    std::string strExtension = extension;

    // Canonicalize extension with leading "."
    if (extension[0] != '.')
      strExtension.insert(strExtension.begin(), '.');

    m_loaders.insert(std::make_pair(std::move(strExtension), loader));
  }
}

void CShaderPresetFactory::UnregisterLoader(IShaderPresetLoader* loader)
{
  for (auto it = m_loaders.begin(); it != m_loaders.end();)
  {
    if (it->second == loader)
      m_loaders.erase(it++);
    else
      ++it;
  }
}

bool CShaderPresetFactory::HasAddons() const
{
  return !m_shaderAddons.empty() || !m_failedAddons.empty();
}

bool CShaderPresetFactory::LoadPreset(const std::string& presetPath, IShaderPreset& shaderPreset)
{
  bool bSuccess = false;

  std::string extension = URIUtils::GetExtension(presetPath);
  if (!extension.empty())
  {
    auto itLoader = m_loaders.find(extension);
    if (itLoader != m_loaders.end())
      bSuccess = itLoader->second->LoadPreset(presetPath, shaderPreset);
  }

  return bSuccess;
}

bool CShaderPresetFactory::CanLoadPreset(const std::string& presetPath)
{
  bool bSuccess = false;

  std::string extension = URIUtils::GetExtension(presetPath);
  if (!extension.empty())
    bSuccess = (m_loaders.contains(extension));

  return bSuccess;
}

void CShaderPresetFactory::UpdateAddons()
{
  using namespace ADDON;

  std::vector<AddonInfoPtr> addonInfo;
  m_addons.GetAddonInfos(addonInfo, true, AddonType::SHADERDLL);

  // Look for removed/disabled add-ons
  auto oldAddons = std::move(m_shaderAddons);
  for (auto it = oldAddons.begin(); it != oldAddons.end(); ++it)
  {
    const std::string& addonId = it->first;
    std::unique_ptr<ADDON::CShaderPresetAddon>& shaderAddon = it->second;

    const bool bIsDisabled =
        std::find_if(addonInfo.begin(), addonInfo.end(), [&addonId](const AddonInfoPtr& addon)
                     { return addonId == addon->ID(); }) == addonInfo.end();

    if (bIsDisabled)
      UnregisterLoader(shaderAddon.get());
    else
      m_shaderAddons.emplace(addonId, std::move(shaderAddon));
  }

  // Look for new add-ons
  for (const AddonInfoPtr& shaderAddon : addonInfo)
  {
    std::string addonId = shaderAddon->ID();

    const bool bIsNew = (!m_shaderAddons.contains(addonId));
    if (!bIsNew)
      continue;

    const bool bIsFailed = (m_failedAddons.contains(addonId));
    if (bIsFailed)
      continue;

    std::unique_ptr<CShaderPresetAddon> addonPtr =
        std::make_unique<CShaderPresetAddon>(shaderAddon);

    if (addonPtr->CreateAddon())
    {
      for (const auto& extension : addonPtr->GetExtensions())
        RegisterLoader(addonPtr.get(), extension);
      m_shaderAddons.emplace(std::move(addonId), std::move(addonPtr));
    }
    else
    {
      m_failedAddons.try_emplace(std::move(addonId), std::move(addonPtr));
    }
  }
}
