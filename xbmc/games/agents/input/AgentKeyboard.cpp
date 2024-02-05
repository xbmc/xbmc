/*
*  Copyright (C) 2024 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "AgentKeyboard.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/input/ControllerActivity.h"
#include "input/keyboard/interfaces/IKeyboardInputProvider.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CAgentKeyboard::CAgentKeyboard(PERIPHERALS::PeripheralPtr peripheral)
  : m_peripheral(std::move(peripheral)), m_keyboardActivity(std::make_unique<CControllerActivity>())
{
}

CAgentKeyboard::~CAgentKeyboard() = default;

void CAgentKeyboard::Initialize()
{
  // Record appearance to detect changes
  m_controllerAppearance = m_peripheral->ControllerProfile();

  // Upcast peripheral to input provider interface
  KEYBOARD::IKeyboardInputProvider* inputProvider = m_peripheral.get();

  // Register input handler to silently observe all input
  inputProvider->RegisterKeyboardHandler(this, true, true);
}

void CAgentKeyboard::Deinitialize()
{
  // Upcast peripheral to input interface
  KEYBOARD::IKeyboardInputProvider* inputProvider = m_peripheral.get();

  // Unregister input handler
  inputProvider->UnregisterKeyboardHandler(this);

  // Reset appearance
  m_controllerAppearance.reset();
}

void CAgentKeyboard::ClearButtonState()
{
  return m_keyboardActivity->ClearButtonState();
}

float CAgentKeyboard::GetActivation() const
{
  return m_keyboardActivity->GetActivation();
}

std::string CAgentKeyboard::ControllerID(void) const
{
  return DEFAULT_KEYBOARD_ID;
}

bool CAgentKeyboard::HasKey(const KEYBOARD::KeyName& key) const
{
  return true; // Capture all keys
}

bool CAgentKeyboard::OnKeyPress(const KEYBOARD::KeyName& key,
                                KEYBOARD::Modifier mod,
                                uint32_t unicode)
{
  m_keyboardActivity->OnKeyPress(key);
  m_keyboardActivity->OnInputFrame();

  return true;
}

void CAgentKeyboard::OnKeyRelease(const KEYBOARD::KeyName& key,
                                  KEYBOARD::Modifier mod,
                                  uint32_t unicode)
{
  m_keyboardActivity->OnKeyRelease(key);
  m_keyboardActivity->OnInputFrame();
}
