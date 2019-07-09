/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/GameClientSubsystem.h"
#include "games/controllers/ControllerTypes.h"
#include "games/controllers/types/ControllerTree.h"
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
  class CGameClientController;
  class CGameClientHardware;
  class CGameClientJoystick;
  class CGameClientKeyboard;
  class CGameClientMouse;
  class CGameClientTopology;
  class IGameInputCallback;

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

    void Start(IGameInputCallback *input);
    void Stop();

    // Input functions
    bool HasFeature(const std::string &controllerId, const std::string &featureName) const;
    bool AcceptsInput() const;
    bool InputEvent(const game_input_event &event);

    // Topology functions
    const CControllerTree &GetControllerTree() const;
    bool SupportsKeyboard() const;
    bool SupportsMouse() const;

    // Agent functions
    bool HasAgent() const;

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
    void SetControllerLayouts(const ControllerVector &controllers);
    void ProcessJoysticks();
    PortMap MapJoysticks(const PERIPHERALS::PeripheralVector &peripheralJoysticks,
                         const JoystickMap &gameClientjoysticks) const;

    // Private callback helpers
    bool SetRumble(const std::string &portAddress, const std::string& feature, float magnitude);

    // Helper functions
    static ControllerVector GetControllers(const CGameClient &gameClient);
    static void ActivateControllers(CControllerHub &hub);

    // Input properties
    IGameInputCallback *m_inputCallback = nullptr;
    std::unique_ptr<CGameClientTopology> m_topology;
    using ControllerLayoutMap = std::map<std::string, std::unique_ptr<CGameClientController>>;
    ControllerLayoutMap m_controllerLayouts;
    JoystickMap m_joysticks;
    PortMap m_portMap;
    std::unique_ptr<CGameClientKeyboard> m_keyboard;
    std::unique_ptr<CGameClientMouse> m_mouse;
    std::unique_ptr<CGameClientHardware> m_hardware;
  };
} // namespace GAME
} // namespace KODI
