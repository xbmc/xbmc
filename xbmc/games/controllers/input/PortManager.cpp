/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PortManager.h"

#include "URL.h"
#include "filesystem/File.h"
#include "games/controllers/Controller.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/controllers/types/PortNode.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr const char* PORT_XML_FILE = "ports.xml";
constexpr const char* XML_ROOT_PORTS = "ports";
constexpr const char* XML_ELM_PORT = "port";
constexpr const char* XML_ELM_CONTROLLER = "controller";
constexpr const char* XML_ATTR_PORT_ID = "id";
constexpr const char* XML_ATTR_PORT_ADDRESS = "address";
constexpr const char* XML_ATTR_PORT_CONNECTED = "connected";
constexpr const char* XML_ATTR_PORT_CONTROLLER = "controller";
constexpr const char* XML_ATTR_CONTROLLER_ID = "id";
} // namespace

CPortManager::CPortManager() = default;

CPortManager::~CPortManager() = default;

void CPortManager::Initialize(const std::string& profilePath)
{
  m_xmlPath = URIUtils::AddFileToFolder(profilePath, PORT_XML_FILE);
}

void CPortManager::Deinitialize()
{
  m_controllerTree.Clear();
  m_xmlPath.clear();
}

void CPortManager::SetControllerTree(const CControllerTree& controllerTree)
{
  m_controllerTree = controllerTree;
}

void CPortManager::LoadXML()
{
  if (!XFILE::CFile::Exists(m_xmlPath))
  {
    CLog::Log(LOGDEBUG, "Can't load port config, file doesn't exist: {}", m_xmlPath);
    return;
  }

  CLog::Log(LOGINFO, "Loading port layout: {}", CURL::GetRedacted(m_xmlPath));

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(m_xmlPath))
  {
    CLog::Log(LOGDEBUG, "Unable to load file: {} at line {}", xmlDoc.ErrorDesc(),
              xmlDoc.ErrorRow());
    return;
  }

  const TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement == nullptr || pRootElement->NoChildren() ||
      pRootElement->ValueStr() != XML_ROOT_PORTS)
  {
    CLog::Log(LOGERROR, "Can't find root <{}> tag", XML_ROOT_PORTS);
    return;
  }

  DeserializePorts(pRootElement, m_controllerTree.GetPorts());
}

void CPortManager::SaveXML()
{
  CXBMCTinyXML doc;
  TiXmlElement node(XML_ROOT_PORTS);

  SerializePorts(node, m_controllerTree.GetPorts());

  doc.InsertEndChild(node);

  doc.SaveFile(m_xmlPath);
}

void CPortManager::Clear()
{
  m_xmlPath.clear();
  m_controllerTree.Clear();
}

void CPortManager::ConnectController(const std::string& portAddress,
                                     bool connected,
                                     const std::string& controllerId /* = "" */)
{
  ConnectController(portAddress, connected, controllerId, m_controllerTree.GetPorts());
}

bool CPortManager::ConnectController(const std::string& portAddress,
                                     bool connected,
                                     const std::string& controllerId,
                                     PortVec& ports)
{
  for (CPortNode& port : ports)
  {
    if (ConnectController(portAddress, connected, controllerId, port))
      return true;
  }

  return false;
}

bool CPortManager::ConnectController(const std::string& portAddress,
                                     bool connected,
                                     const std::string& controllerId,
                                     CPortNode& port)
{
  // Base case
  if (port.GetAddress() == portAddress)
  {
    port.SetConnected(connected);
    if (!controllerId.empty())
      port.SetActiveController(controllerId);
    return true;
  }

  // Check children
  return ConnectController(portAddress, connected, controllerId, port.GetCompatibleControllers());
}

bool CPortManager::ConnectController(const std::string& portAddress,
                                     bool connected,
                                     const std::string& controllerId,
                                     ControllerNodeVec& controllers)
{
  for (CControllerNode& controller : controllers)
  {
    if (ConnectController(portAddress, connected, controllerId, controller))
      return true;
  }

  return false;
}

bool CPortManager::ConnectController(const std::string& portAddress,
                                     bool connected,
                                     const std::string& controllerId,
                                     CControllerNode& controller)
{
  for (CPortNode& childPort : controller.GetHub().GetPorts())
  {
    if (ConnectController(portAddress, connected, controllerId, childPort))
      return true;
  }

  return false;
}

