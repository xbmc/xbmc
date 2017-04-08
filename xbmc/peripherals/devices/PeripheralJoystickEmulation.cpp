/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "PeripheralJoystickEmulation.h"
#include "input/keyboard/generic/JoystickEmulation.h"
#include "input/InputManager.h"
#include "threads/SingleLock.h"

#include <sstream>

using namespace KODI;
using namespace PERIPHERALS;

CPeripheralJoystickEmulation::CPeripheralJoystickEmulation(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(manager, scanResult, bus)
{
  m_features.push_back(FEATURE_JOYSTICK);
}

CPeripheralJoystickEmulation::~CPeripheralJoystickEmulation(void)
{
  CInputManager::GetInstance().UnregisterKeyboardHandler(this);
}

bool CPeripheralJoystickEmulation::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    if (feature == FEATURE_JOYSTICK)
    {
      CInputManager::GetInstance().RegisterKeyboardHandler(this);
    }
    bSuccess = true;
  }

  return bSuccess;
}

void CPeripheralJoystickEmulation::RegisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler, bool bPromiscuous)
{
  using namespace KEYBOARD;

  CSingleLock lock(m_mutex);

  if (m_keyboardHandlers.find(handler) == m_keyboardHandlers.end())
    m_keyboardHandlers[handler] = KeyboardHandle{ new CJoystickEmulation(handler), bPromiscuous };
}

void CPeripheralJoystickEmulation::UnregisterJoystickDriverHandler(JOYSTICK::IDriverHandler* handler)
{
  CSingleLock lock(m_mutex);

  KeyboardHandlers::iterator it = m_keyboardHandlers.find(handler);
  if (it != m_keyboardHandlers.end())
  {
    delete it->second.handler;
    m_keyboardHandlers.erase(it);
  }
}

bool CPeripheralJoystickEmulation::OnKeyPress(const CKey& key)
{
  CSingleLock lock(m_mutex);

  bool bHandled = false;

  // Process promiscuous handlers
  for (KeyboardHandlers::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
  {
    if (it->second.bPromiscuous)
      it->second.handler->OnKeyPress(key);
  }

  // Process handlers until one is handled
  for (KeyboardHandlers::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
  {
    if (!it->second.bPromiscuous)
    {
      bHandled = it->second.handler->OnKeyPress(key);
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

void CPeripheralJoystickEmulation::OnKeyRelease(const CKey& key)
{
  CSingleLock lock(m_mutex);

  for (KeyboardHandlers::iterator it = m_keyboardHandlers.begin(); it != m_keyboardHandlers.end(); ++it)
    it->second.handler->OnKeyRelease(key);
}

unsigned int CPeripheralJoystickEmulation::ControllerNumber(void) const
{
  unsigned int number;
  std::istringstream str(m_strLocation);
  str >> number;
  return number;
}
