/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PortNode.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTranslator.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/ports/input/PhysicalPort.h"
#include "utils/log.h"

#include <algorithm>
#include <utility>

#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto XML_ELM_ACCEPTS = "accepts";
constexpr auto XML_ATTR_PORT_TYPE = "type";
constexpr auto XML_ATTR_PORT_ID = "id";
} // namespace

CPortNode::~CPortNode() = default;

CPortNode& CPortNode::operator=(const CPortNode& rhs)
{
  if (this != &rhs)
  {
    m_bConnected = rhs.m_bConnected;
    m_active = rhs.m_active;
    m_portType = rhs.m_portType;
    m_portId = rhs.m_portId;
    m_address = rhs.m_address;
    m_forceConnected = rhs.m_forceConnected;
    m_controllers = rhs.m_controllers;
  }

  return *this;
}

CPortNode& CPortNode::operator=(CPortNode&& rhs) noexcept
{
  if (this != &rhs)
  {
    m_bConnected = rhs.m_bConnected;
    m_active = rhs.m_active;
    m_portType = rhs.m_portType;
    m_portId = std::move(rhs.m_portId);
    m_address = std::move(rhs.m_address);
    m_forceConnected = rhs.m_forceConnected;
    m_controllers = std::move(rhs.m_controllers);
  }

  return *this;
}

void CPortNode::Clear()
{
  *this = CPortNode{};
}

const CControllerNode& CPortNode::GetActiveController() const
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static const CControllerNode invalid{};
  return invalid;
}

CControllerNode& CPortNode::GetActiveController()
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static CControllerNode invalid;
  invalid.Clear();
  return invalid;
}

bool CPortNode::SetActiveController(const std::string& controllerId)
{
  for (size_t i = 0; i < m_controllers.size(); ++i)
  {
    const ControllerPtr& controller = m_controllers.at(i).GetController();
    if (controller && controller->ID() == controllerId)
    {
      m_active = i;
      return true;
    }
  }

  return false;
}

void CPortNode::SetPortID(std::string portId)
{
  m_portId = std::move(portId);
}

void CPortNode::SetAddress(std::string address)
{
  m_address = std::move(address);
}

void CPortNode::SetCompatibleControllers(ControllerNodeVec controllers)
{
  m_controllers = std::move(controllers);
}

bool CPortNode::IsControllerAccepted(const std::string& controllerId) const
{
  // Base case
  CPhysicalPort port;
  GetPort(port);
  if (port.IsCompatible(controllerId))
    return true;

  // Visit nodes
  return std::any_of(m_controllers.begin(), m_controllers.end(),
                     [controllerId](const CControllerNode& node)
                     { return node.IsControllerAccepted(controllerId); });
}

bool CPortNode::IsControllerAccepted(const std::string& portAddress,
                                     const std::string& controllerId) const
{
  bool bAccepted = false;

  if (m_address == portAddress)
  {
    // Base case
    CPhysicalPort port;
    GetPort(port);
    if (port.IsCompatible(controllerId))
      bAccepted = true;
  }
  else
  {
    // Visit nodes
    if (std::any_of(m_controllers.begin(), m_controllers.end(),
                    [portAddress, controllerId](const CControllerNode& node)
                    { return node.IsControllerAccepted(portAddress, controllerId); }))
    {
      bAccepted = true;
    }
  }

  return bAccepted;
}

void CPortNode::GetInputPorts(std::vector<std::string>& inputPorts) const
{
  if (IsConnected())
  {
    const CControllerNode& controller = GetActiveController();
    controller.GetInputPorts(inputPorts);
  }
}

void CPortNode::GetKeyboardPorts(std::vector<std::string>& keyboardPorts) const
{
  // Base case: we're a keyboard port
  if (GetPortType() == PORT_TYPE::KEYBOARD)
    keyboardPorts.emplace_back(GetAddress());

  // Visit children
  if (IsConnected())
  {
    const CControllerNode& controller = GetActiveController();
    controller.GetKeyboardPorts(keyboardPorts);
  }
}

void CPortNode::GetMousePorts(std::vector<std::string>& mousePorts) const
{
  // Base case: we're a mouse port
  if (GetPortType() == PORT_TYPE::MOUSE)
    mousePorts.emplace_back(GetAddress());

  // Visit children
  if (IsConnected())
  {
    const CControllerNode& controller = GetActiveController();
    controller.GetMousePorts(mousePorts);
  }
}

