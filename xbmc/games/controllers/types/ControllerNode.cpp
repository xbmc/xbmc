/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerNode.h"

#include "ControllerHub.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTranslator.h"
#include "games/controllers/input/PhysicalTopology.h"
#include "games/ports/types/PortNode.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto XML_ATTR_CONTROLLER = "controller";
} // namespace

CControllerNode::CControllerNode() : m_hub(new CControllerHub)
{
}

CControllerNode::~CControllerNode() = default;

CControllerNode& CControllerNode::operator=(const CControllerNode& rhs)
{
  if (this != &rhs)
  {
    m_controller = rhs.m_controller;
    m_portAddress = rhs.m_portAddress;
    m_controllerAddress = rhs.m_controllerAddress;
    m_hub = std::make_unique<CControllerHub>(*rhs.m_hub);
  }

  return *this;
}

CControllerNode& CControllerNode::operator=(CControllerNode&& rhs) noexcept
{
  if (this != &rhs)
  {
    m_controller = std::move(rhs.m_controller);
    m_portAddress = std::move(rhs.m_portAddress);
    m_controllerAddress = std::move(rhs.m_controllerAddress);
    m_hub = std::move(rhs.m_hub);
  }

  return *this;
}

void CControllerNode::Clear()
{
  m_controller.reset();
  m_portAddress.clear();
  m_controllerAddress.clear();
  m_hub = std::make_unique<CControllerHub>();
}

void CControllerNode::SetController(ControllerPtr controller)
{
  m_controller = std::move(controller);
}

void CControllerNode::GetControllers(ControllerVector& controllers) const
{
  if (m_controller)
  {
    const ControllerPtr& myController = m_controller;

    auto it = std::find_if(controllers.begin(), controllers.end(),
                           [&myController](const ControllerPtr& controller)
                           { return myController->ID() == controller->ID(); });

    if (it == controllers.end())
      controllers.emplace_back(m_controller);
  }

  m_hub->GetControllers(controllers);
}

void CControllerNode::SetPortAddress(std::string portAddress)
{
  m_portAddress = std::move(portAddress);
}

void CControllerNode::SetControllerAddress(std::string controllerAddress)
{
  m_controllerAddress = std::move(controllerAddress);
}

void CControllerNode::SetHub(CControllerHub hub)
{
  m_hub = std::make_unique<CControllerHub>(std::move(hub));
}

bool CControllerNode::IsControllerAccepted(const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const auto& port : m_hub->GetPorts())
  {
    if (port.IsControllerAccepted(controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

bool CControllerNode::IsControllerAccepted(const std::string& portAddress,
                                           const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const auto& port : m_hub->GetPorts())
  {
    if (port.IsControllerAccepted(portAddress, controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

bool CControllerNode::ProvidesInput() const
{
  return m_controller && m_controller->Topology().ProvidesInput();
}

void CControllerNode::GetInputPorts(std::vector<std::string>& inputPorts) const
{
  if (ProvidesInput())
    inputPorts.emplace_back(m_portAddress);

  m_hub->GetInputPorts(inputPorts);
}

void CControllerNode::GetKeyboardPorts(std::vector<std::string>& keyboardPorts) const
{
  m_hub->GetKeyboardPorts(keyboardPorts);
}

void CControllerNode::GetMousePorts(std::vector<std::string>& mousePorts) const
{
  m_hub->GetMousePorts(mousePorts);
}

bool CControllerNode::Serialize(tinyxml2::XMLElement& controllerElement) const
{
  // Validate state
  if (!m_controller)
  {
    CLog::Log(LOGERROR, "Controller is not set");
    return false;
  }

  // Set controller ID
  controllerElement.SetAttribute(XML_ATTR_CONTROLLER, m_controller->ID().c_str());

  // Serialize hub
  if (!m_hub->Serialize(controllerElement))
    return false;

  return true;
}

bool CControllerNode::Deserialize(const tinyxml2::XMLElement& controllerElement)
{
  Clear();

  // Get controller ID
  const char* controllerId = controllerElement.Attribute(XML_ATTR_CONTROLLER);
  if (controllerId == nullptr)
  {
    CLog::Log(LOGERROR, "Controller is missing \"{}\" attribute", XML_ATTR_CONTROLLER);
    return false;
  }

  ADDON::AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(
          controllerId, addon, ADDON::AddonType::GAME_CONTROLLER, ADDON::OnlyEnabled::CHOICE_NO))
  {
    CLog::Log(LOGERROR, "Unknown controller: \"{}\"", controllerId);
    return false;
  }
  m_controller = std::static_pointer_cast<CController>(addon);

  // Deserialize hub
  if (!m_hub->Deserialize(controllerElement))
    return false;

  return true;
}

std::string CControllerNode::GetDigest(UTILITY::CDigest::Type digestType) const
{
  UTILITY::CDigest digest{digestType};

  if (m_controller)
    digest.Update(m_controller->ID());
  digest.Update(m_hub->GetDigest(digestType));

  return digest.FinalizeRaw();
}
