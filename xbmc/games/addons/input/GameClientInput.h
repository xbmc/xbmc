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

#include "games/addons/GameClientSubsystem.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/controllers/ControllerTypes.h"
#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <map>
#include <memory>
#include <string>

class CCriticalSection;
struct game_input_event;

namespace KODI
{
namespace JOYSTICK
{
  class IInputProvider;
}

namespace GAME
{
  class CGameClient;
  class CGameClientHardware;
  class CGameClientJoystick;
  class CGameClientKeyboard;
  class CGameClientMouse;

  class CGameClientInput : protected CGameClientSubsystem,
                           public Observer
  {
  public:
    CGameClientInput(CGameClient &gameClient,
                     AddonInstance_Game &addonStruct,
                     CCriticalSection &clientAccess);
    ~CGameClientInput() override;

    void Initialize();
    void Deinitialize();

    void Start();
    void Stop();

    // Input functions
    bool AcceptsInput() const;

    // Topology functions
    const CControllerTree &GetControllerTree() const { return m_controllers; }
    bool SupportsKeyboard() const;
    bool SupportsMouse() const;

    // Keyboard functions
    bool OpenKeyboard(const ControllerPtr &controller);
    void CloseKeyboard();

    // Mouse functions
    bool OpenMouse(const ControllerPtr &controller);
    void CloseMouse();

    // Joystick functions
    bool OpenJoystick(const std::string &portAddress, const ControllerPtr &controller);
    void CloseJoystick(const std::string &portAddress);

    // Hardware input functions
    void HardwareReset();

    // Input callbacks
    bool ReceiveInputEvent(const game_input_event& eventStruct);

    // Implementation of Observer
    void Notify(const Observable& obs, const ObservableMessage msg) override;

  private:
    using PortAddress = std::string;
    using JoystickMap = std::map<PortAddress, std::unique_ptr<CGameClientJoystick>>;
    using PortMap = std::map<JOYSTICK::IInputProvider*, CGameClientJoystick*>;

    // Private input helpers
    void LoadTopology();
    void ProcessJoysticks();
    PortMap MapJoysticks(const PERIPHERALS::PeripheralVector &peripheralJoysticks,
                         const JoystickMap &gameClientjoysticks) const;

    // Private callback helpers
    bool SetRumble(const std::string &portAddress, const std::string& feature, float magnitude);

    // Helper functions
    static ControllerVector GetControllers(const CGameClient &gameClient);

    // Input properties
    CControllerTree m_controllers;
    JoystickMap m_joysticks;
    PortMap m_portMap;
    std::unique_ptr<CGameClientKeyboard> m_keyboard;
    std::unique_ptr<CGameClientMouse> m_mouse;
    std::unique_ptr<CGameClientHardware> m_hardware;
    int m_playerLimit = -1; // No limit
  };
} // namespace GAME
} // namespace KODI
