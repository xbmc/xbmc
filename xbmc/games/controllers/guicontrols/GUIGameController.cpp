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
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "guilib/GUIListItem.h"
#include "utils/log.h"

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
    m_currentController(from.m_currentController),
    m_portAddress(from.m_portAddress)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLER;
}

CGUIGameController* CGUIGameController::Clone(void) const
{
  return new CGUIGameController(*this);
}

void CGUIGameController::Render(void)
{
  CGUIImage::Render();

  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_currentController)
  {
    //! @todo Render pressed buttons
  }
}

void CGUIGameController::UpdateInfo(const CGUIListItem* item /* = nullptr */)
{
  CGUIImage::UpdateInfo(item);

  if (item != nullptr)
  {
    std::string controllerId;
    std::string portAddress;

    if (item->HasProperty("Addon.ID"))
      controllerId = item->GetProperty("Addon.ID").asString();

    if (controllerId.empty())
      controllerId = m_controllerIdInfo.GetItemLabel(item);

    std::string controllerAddress = m_controllerAddressInfo.GetItemLabel(item);
    if (!controllerAddress.empty())
      std::tie(portAddress, controllerId) = CGameClientTopology::SplitAddress(controllerAddress);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!controllerId.empty())
      ActivateController(controllerId);
    if (!portAddress.empty())
      m_portAddress = portAddress;
  }
}

void CGUIGameController::SetControllerID(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& controllerId)
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
    const KODI::GUILIB::GUIINFO::CGUIInfoLabel& controllerAddress)
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

void CGUIGameController::ActivateController(const std::string& controllerId)
{
  CGameServices& gameServices = CServiceBroker::GetGameServices();

  ControllerPtr controller = gameServices.GetController(controllerId);

  ActivateController(controller);
}

void CGUIGameController::ActivateController(const ControllerPtr& controller)
{
  if (!controller)
    return;

  bool updated = false;

  if (!m_currentController || controller->ID() != m_currentController->ID())
  {
    m_currentController = controller;
    updated = true;
  }

  if (updated)
    SetFileName(m_currentController->Layout().ImagePath());
}

std::string CGUIGameController::GetPortAddress()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_portAddress;
}
