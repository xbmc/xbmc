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

#include "GameServices.h"
#include "cores/RetroPlayer/rendering/GUIRenderSettings.h"
#include "controllers/Controller.h"
#include "controllers/ControllerManager.h"
#include "games/ports/PortManager.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

CGameServices::CGameServices(CControllerManager &controllerManager, PERIPHERALS::CPeripherals& peripheralManager) :
  m_controllerManager(controllerManager),
  m_portManager(new CPortManager(peripheralManager)),
  m_renderSettings(new RETRO::CGUIRenderSettings)
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

ControllerVector CGameServices::GetControllers()
{
  return m_controllerManager.GetControllers();
}

CPortManager& CGameServices::PortManager()
{
  return *m_portManager;
}
