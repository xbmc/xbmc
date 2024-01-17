/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralKeyboard.h"

#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerManager.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"

#include <mutex>
#include <sstream>

using namespace KODI;
using namespace PERIPHERALS;

CPeripheralKeyboard::CPeripheralKeyboard(CPeripherals& manager,
                                         const PeripheralScanResult& scanResult,
                                         CPeripheralBus* bus)
  : CPeripheral(manager, scanResult, bus)
{
  // Initialize CPeripheral
  m_features.push_back(FEATURE_KEYBOARD);
}

CPeripheralKeyboard::~CPeripheralKeyboard(void)
{
  m_manager.GetInputManager().UnregisterKeyboardDriverHandler(this);
}

bool CPeripheralKeyboard::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    switch (feature)
    {
      case FEATURE_KEYBOARD:
      {
        m_manager.GetInputManager().RegisterKeyboardDriverHandler(this);
        break;
      }
      default:
        break;
    }

    bSuccess = true;
  }

  return bSuccess;
}

void CPeripheralKeyboard::RegisterKeyboardDriverHandler(
    KODI::KEYBOARD::IKeyboardDriverHandler* handler, bool bPromiscuous)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  KeyboardHandle handle{handler, bPromiscuous};
  m_keyboardHandlers.insert(m_keyboardHandlers.begin(), handle);
}

void CPeripheralKeyboard::UnregisterKeyboardDriverHandler(
    KODI::KEYBOARD::IKeyboardDriverHandler* handler)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  auto it =
      std::find_if(m_keyboardHandlers.begin(), m_keyboardHandlers.end(),
                   [handler](const KeyboardHandle& handle) { return handle.handler == handler; });

  if (it != m_keyboardHandlers.end())
    m_keyboardHandlers.erase(it);
}

GAME::ControllerPtr CPeripheralKeyboard::ControllerProfile() const
{
  if (m_controllerProfile)
    return m_controllerProfile;

  return m_manager.GetControllerProfiles().GetDefaultKeyboard();
}

bool CPeripheralKeyboard::OnKeyPress(const CKey& key)
{
  m_lastActive = CDateTime::GetCurrentDateTime();

  std::unique_lock<CCriticalSection> lock(m_mutex);

  bool bHandled = false;

  // Process promiscuous handlers
  for (const KeyboardHandle& handle : m_keyboardHandlers)
  {
    if (handle.bPromiscuous)
      handle.handler->OnKeyPress(key);
  }

  // Process handlers until one is handled
  for (const KeyboardHandle& handle : m_keyboardHandlers)
  {
    if (!handle.bPromiscuous)
    {
      bHandled = handle.handler->OnKeyPress(key);
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

void CPeripheralKeyboard::OnKeyRelease(const CKey& key)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  for (const KeyboardHandle& handle : m_keyboardHandlers)
    handle.handler->OnKeyRelease(key);
}
