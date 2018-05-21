/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
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
#pragma once

#include "controllers/ControllerTypes.h"

#include <memory>
#include <string>

class CProfilesManager;
class CSettings;

namespace PERIPHERALS
{
  class CPeripherals;
}

namespace KODI
{
namespace RETRO
{
  class CGUIGameRenderManager;
}

namespace GAME
{
  class CControllerManager;
  class CGameSettings;

  class CGameServices
  {
  public:
    CGameServices(CControllerManager &controllerManager,
                  RETRO::CGUIGameRenderManager &renderManager,
                  CSettings &settings,
                  PERIPHERALS::CPeripherals &peripheralManager,
                  const CProfilesManager &profileManager);
    ~CGameServices();

    ControllerPtr GetController(const std::string& controllerId);
    ControllerPtr GetDefaultController();
    ControllerPtr GetDefaultKeyboard();
    ControllerPtr GetDefaultMouse();
    ControllerVector GetControllers();

    std::string GetSavestatesFolder() const;

    CGameSettings& GameSettings() { return *m_gameSettings; }

    RETRO::CGUIGameRenderManager &GameRenderManager() { return m_gameRenderManager; }

  private:
    // Construction parameters
    CControllerManager &m_controllerManager;
    RETRO::CGUIGameRenderManager &m_gameRenderManager;
    const CProfilesManager &m_profileManager;

    // Game services
    std::unique_ptr<CGameSettings> m_gameSettings;
  };
}
}
