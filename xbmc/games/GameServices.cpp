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

#include "GameServices.h"
#include "controllers/Controller.h"
#include "controllers/ControllerManager.h"
#include "games/GameSettings.h"
#include "profiles/ProfilesManager.h"

using namespace KODI;
using namespace GAME;

CGameServices::CGameServices(CControllerManager &controllerManager,
                             RETRO:: CGUIGameRenderManager &renderManager,
                             CSettings &settings,
                             PERIPHERALS::CPeripherals &peripheralManager,
                             const CProfilesManager &profileManager) :
  m_controllerManager(controllerManager),
  m_gameRenderManager(renderManager),
  m_profileManager(profileManager),
  m_gameSettings(new CGameSettings(settings))
{
}

CGameServices::~CGameServices() = default;

ControllerPtr CGameServices::GetController(const std::string& controllerId)
{
  return m_controllerManager.GetController(controllerId);
}

ControllerPtr CGameServices::GetDefaultController()
{
  return m_controllerManager.GetDefaultController();
}

ControllerPtr CGameServices::GetDefaultKeyboard()
{
  return m_controllerManager.GetDefaultKeyboard();
}

ControllerPtr CGameServices::GetDefaultMouse()
{
  return m_controllerManager.GetDefaultMouse();
}

ControllerVector CGameServices::GetControllers()
{
  return m_controllerManager.GetControllers();
}

std::string CGameServices::GetSavestatesFolder() const
{
  return m_profileManager.GetSavestatesFolder();
}
