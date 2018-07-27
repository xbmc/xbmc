/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
