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
#pragma once

#include "controllers/ControllerTypes.h"

#include <memory>
#include <string>

namespace PERIPHERALS
{
  class CPeripherals;
}

namespace KODI
{
namespace GAME
{
  class CControllerManager;
  class CPortManager;

  class CGameServices
  {
  public:
    CGameServices();
    ~CGameServices();

    void Init(PERIPHERALS::CPeripherals& peripheralManager);
    void Deinit();

    ControllerPtr GetController(const std::string& controllerId);
    ControllerPtr GetDefaultController();
    ControllerVector GetControllers();

    CPortManager& PortManager();

  private:
    std::unique_ptr<CControllerManager> m_controllerManager;
    std::unique_ptr<CPortManager> m_portManager;
  };
}
}
