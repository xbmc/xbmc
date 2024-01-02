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
#include "games/agents/input/AgentInput.h"
#include "profiles/ProfileManager.h"

using namespace KODI;
using namespace GAME;

CGameServices::CGameServices(CControllerManager& controllerManager,
                             RETRO::CGUIGameRenderManager& renderManager,
                             PERIPHERALS::CPeripherals& peripheralManager,
                             const CProfileManager& profileManager,
                             CInputManager& inputManager)
  : m_controllerManager(controllerManager),
    m_gameRenderManager(renderManager),
    m_profileManager(profileManager),
    m_gameSettings(new CGameSettings()),
    m_agentInput(std::make_unique<CAgentInput>(peripheralManager, inputManager))
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

std::string CGameServices::TranslateFeature(const std::string& controllerId,
                                            const std::string& featureName)
{
  return m_controllerManager.TranslateFeature(controllerId, featureName);
}

std::string CGameServices::GetSavestatesFolder() const
{
  return m_profileManager.GetSavestatesFolder();
}
