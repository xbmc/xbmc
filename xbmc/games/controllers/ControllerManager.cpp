/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "ControllerManager.h"
#include "Controller.h"
#include "ControllerIDs.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"

using namespace KODI;
using namespace GAME;

ControllerPtr CControllerManager::GetController(const std::string& controllerId)
{
  using namespace ADDON;

  ControllerPtr& cachedController = m_cache[controllerId];

  if (!cachedController && m_failedControllers.find(controllerId) == m_failedControllers.end())
  {
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(controllerId, addon, ADDON_GAME_CONTROLLER, false))
      cachedController = LoadController(std::move(addon));
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
  if (CServiceBroker::GetAddonMgr().GetInstalledAddons(addons, ADDON_GAME_CONTROLLER))
  {
    for (auto& addon : addons)
    {
      ControllerPtr& cachedController = m_cache[addon->ID()];
      if (!cachedController && m_failedControllers.find(addon->ID()) == m_failedControllers.end())
        cachedController = LoadController(std::move(addon));

      if (cachedController)
        controllers.emplace_back(cachedController);
    }
  }

  return controllers;
}

ControllerPtr CControllerManager::LoadController(ADDON::AddonPtr addon)
{
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  if (!controller->LoadLayout())
  {
    m_failedControllers.insert(addon->ID());
    controller.reset();
  }

  return controller;
}
