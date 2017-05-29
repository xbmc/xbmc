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

#include "PortManager.h"
#include "PortMapper.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "peripherals/devices/PeripheralJoystickEmulation.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;
using namespace JOYSTICK;
using namespace PERIPHERALS;

// --- GetRequestedPort() -----------------------------------------------------

namespace GAME
{
  int GetRequestedPort(const PERIPHERALS::PeripheralPtr& device)
  {
    if (device->Type() == PERIPHERAL_JOYSTICK)
      return std::static_pointer_cast<CPeripheralJoystick>(device)->RequestedPort();
    return JOYSTICK_PORT_UNKNOWN;
  }
}

// --- CPortManager -----------------------------------------------------------

CPortManager::CPortManager() :
  m_portMapper(new CPortMapper)
{
}

CPortManager::~CPortManager()
{
  Deinitialize();
}

void CPortManager::Initialize(CPeripherals& peripheralManager)
{
  m_portMapper->Initialize(peripheralManager, *this);
}

void CPortManager::Deinitialize()
{
  m_portMapper->Deinitialize();
}

void CPortManager::OpenPort(IInputHandler* handler,
                            unsigned int port,
                            PERIPHERALS::PeripheralType requiredType /* = PERIPHERALS::PERIPHERAL_UNKNOWN) */)
{
  CSingleLock lock(m_mutex);

  SPort newPort = { };
  newPort.handler = handler;
  newPort.port = port;
  newPort.requiredType = requiredType;
  m_ports.push_back(newPort);

  SetChanged();
  NotifyObservers(ObservableMessagePortsChanged);
}

void CPortManager::ClosePort(IInputHandler* handler)
{
  CSingleLock lock(m_mutex);

  m_ports.erase(std::remove_if(m_ports.begin(), m_ports.end(),
    [handler](const SPort& port)
    {
      return port.handler == handler;
    }), m_ports.end());

  SetChanged();
  NotifyObservers(ObservableMessagePortsChanged);
}

void CPortManager::MapDevices(const PeripheralVector& devices,
                              std::map<PeripheralPtr, IInputHandler*>& deviceToPortMap)
{
  CSingleLock lock(m_mutex);

  if (m_ports.empty())
    return; // Nothing to do

  // Clear all ports
  for (SPort& port : m_ports)
    port.device = nullptr;

  // Prioritize devices by several criteria
  PeripheralVector devicesCopy = devices;
  std::sort(devicesCopy.begin(), devicesCopy.end(),
    [](const PeripheralPtr& lhs, const PeripheralPtr& rhs)
    {
      // Prioritize physical joysticks over emulated ones
      if (lhs->Type() == PERIPHERAL_JOYSTICK && rhs->Type() != PERIPHERAL_JOYSTICK)
        return true;
      if (lhs->Type() != PERIPHERAL_JOYSTICK && rhs->Type() == PERIPHERAL_JOYSTICK)
        return false;

      if (lhs->Type() == PERIPHERAL_JOYSTICK && rhs->Type() == PERIPHERAL_JOYSTICK)
      {
        std::shared_ptr<CPeripheralJoystick> i = std::static_pointer_cast<CPeripheralJoystick>(lhs);
        std::shared_ptr<CPeripheralJoystick> j = std::static_pointer_cast<CPeripheralJoystick>(rhs);

        // Prioritize requested a port over no port requested
        if (i->RequestedPort() != JOYSTICK_PORT_UNKNOWN && j->RequestedPort() == JOYSTICK_PORT_UNKNOWN)
          return true;
        if (i->RequestedPort() == JOYSTICK_PORT_UNKNOWN && j->RequestedPort() != JOYSTICK_PORT_UNKNOWN)
          return false;

        // Sort joystick by requested port
        return i->RequestedPort() < j->RequestedPort();
      }

      if (lhs->Type() == PERIPHERAL_JOYSTICK_EMULATION && rhs->Type() == PERIPHERAL_JOYSTICK_EMULATION)
      {
        std::shared_ptr<CPeripheralJoystickEmulation> i = std::static_pointer_cast<CPeripheralJoystickEmulation>(lhs);
        std::shared_ptr<CPeripheralJoystickEmulation> j = std::static_pointer_cast<CPeripheralJoystickEmulation>(rhs);

        // Sort emulated joysticks by player number
        return i->ControllerNumber() < j->ControllerNumber();
      }

      return false;
    });

  // Record mapped devices in output variable
  for (auto& device : devicesCopy)
  {
    IInputHandler* handler = AssignToPort(device);
    if (handler)
      deviceToPortMap[device] = handler;
  }
}

IInputHandler* CPortManager::AssignToPort(const PeripheralPtr& device, bool checkPortNumber /* = true */)
{
  const int requestedPort = GetRequestedPort(device);
  const bool bPortRequested = (requestedPort != JOYSTICK_PORT_UNKNOWN);

  for (SPort& port : m_ports)
  {
    // Skip occupied ports
    if (port.device != nullptr)
      continue;

    // If specified, check port numbers
    if (checkPortNumber)
    {
      if (bPortRequested && requestedPort != static_cast<int>(port.port))
        continue;
    }

    // If required, filter by type 
    const bool bTypeRequired = (port.requiredType != PERIPHERAL_UNKNOWN);
    if (bTypeRequired && port.requiredType != device->Type())
      continue;

    // Success
    port.device = device.get();
    return port.handler;
  }

  // If joystick requested a port but wasn't mapped, try again without checking port numbers
  if (checkPortNumber && bPortRequested)
    return AssignToPort(device, false);

  return nullptr;
}