void CPortNode::GetPort(CPhysicalPort& port) const
{
  std::vector<std::string> accepts;
  for (const CControllerNode& node : m_controllers)
  {
    if (node.GetController())
      accepts.emplace_back(node.GetController()->ID());
  }

  port = CPhysicalPort(m_portId, std::move(accepts));
}

bool CPortNode::Serialize(tinyxml2::XMLElement& portElement) const
{
  // Validate state
  if (m_portType == PORT_TYPE::UNKNOWN)
  {
    CLog::Log(LOGERROR, "Port type is unknown");
    return false;
  }
  if (m_portId.empty())
  {
    CLog::Log(LOGERROR, "Port ID is empty");
    return false;
  }
  if (m_controllers.empty())
  {
    CLog::Log(LOGERROR, "Port has no accepted controllers");
    return false;
  }

  // Set the port type
  portElement.SetAttribute(XML_ATTR_PORT_TYPE,
                           CControllerTranslator::TranslatePortType(m_portType));

  // Set the port ID
  portElement.SetAttribute(XML_ATTR_PORT_ID, m_portId.c_str());

  // Iterate and serialize each controller accepted by this port
  for (const auto& controllerNode : m_controllers)
  {
    // Create a new "accepts" element
    tinyxml2::XMLElement* acceptsElement = portElement.GetDocument()->NewElement(XML_ELM_ACCEPTS);
    if (acceptsElement == nullptr)
      return false;

    // Serialize the controller node
    if (!controllerNode.Serialize(*acceptsElement))
      return false;

    // Add the "accepts" element to the port element
    portElement.InsertEndChild(acceptsElement);
  }

  return true;
}

bool CPortNode::Deserialize(const tinyxml2::XMLElement& portElement)
{
  Clear();

  // Get port type
  const char* portType = portElement.Attribute(XML_ATTR_PORT_TYPE);
  if (portType != nullptr)
    m_portType = CControllerTranslator::TranslatePortType(portType);

  // Default to controller port
  if (m_portType == PORT_TYPE::UNKNOWN)
    m_portType = PORT_TYPE::CONTROLLER;

  // Get port ID
  const char* portId = portElement.Attribute(XML_ATTR_PORT_ID);
  if (portId != nullptr)
    m_portId = portId;
  else
  {
    // Get port ID from port type
    switch (m_portType)
    {
      case PORT_TYPE::KEYBOARD:
        m_portId = KEYBOARD_PORT_ID;
        break;
      case PORT_TYPE::MOUSE:
        m_portId = MOUSE_PORT_ID;
        break;
      default:
        break;
    }
  }

  if (m_portId.empty())
  {
    CLog::Log(LOGERROR, "Port of type {} is missing \"{}\" attribute",
              CControllerTranslator::TranslatePortType(m_portType), XML_ATTR_PORT_ID);
    return false;
  }

  // Get first "accepts" element
  const tinyxml2::XMLElement* controllerElement = portElement.FirstChildElement(XML_ELM_ACCEPTS);
  if (controllerElement == nullptr)
  {
    CLog::Log(LOGERROR, "Port {} of type {} is missing \"{}\" element", m_portId,
              CControllerTranslator::TranslatePortType(m_portType), XML_ELM_ACCEPTS);
    return false;
  }

  // Iterate over all "accepts" elements
  while (controllerElement != nullptr)
  {
    CControllerNode controllerNode;
    if (!controllerNode.Deserialize(*controllerElement))
      return false;

    m_controllers.emplace_back(std::move(controllerNode));

    // Get next "accepts" element
    controllerElement = controllerElement->NextSiblingElement(XML_ELM_ACCEPTS);
  }

  return true;
}

std::string CPortNode::GetDigest(UTILITY::CDigest::Type digestType) const
{
  UTILITY::CDigest digest{digestType};

  digest.Update(CControllerTranslator::TranslatePortType(m_portType));
  digest.Update(m_portId);

  for (const CControllerNode& controller : m_controllers)
    digest.Update(controller.GetDigest(digestType));

  return digest.FinalizeRaw();
}
