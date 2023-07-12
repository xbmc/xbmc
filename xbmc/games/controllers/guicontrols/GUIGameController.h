/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "guilib/GUIImage.h"

#include <mutex>

namespace KODI
{
namespace GAME
{
class CGUIGameController : public CGUIImage
{
public:
  CGUIGameController(int parentID,
                     int controlID,
                     float posX,
                     float posY,
                     float width,
                     float height,
                     const CTextureInfo& texture);
  CGUIGameController(const CGUIGameController& from);

  ~CGUIGameController() override = default;

  // implementation of CGUIControl via CGUIImage
  CGUIGameController* Clone() const override;
  void Render() override;
  void UpdateInfo(const CGUIListItem* item = nullptr) override;

  // GUI functions
  void SetControllerID(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& controllerId);
  void SetControllerAddress(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& controllerAddress);

  // Game functions
  void ActivateController(const std::string& controllerId);
  void ActivateController(const ControllerPtr& controller);
  std::string GetPortAddress();

private:
  // GUI parameters
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_controllerIdInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_controllerAddressInfo;

  // Game parameters
  ControllerPtr m_currentController;
  std::string m_portAddress;

  // Synchronization parameters
  std::mutex m_mutex;
};
} // namespace GAME
} // namespace KODI
