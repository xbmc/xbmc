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
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"

using namespace KODI;
using namespace GAME;

CControllerManager::CControllerManager(ADDON::CAddonMgr& addonManager)
  : m_addonManager(addonManager)
{
}

ControllerPtr CControllerManager::GetController(const std::string& controllerId)
{
  using namespace ADDON;

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