void CPortManager::DeserializePorts(const TiXmlElement* pElement, PortVec& ports)
{
  for (const TiXmlElement* pPort = pElement->FirstChildElement(); pPort != nullptr;
       pPort = pPort->NextSiblingElement())
  {
    if (pPort->ValueStr() != XML_ELM_PORT)
    {
      CLog::Log(LOGDEBUG, "Inside <{}> tag: Ignoring <{}> tag", pElement->ValueStr(),
                pPort->ValueStr());
      continue;
    }

    std::string portId = XMLUtils::GetAttribute(pPort, XML_ATTR_PORT_ID);

    auto it = std::find_if(ports.begin(), ports.end(),
                           [&portId](const CPortNode& port) { return port.GetPortID() == portId; });
    if (it != ports.end())
    {
      CPortNode& port = *it;

      DeserializePort(pPort, port);
    }
  }
}

void CPortManager::DeserializePort(const TiXmlElement* pPort, CPortNode& port)
{
  // Connected
  bool connected = (XMLUtils::GetAttribute(pPort, XML_ATTR_PORT_CONNECTED) == "true");
  port.SetConnected(connected);

  // Controller
  const std::string activeControllerId = XMLUtils::GetAttribute(pPort, XML_ATTR_PORT_CONTROLLER);
  if (!port.SetActiveController(activeControllerId))
    port.SetConnected(false);

  DeserializeControllers(pPort, port.GetCompatibleControllers());
}

void CPortManager::DeserializeControllers(const TiXmlElement* pPort, ControllerNodeVec& controllers)
{
  for (const TiXmlElement* pController = pPort->FirstChildElement(); pController != nullptr;
       pController = pController->NextSiblingElement())
  {
    if (pController->ValueStr() != XML_ELM_CONTROLLER)
    {
      CLog::Log(LOGDEBUG, "Inside <{}> tag: Ignoring <{}> tag", pPort->ValueStr(),
                pController->ValueStr());
      continue;
    }

    std::string controllerId = XMLUtils::GetAttribute(pPort, XML_ATTR_CONTROLLER_ID);

    auto it = std::find_if(controllers.begin(), controllers.end(),
                           [&controllerId](const CControllerNode& controller) {
                             return controller.GetController()->ID() == controllerId;
                           });
    if (it != controllers.end())
    {
      CControllerNode& controller = *it;

      DeserializeController(pController, controller);
    }
  }
}

void CPortManager::DeserializeController(const TiXmlElement* pController,
                                         CControllerNode& controller)
{
  // Child ports
  DeserializePorts(pController, controller.GetHub().GetPorts());
}

void CPortManager::SerializePorts(TiXmlElement& node, const PortVec& ports)
{
  for (const CPortNode& port : ports)
  {
    TiXmlElement portNode(XML_ELM_PORT);

    SerializePort(portNode, port);

    node.InsertEndChild(portNode);
  }
}

void CPortManager::SerializePort(TiXmlElement& portNode, const CPortNode& port)
{
  // Port ID
  portNode.SetAttribute(XML_ATTR_PORT_ID, port.GetPortID());

  // Port address
  portNode.SetAttribute(XML_ATTR_PORT_ADDRESS, port.GetAddress());

  // Connected state
  portNode.SetAttribute(XML_ATTR_PORT_CONNECTED, port.IsConnected() ? "true" : "false");

  // Active controller
  if (port.GetActiveController().GetController())
  {
    const std::string controllerId = port.GetActiveController().GetController()->ID();
    portNode.SetAttribute(XML_ATTR_PORT_CONTROLLER, controllerId);
  }

  // All compatible controllers
  SerializeControllers(portNode, port.GetCompatibleControllers());
}

void CPortManager::SerializeControllers(TiXmlElement& portNode,
                                        const ControllerNodeVec& controllers)
{
  for (const CControllerNode& controller : controllers)
  {
    // Skip controller if it has no state
    if (!HasState(controller))
      continue;

    TiXmlElement controllerNode(XML_ELM_CONTROLLER);

    SerializeController(controllerNode, controller);

    portNode.InsertEndChild(controllerNode);
  }
}

void CPortManager::SerializeController(TiXmlElement& controllerNode,
                                       const CControllerNode& controller)
{
  // Controller ID
  if (controller.GetController())
    controllerNode.SetAttribute(XML_ATTR_CONTROLLER_ID, controller.GetController()->ID());

  // Ports
  SerializePorts(controllerNode, controller.GetHub().GetPorts());
}

bool CPortManager::HasState(const CPortNode& port)
{
  // Ports have state (is connected / active controller)
  return true;
}

bool CPortManager::HasState(const CControllerNode& controller)
{
  // Check controller ports
  for (const CPortNode& port : controller.GetHub().GetPorts())
  {
    if (HasState(port))
      return true;
  }

  // Controller itself has no state
  return false;
}
