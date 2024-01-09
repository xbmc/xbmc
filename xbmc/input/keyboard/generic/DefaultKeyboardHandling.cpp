/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DefaultKeyboardHandling.h"

#include "KeyboardInputHandling.h"
#include "games/controllers/input/DefaultButtonMap.h"
#include "input/keyboard/interfaces/IKeyboardInputHandler.h"

#include <memory>

using namespace KODI;
using namespace KEYBOARD;

CDefaultKeyboardHandling::CDefaultKeyboardHandling(PERIPHERALS::CPeripheral* peripheral,
                                                   IKeyboardInputHandler* handler)
  : m_peripheral(peripheral), m_inputHandler(handler)
{
}

CDefaultKeyboardHandling::~CDefaultKeyboardHandling(void)
{
  m_driverHandler.reset();
  m_buttonMap.reset();
}

bool CDefaultKeyboardHandling::Load()
{
  std::string controllerId;
  if (m_inputHandler != nullptr)
    controllerId = m_inputHandler->ControllerID();

  if (!controllerId.empty())
    m_buttonMap = std::make_unique<GAME::CDefaultButtonMap>(m_peripheral, std::move(controllerId));

  if (m_buttonMap && m_buttonMap->Load())
  {
    m_driverHandler = std::make_unique<CKeyboardInputHandling>(m_inputHandler, m_buttonMap.get());
    return true;
  }

  return false;
}

bool CDefaultKeyboardHandling::OnKeyPress(const CKey& key)
{
  if (m_driverHandler)
    return m_driverHandler->OnKeyPress(key);

  return false;
}

void CDefaultKeyboardHandling::OnKeyRelease(const CKey& key)
{
  if (m_driverHandler)
    m_driverHandler->OnKeyRelease(key);
}
