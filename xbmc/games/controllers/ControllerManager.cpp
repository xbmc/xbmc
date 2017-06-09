/*
 *      Copyright (C) 2017 Team Kodi
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

#include "ControllerManager.h"
#include "Controller.h"
#include "addons/AddonManager.h"
#include "input/joysticks/JoystickIDs.h"

using namespace KODI;
using namespace GAME;

ControllerPtr CControllerManager::GetController(const std::string& controllerId)
{
  using namespace ADDON;

  ControllerPtr& cachedController = m_cache[controllerId];

  if (!cachedController && m_failedControllers.find(controllerId) == m_failedControllers.end())
  {
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(controllerId, addon, ADDON_GAME_CONTROLLER, false))
      cachedController = LoadController(std::move(addon));
  }

  return cachedController;
}

ControllerPtr CControllerManager::GetDefaultController()
{
  return GetController(DEFAULT_CONTROLLER_ID);
}

ControllerVector CControllerManager::GetControllers()
{
  using namespace ADDON;

  ControllerVector controllers;

  VECADDONS addons;
  if (CAddonMgr::GetInstance().GetInstalledAddons(addons, ADDON_GAME_CONTROLLER))
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
