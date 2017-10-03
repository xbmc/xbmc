/*
 *      Copyright (C) 2017-2018 Team Kodi
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

#include "PeripheralMouse.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"

#include <sstream>

using namespace KODI;
using namespace PERIPHERALS;

CPeripheralMouse::CPeripheralMouse(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(manager, scanResult, bus)
{
  // Initialize CPeripheral
  m_features.push_back(FEATURE_MOUSE);
}

CPeripheralMouse::~CPeripheralMouse(void)
{
  m_manager.GetInputManager().UnregisterMouseDriverHandler(this);
}

bool CPeripheralMouse::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    if (feature == FEATURE_MOUSE)
    {
      m_manager.GetInputManager().RegisterMouseDriverHandler(this);
    }

    bSuccess = true;
  }

  return bSuccess;
}

void CPeripheralMouse::RegisterMouseDriverHandler(MOUSE::IMouseDriverHandler* handler, bool bPromiscuous)
{
  using namespace KEYBOARD;

  CSingleLock lock(m_mutex);

  MouseHandle handle{ handler, bPromiscuous };
  m_mouseHandlers.insert(m_mouseHandlers.begin(), handle);
}

void CPeripheralMouse::UnregisterMouseDriverHandler(MOUSE::IMouseDriverHandler* handler)
{
  CSingleLock lock(m_mutex);

  auto it = std::find_if(m_mouseHandlers.begin(), m_mouseHandlers.end(),
    [handler](const MouseHandle &handle)
    {
      return handle.handler == handler;
    });

  if (it != m_mouseHandlers.end())
    m_mouseHandlers.erase(it);
}

bool CPeripheralMouse::OnPosition(int x, int y)
{
  CSingleLock lock(m_mutex);

  bool bHandled = false;

  // Process promiscuous handlers
  for (const MouseHandle &handle : m_mouseHandlers)
  {
    if (handle.bPromiscuous)
      handle.handler->OnPosition(x, y);
  }

  // Process handlers until one is handled
  for (const MouseHandle &handle : m_mouseHandlers)
  {
    if (!handle.bPromiscuous)
    {
      bHandled = handle.handler->OnPosition(x, y);
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

bool CPeripheralMouse::OnButtonPress(MOUSE::BUTTON_ID button)
{
  CSingleLock lock(m_mutex);

  bool bHandled = false;

  // Process promiscuous handlers
  for (const MouseHandle &handle : m_mouseHandlers)
  {
    if (handle.bPromiscuous)
      handle.handler->OnButtonPress(button);
  }

  // Process handlers until one is handled
  for (const MouseHandle &handle : m_mouseHandlers)
  {
    if (!handle.bPromiscuous)
    {
      bHandled = handle.handler->OnButtonPress(button);
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

void CPeripheralMouse::OnButtonRelease(MOUSE::BUTTON_ID button)
{
  CSingleLock lock(m_mutex);

  for (const MouseHandle &handle : m_mouseHandlers)
    handle.handler->OnButtonRelease(button);
}
