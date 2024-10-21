/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace PERIPHERALS
{
class CPeripheral;
}

namespace KODI
{
namespace GAME
{
class CControllerState;
}

namespace SMART_HOME
{
class ISmartHomeJoystickHandler;
/*!
 * \ingroup smarthome
 * \brief Handles game controller events for smart home functionality
 *
 * Listens to game controller events and forwards them to the smart home
 * messaging layer.
 */
class ISmartHomeJoystickHandler
{
public:
  virtual ~ISmartHomeJoystickHandler() = default;

  /*!
   * \brief Called after all input has been accounted for in a single frame
   *
   * \param peripheral The peripheral providing the input
   * \param controllerState The state of the controller at the end of the input frame
   */
  virtual void OnInputFrame(const PERIPHERALS::CPeripheral& peripheral,
                            const GAME::CControllerState& controllerState) = 0;
};
} // namespace SMART_HOME
} // namespace KODI
