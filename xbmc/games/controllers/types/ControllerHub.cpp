/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerHub.h"

#include "ControllerTree.h"
#include "games/controllers/Controller.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace GAME;

CControllerHub::~CControllerHub() = default;

CControllerHub& CControllerHub::operator=(const CControllerHub& rhs)
{
  if (this != &rhs)
  {
    m_ports = rhs.m_ports;
  }

  return *this;
}

CControllerHub& CControllerHub::operator=(CControllerHub&& rhs) noexcept
{
  if (this != &rhs)
  {
    m_ports = std::move(rhs.m_ports);
  }

  return *this;
}

void CControllerHub::Clear()
{
  m_ports.clear();
}

void CControllerHub::SetPorts(PortVec ports)
{
  m_ports = std::move(ports);
}

bool CControllerHub::IsControllerAccepted(const std::string& controllerId) const
{
  return std::any_of(m_ports.begin(), m_ports.end(),
                     [controllerId](const CPortNode& port)
                     { return port.IsControllerAccepted(controllerId); });
}

bool CControllerHub::IsControllerAccepted(const std::string& portAddress,
                                          const std::string& controllerId) const
{
  return std::any_of(m_ports.begin(), m_ports.end(),
                     [portAddress, controllerId](const CPortNode& port)
                     { return port.IsControllerAccepted(portAddress, controllerId); });
}

ControllerVector CControllerHub::GetControllers() const
{
  ControllerVector controllers;
  GetControllers(controllers);
  return controllers;
}

void CControllerHub::GetControllers(ControllerVector& controllers) const
{
  for (const CPortNode& port : m_ports)
  {
    for (const CControllerNode& node : port.GetCompatibleControllers())
      node.GetControllers(controllers);
  }
}

const CPortNode& CControllerHub::GetPort(const std::string& address) const
{
  return GetPortInternal(m_ports, address);
}

const CPortNode& CControllerHub::GetPortInternal(const PortVec& ports, const std::string& address)
{
  for (const CPortNode& port : ports)
  {
    // Base case
    if (port.GetAddress() == address)
      return port;

    // Check children
    for (const CControllerNode& controller : port.GetCompatibleControllers())
    {
      const CPortNode& port = GetPortInternal(controller.GetHub().GetPorts(), address);
      if (port.GetAddress() == address)
        return port;
    }
  }

  // Not found
  static const CPortNode empty{};
  return empty;
}

void CControllerHub::GetInputPorts(std::vector<std::string>& inputPorts) const
{
  for (const CPortNode& port : m_ports)
    port.GetInputPorts(inputPorts);
}
