/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerManager.h"

#include "Controller.h"
#include "ControllerIDs.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "games/controllers/input/PhysicalFeature.h"

#include <mutex>

using namespace KODI;
using namespace GAME;

CControllerManager::CControllerManager(ADDON::CAddonMgr& addonManager)
  : m_addonManager(addonManager)
{
  m_addonManager.Events().Subscribe(this, &CControllerManager::OnEvent);
}

CControllerManager::~CControllerManager()
{
  m_addonManager.Events().Unsubscribe(this);
}

ControllerPtr CControllerManager::GetController(const std::string& controllerId)
{
  using namespace ADDON;

  std::lock_guard<CCriticalSection> lock(m_mutex);

  ControllerPtr& cachedController = m_cache[controllerId];

  if (!cachedController && m_failedControllers.find(controllerId) == m_failedControllers.end())
  {
    AddonPtr addon;
    if (m_addonManager.GetAddon(controllerId, addon, AddonType::GAME_CONTROLLER,
                                OnlyEnabled::CHOICE_NO))
      cachedController = LoadController(addon);
  }

  return cachedController;
}

ControllerPtr CControllerManager::GetDefaultController()
{
  return GetController(DEFAULT_CONTROLLER_ID);
}

ControllerPtr CControllerManager::GetDefaultKeyboard()
{
  return GetController(DEFAULT_KEYBOARD_ID);
}

ControllerPtr CControllerManager::GetDefaultMouse()
{
  return GetController(DEFAULT_MOUSE_ID);
}

ControllerVector CControllerManager::GetControllers()
{
  using namespace ADDON;

  ControllerVector controllers;

  std::lock_guard<CCriticalSection> lock(m_mutex);

  VECADDONS addons;
  if (m_addonManager.GetInstalledAddons(addons, AddonType::GAME_CONTROLLER))
  {
    for (auto& addon : addons)
    {
      ControllerPtr& cachedController = m_cache[addon->ID()];
      if (!cachedController && m_failedControllers.find(addon->ID()) == m_failedControllers.end())
        cachedController = LoadController(addon);

      if (cachedController)
        controllers.emplace_back(cachedController);
    }
  }

  return controllers;
}

std::string CControllerManager::TranslateFeature(const std::string& controllerId,
                                                 const std::string& featureName)
{
  ControllerPtr controller = GetController(controllerId);
  if (controller)
  {
    const std::vector<CPhysicalFeature>& features = controller->Features();

    auto it = std::find_if(features.begin(), features.end(),
                           [&featureName](const CPhysicalFeature& feature)
                           { return feature.Name() == featureName; });

    if (it != features.end())
      return it->Label();
  }

  return "";
}

void CControllerManager::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled))
  {
    std::lock_guard<CCriticalSection> lock(m_mutex);

    const std::string& addonId = event.addonId;

    // Clear caches for add-on
    auto it = m_cache.find(addonId);
    if (it != m_cache.end())
      m_cache.erase(it);

    auto it2 = m_failedControllers.find(addonId);
    if (it2 != m_failedControllers.end())
      m_failedControllers.erase(it2);
  }
}

ControllerPtr CControllerManager::LoadController(const ADDON::AddonPtr& addon)
{
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  if (!controller->LoadLayout())
  {
    m_failedControllers.insert(addon->ID());
    controller.reset();
  }

  return controller;
}
