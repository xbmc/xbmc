/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PortManager.h"

#include "URL.h"
#include "games/controllers/Controller.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/ports/types/PortNode.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>

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
  // Wait for save tasks
  for (std::future<void>& task : m_saveFutures)
    task.wait();
  m_saveFutures.clear();

  m_controllerTree.Clear();
  m_xmlPath.clear();
}

void CPortManager::SetControllerTree(const CControllerTree& controllerTree)
{
  m_controllerTree = controllerTree;
}

void CPortManager::LoadXML()
{
  if (!CFileUtils::Exists(m_xmlPath))
  {
    CLog::Log(LOGDEBUG, "Can't load port config, file doesn't exist: {}",
              CURL::GetRedacted(m_xmlPath));
    return;
  }

  CLog::Log(LOGINFO, "Loading port layout: {}", CURL::GetRedacted(m_xmlPath));

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(m_xmlPath))
  {
    CLog::Log(LOGDEBUG, "Unable to load file: {} at line {}", xmlDoc.ErrorStr(),
              xmlDoc.ErrorLineNum());
    return;
  }

  const auto* pRootElement = xmlDoc.RootElement();
  if (pRootElement == nullptr || pRootElement->NoChildren() ||
      std::strcmp(pRootElement->Value(), XML_ROOT_PORTS) != 0)
  {
    CLog::Log(LOGERROR, "Can't find root <{}> tag", XML_ROOT_PORTS);
    return;
  }

  DeserializePorts(pRootElement, m_controllerTree.GetPorts());
}

void CPortManager::SaveXMLAsync()
{
  PortVec ports = m_controllerTree.GetPorts();

  // Prune any finished save tasks
  m_saveFutures.erase(std::remove_if(m_saveFutures.begin(), m_saveFutures.end(),
                                     [](std::future<void>& task) {
                                       return task.wait_for(std::chrono::seconds(0)) ==
                                              std::future_status::ready;
                                     }),
                      m_saveFutures.end());

  // Save async
  std::future<void> task = std::async(std::launch::async,
                                      [this, ports = std::move(ports)]()
                                      {
                                        CXBMCTinyXML2 doc;
                                        auto* node = doc.NewElement(XML_ROOT_PORTS);
                                        if (node == nullptr)
                                          return;

                                        SerializePorts(*node, ports);

                                        doc.InsertEndChild(node);

                                        std::lock_guard<std::mutex> lock(m_saveMutex);
                                        doc.SaveFile(m_xmlPath);
                                      });

  m_saveFutures.emplace_back(std::move(task));
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

void CPortManager::DeserializePorts(const tinyxml2::XMLElement* pElement, PortVec& ports)
{
  for (const auto* pPort = pElement->FirstChildElement(); pPort != nullptr;
       pPort = pPort->NextSiblingElement())
  {
    if (std::strcmp(pPort->Value(), XML_ELM_PORT) != 0)
    {
      CLog::Log(LOGDEBUG, "Inside <{}> tag: Ignoring <{}> tag", pElement->Value(), pPort->Value());
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

void CPortManager::DeserializePort(const tinyxml2::XMLElement* pPort, CPortNode& port)
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

void CPortManager::DeserializeControllers(const tinyxml2::XMLElement* pPort,
                                          ControllerNodeVec& controllers)
{
  for (const auto* pController = pPort->FirstChildElement(); pController != nullptr;
       pController = pController->NextSiblingElement())
  {
    if (std::strcmp(pController->Value(), XML_ELM_CONTROLLER) != 0)
    {
      CLog::Log(LOGDEBUG, "Inside <{}> tag: Ignoring <{}> tag", pPort->Value(),
                pController->Value());
      continue;
    }

    std::string controllerId = XMLUtils::GetAttribute(pController, XML_ATTR_CONTROLLER_ID);

    auto it = std::find_if(controllers.begin(), controllers.end(),
                           [&controllerId](const CControllerNode& controller)
                           { return controller.GetController()->ID() == controllerId; });
    if (it != controllers.end())
    {
      CControllerNode& controller = *it;

      DeserializeController(pController, controller);
    }
  }
}

void CPortManager::DeserializeController(const tinyxml2::XMLElement* pController,
                                         CControllerNode& controller)
{
  // Child ports
  DeserializePorts(pController, controller.GetHub().GetPorts());
}

void CPortManager::SerializePorts(tinyxml2::XMLElement& node, const PortVec& ports)
{
  auto doc = node.GetDocument();
  for (const CPortNode& port : ports)
  {
    auto portNode = doc->NewElement(XML_ELM_PORT);
    if (portNode == nullptr)
      continue;

    SerializePort(*portNode, port);

    node.InsertEndChild(portNode);
  }
}

void CPortManager::SerializePort(tinyxml2::XMLElement& portNode, const CPortNode& port)
{
  // Port ID
  portNode.SetAttribute(XML_ATTR_PORT_ID, port.GetPortID().c_str());

  // Port address
  portNode.SetAttribute(XML_ATTR_PORT_ADDRESS, port.GetAddress().c_str());

  // Connected state
  portNode.SetAttribute(XML_ATTR_PORT_CONNECTED, port.IsConnected() ? "true" : "false");

  // Active controller
  if (port.GetActiveController().GetController())
  {
    const std::string controllerId = port.GetActiveController().GetController()->ID();
    portNode.SetAttribute(XML_ATTR_PORT_CONTROLLER, controllerId.c_str());
  }

  // All compatible controllers
  SerializeControllers(portNode, port.GetCompatibleControllers());
}

void CPortManager::SerializeControllers(tinyxml2::XMLElement& portNode,
                                        const ControllerNodeVec& controllers)
{
  for (const CControllerNode& controller : controllers)
  {
    // Skip controller if it has no state
    if (!HasState(controller))
      continue;

    auto controllerNode = portNode.GetDocument()->NewElement(XML_ELM_CONTROLLER);
    if (controllerNode == nullptr)
      continue;

    SerializeController(*controllerNode, controller);

    portNode.InsertEndChild(controllerNode);
  }
}

void CPortManager::SerializeController(tinyxml2::XMLElement& controllerNode,
                                       const CControllerNode& controller)
{
  // Controller ID
  if (controller.GetController())
    controllerNode.SetAttribute(XML_ATTR_CONTROLLER_ID, controller.GetController()->ID().c_str());

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
