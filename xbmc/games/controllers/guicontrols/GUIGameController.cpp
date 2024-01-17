/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameController.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/addons/input/GameClientTopology.h"
#include "games/agents/input/AgentInput.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "guilib/GUIListItem.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>
#include <tuple>

using namespace KODI;
using namespace GAME;

CGUIGameController::CGUIGameController(int parentID,
                                       int controlID,
                                       float posX,
                                       float posY,
                                       float width,
                                       float height,
                                       const CTextureInfo& texture)
  : CGUIImage(parentID, controlID, posX, posY, width, height, texture)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController::CGUIGameController(const CGUIGameController& from)
  : CGUIImage(from),
    m_controllerIdInfo(from.m_controllerIdInfo),
    m_controllerAddressInfo(from.m_controllerAddressInfo),
    m_controllerDiffuse(from.m_controllerDiffuse),
    m_portAddressInfo(from.m_portAddressInfo),
    m_peripheralLocationInfo(from.m_peripheralLocationInfo),
    m_currentController(from.m_currentController),
    m_portAddress(from.m_portAddress),
    m_peripheralLocation(from.m_peripheralLocation)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController* CGUIGameController::Clone(void) const
{
  return new CGUIGameController(*this);
}

void CGUIGameController::DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  std::string portAddress;
  std::string peripheralLocation;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    portAddress = m_portAddress;
    peripheralLocation = m_peripheralLocation;
  }

  const GAME::CAgentInput& agentInput = CServiceBroker::GetGameServices().AgentInput();

  // Highlight the controller if it is active
  float activation = 0.0f;

  if (!portAddress.empty())
    activation = agentInput.GetGamePortActivation(portAddress);

  if (!peripheralLocation.empty())
    activation = std::max(agentInput.GetPeripheralActivation(peripheralLocation), activation);

  SetActivation(activation);

  CGUIImage::DoProcess(currentTime, dirtyregions);
}

void CGUIGameController::UpdateInfo(const CGUIListItem* item /* = nullptr */)
{
  CGUIImage::UpdateInfo(item);

  if (item != nullptr)
  {
    std::string controllerId;
    std::string portAddress;
    std::string peripheralLocation;

    if (item->HasProperty("Addon.ID"))
      controllerId = item->GetProperty("Addon.ID").asString();

    if (controllerId.empty())
      controllerId = m_controllerIdInfo.GetItemLabel(item);

    portAddress = m_portAddressInfo.GetItemLabel(item);
    peripheralLocation = m_peripheralLocationInfo.GetItemLabel(item);

    std::string controllerAddress = m_controllerAddressInfo.GetItemLabel(item);
    if (!controllerAddress.empty())
      std::tie(portAddress, controllerId) = CGameClientTopology::SplitAddress(controllerAddress);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!controllerId.empty())
      ActivateController(controllerId);
    if (!portAddress.empty())
      m_portAddress = portAddress;
    if (!peripheralLocation.empty())
      m_peripheralLocation = peripheralLocation;
  }
}

void CGUIGameController::SetControllerID(const GUILIB::GUIINFO::CGUIInfoLabel& controllerId)
{
  m_controllerIdInfo = controllerId;

  // Check if a controller ID is available without a listitem
  static const CFileItem empty;
  const std::string strControllerId = m_controllerIdInfo.GetItemLabel(&empty);
  if (!strControllerId.empty())
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    ActivateController(strControllerId);
  }
}

void CGUIGameController::SetControllerAddress(
    const GUILIB::GUIINFO::CGUIInfoLabel& controllerAddress)
{
  m_controllerAddressInfo = controllerAddress;

  // Check if a controller address is available without a listitem
  static const CFileItem empty;
  const std::string strControllerAddress = m_controllerAddressInfo.GetItemLabel(&empty);
  if (!strControllerAddress.empty())
  {
    std::string controllerId;
    std::string portAddress;
    std::tie(portAddress, controllerId) = CGameClientTopology::SplitAddress(strControllerAddress);

    std::lock_guard<std::mutex> lock(m_mutex);
    ActivateController(controllerId);
    m_portAddress = portAddress;
  }
}

void CGUIGameController::SetControllerDiffuse(const GUILIB::GUIINFO::CGUIInfoColor& color)
{
  m_controllerDiffuse = color;
}

void CGUIGameController::SetPortAddress(const GUILIB::GUIINFO::CGUIInfoLabel& portAddress)
{
  m_portAddressInfo = portAddress;

  // Check if a port address is available without a listitem
  static const CFileItem empty;
  const std::string strPortAddress = m_portAddressInfo.GetItemLabel(&empty);
  if (!strPortAddress.empty())
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_portAddress = strPortAddress;
  }
}

void CGUIGameController::SetPeripheralLocation(
    const GUILIB::GUIINFO::CGUIInfoLabel& peripheralLocation)
{
  m_peripheralLocationInfo = peripheralLocation;

  // Check if a port address is available without a listitem
  static const CFileItem empty;
  const std::string strPeripheralLocation = m_peripheralLocationInfo.GetItemLabel(&empty);
  if (!strPeripheralLocation.empty())
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_peripheralLocation = strPeripheralLocation;
  }
}

void CGUIGameController::ActivateController(const std::string& controllerId)
{
  CGameServices& gameServices = CServiceBroker::GetGameServices();

  ControllerPtr controller = gameServices.GetController(controllerId);

  ActivateController(controller);
}

void CGUIGameController::ActivateController(const ControllerPtr& controller)
{
  m_currentController = controller;

  if (m_currentController)
    SetFileName(m_currentController->Layout().ImagePath());
  else
    SetFileName("");
}

std::string CGUIGameController::GetPortAddress()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_portAddress;
}

std::string CGUIGameController::GetPeripheralLocation()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_peripheralLocation;
}

void CGUIGameController::SetActivation(float activation)
{
  // Validate parameters
  if (activation < 0.0f)
    activation = 0.0f;
  if (activation > 1.0f)
    activation = 1.0f;

  // Get diffuse color parts
  const uint8_t alpha = (m_controllerDiffuse >> 24) & 0xff;
  const uint8_t red = (m_controllerDiffuse >> 16) & 0xff;
  const uint8_t green = (m_controllerDiffuse >> 8) & 0xff;
  const uint8_t blue = m_controllerDiffuse & 0xff;

  // Merge the diffuse color with white as a portion of the activation
  const uint8_t newAlpha = static_cast<uint8_t>(0xff - (0xff - alpha) * activation);
  const uint8_t newRed = static_cast<uint8_t>(0xff - (0xff - red) * activation);
  const uint8_t newGreen = static_cast<uint8_t>(0xff - (0xff - green) * activation);
  const uint8_t newBlue = static_cast<uint8_t>(0xff - (0xff - blue) * activation);

  const UTILS::COLOR::Color activationColor =
      (newAlpha << 24) | (newRed << 16) | (newGreen << 8) | newBlue;

  if (CGUIImage::SetColorDiffuse(activationColor))
    CGUIImage::UpdateDiffuseColor(nullptr);
}
