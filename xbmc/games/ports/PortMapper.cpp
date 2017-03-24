/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "PortMapper.h"
#include "PortManager.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/Peripherals.h"

using namespace KODI;
using namespace GAME;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CPortMapper::CPortMapper(PERIPHERALS::CPeripherals& peripheralManager) :
  m_peripheralManager(peripheralManager)
{
  CPortManager::GetInstance().RegisterObserver(this);
}

CPortMapper::~CPortMapper()
{
  CPortManager::GetInstance().UnregisterObserver(this);
}

void CPortMapper::Notify(const Observable &obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessagePeripheralsChanged:
    case ObservableMessagePortsChanged:
      ProcessPeripherals();
      break;
    default:
      break;
  }
}

void CPortMapper::ProcessPeripherals()
{
  auto& oldPortMap = m_portMap;

  PeripheralVector devices;
  m_peripheralManager.GetPeripheralsWithFeature(devices, FEATURE_JOYSTICK);

  std::map<PeripheralPtr, IInputHandler*> newPortMap;
  CPortManager::GetInstance().MapDevices(devices, newPortMap);

  for (auto& device : devices)
  {
    std::map<PeripheralPtr, IInputHandler*>::const_iterator itOld = oldPortMap.find(device);
    std::map<PeripheralPtr, IInputHandler*>::const_iterator itNew = newPortMap.find(device);

    IInputHandler* oldHandler = itOld != oldPortMap.end() ? itOld->second : NULL;
    IInputHandler* newHandler = itNew != newPortMap.end() ? itNew->second : NULL;

    if (oldHandler != newHandler)
    {
      // Unregister old handler
      if (oldHandler != NULL)
        device->UnregisterJoystickInputHandler(oldHandler);

      // Register new handler
      if (newHandler != NULL)
        device->RegisterJoystickInputHandler(newHandler);
    }
  }

  oldPortMap.swap(newPortMap);
}
