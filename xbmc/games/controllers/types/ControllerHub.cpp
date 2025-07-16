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
#include "utils/log.h"

#include <algorithm>
#include <utility>

#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto XML_ELM_PORT = "port";
} // namespace

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
  return std::ranges::any_of(m_ports, [&controllerId](const CPortNode& port)
                             { return port.IsControllerAccepted(controllerId); });
}

bool CControllerHub::IsControllerAccepted(const std::string& portAddress,
                                          const std::string& controllerId) const
{
  return std::ranges::any_of(m_ports, [&portAddress, &controllerId](const CPortNode& port)
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

void CControllerHub::GetKeyboardPorts(std::vector<std::string>& keyboardPorts) const
{
  for (const CPortNode& port : m_ports)
    port.GetKeyboardPorts(keyboardPorts);
}

void CControllerHub::GetMousePorts(std::vector<std::string>& mousePorts) const
{
  for (const CPortNode& port : m_ports)
    port.GetMousePorts(mousePorts);
}

bool CControllerHub::Serialize(tinyxml2::XMLElement& controllerElement) const
{
  // Iterate and serialize each port
  for (const CPortNode& portNode : m_ports)
  {
    // Create a new "port" element
    tinyxml2::XMLElement* portElement = controllerElement.GetDocument()->NewElement(XML_ELM_PORT);
    if (portElement == nullptr)
    {
      CLog::Log(LOGERROR, "Serialize: Failed to create \"{}\" element", XML_ELM_PORT);
      return false;
    }

    // Serialize the port node
    if (!portNode.Serialize(*portElement))
      return false;

    // Add the "port" element to the controller element
    controllerElement.InsertEndChild(portElement);
  }

  return true;
}

bool CControllerHub::Deserialize(const tinyxml2::XMLElement& controllerElement)
{
  Clear();

  // Get first "port" element
  const tinyxml2::XMLElement* portElement = controllerElement.FirstChildElement(XML_ELM_PORT);

  // Iterate over all "port" elements
  while (portElement != nullptr)
  {
    CPortNode port;

    // Deserialize port
    if (!port.Deserialize(*portElement))
      return false;

    m_ports.emplace_back(std::move(port));

    // Get next "port" element
    portElement = portElement->NextSiblingElement(XML_ELM_PORT);
  }

  return true;
}

std::string CControllerHub::GetDigest(UTILITY::CDigest::Type digestType) const
{
  UTILITY::CDigest digest{digestType};

  for (const CPortNode& port : m_ports)
    digest.Update(port.GetDigest(digestType));

  return digest.FinalizeRaw();
}
