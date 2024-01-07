/*
*  Copyright (C) 2024 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/keyboard/interfaces/IKeyboardInputHandler.h"
#include "peripherals/PeripheralTypes.h"

namespace KODI
{
namespace GAME
{
class CControllerActivity;

/*!
 * \ingroup games
 *
 * \brief Handles keyboard events for game agent functionality
 */
class CAgentKeyboard : public KEYBOARD::IKeyboardInputHandler
{
public:
  CAgentKeyboard(PERIPHERALS::PeripheralPtr peripheral);

  ~CAgentKeyboard() override;

  void Initialize();
  void Deinitialize();
  void ClearButtonState();

  // Input parameters
  float GetActivation() const;
  ControllerPtr Appearance() const { return m_controllerAppearance; }

  // Implementation of IKeyboardInputHandler
  std::string ControllerID() const override;
  bool HasKey(const KEYBOARD::KeyName& key) const override;
  bool OnKeyPress(const KEYBOARD::KeyName& key, KEYBOARD::Modifier mod, uint32_t unicode) override;
  void OnKeyRelease(const KEYBOARD::KeyName& key,
                    KEYBOARD::Modifier mod,
                    uint32_t unicode) override;

private:
  // Construction parameters
  const PERIPHERALS::PeripheralPtr m_peripheral;

  // Input state
  std::unique_ptr<CControllerActivity> m_keyboardActivity;
  ControllerPtr m_controllerAppearance;
};
} // namespace GAME
} // namespace KODI
