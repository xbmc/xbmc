/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameServices.h"

#include "controllers/Controller.h"
#include "controllers/ControllerManager.h"
#include "games/GameSettings.h"
#include "profiles/ProfileManager.h"

using namespace KODI;
using namespace GAME;

CGameServices::CGameServices(CControllerManager& controllerManager,
                             RETRO::CGUIGameRenderManager& renderManager,
                             PERIPHERALS::CPeripherals& peripheralManager,
                             const CProfileManager& profileManager)
  : m_controllerManager(controllerManager),
    m_gameRenderManager(renderManager),
    m_profileManager(profileManager),
    m_gameSettings(new CGameSettings())
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
