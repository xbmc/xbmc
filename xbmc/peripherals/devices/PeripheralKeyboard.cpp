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

#include "PeripheralKeyboard.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"

#include <sstream>

using namespace KODI;
using namespace PERIPHERALS;

CPeripheralKeyboard::CPeripheralKeyboard(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(manager, scanResult, bus)
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

void CPeripheralKeyboard::RegisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler, bool bPromiscuous)
{
  CSingleLock lock(m_mutex);

  KeyboardHandle handle{ handler, bPromiscuous };
  m_keyboardHandlers.insert(m_keyboardHandlers.begin(), handle);
}

void CPeripheralKeyboard::UnregisterKeyboardDriverHandler(KODI::KEYBOARD::IKeyboardDriverHandler* handler)
{
  CSingleLock lock(m_mutex);

  auto it = std::find_if(m_keyboardHandlers.begin(), m_keyboardHandlers.end(),
    [handler](const KeyboardHandle &handle)
    {
      return handle.handler == handler;
    });

  if (it != m_keyboardHandlers.end())
    m_keyboardHandlers.erase(it);
}

bool CPeripheralKeyboard::OnKeyPress(const CKey& key)
{
  CSingleLock lock(m_mutex);

  bool bHandled = false;

  // Process promiscuous handlers
  for (const KeyboardHandle &handle : m_keyboardHandlers)
  {
    if (handle.bPromiscuous)
      handle.handler->OnKeyPress(key);
  }

  // Process handlers until one is handled
  for (const KeyboardHandle &handle : m_keyboardHandlers)
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
  CSingleLock lock(m_mutex);

  for (const KeyboardHandle &handle : m_keyboardHandlers)
    handle.handler->OnKeyRelease(key);
}
