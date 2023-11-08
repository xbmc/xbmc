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
/*!
 * \ingroup games
 */
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
  void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void UpdateInfo(const CGUIListItem* item = nullptr) override;

  // GUI functions
  void SetControllerID(const GUILIB::GUIINFO::CGUIInfoLabel& controllerId);
  void SetControllerAddress(const GUILIB::GUIINFO::CGUIInfoLabel& controllerAddress);
  void SetControllerDiffuse(const GUILIB::GUIINFO::CGUIInfoColor& color);
  void SetPortAddress(const GUILIB::GUIINFO::CGUIInfoLabel& portAddress);
  void SetPeripheralLocation(const GUILIB::GUIINFO::CGUIInfoLabel& peripheralLocation);

  // Game functions
  void ActivateController(const std::string& controllerId);
  void ActivateController(const ControllerPtr& controller);
  std::string GetPortAddress();
  std::string GetPeripheralLocation();

private:
  // GUI functions
  void SetActivation(float activation);

  // GUI parameters
  GUILIB::GUIINFO::CGUIInfoLabel m_controllerIdInfo;
  GUILIB::GUIINFO::CGUIInfoLabel m_controllerAddressInfo;
  GUILIB::GUIINFO::CGUIInfoColor m_controllerDiffuse;
  GUILIB::GUIINFO::CGUIInfoLabel m_portAddressInfo;
  GUILIB::GUIINFO::CGUIInfoLabel m_peripheralLocationInfo;

  // Game parameters
  ControllerPtr m_currentController;
  std::string m_portAddress;
  std::string m_peripheralLocation;

  // Synchronization parameters
  std::mutex m_mutex;
};
} // namespace GAME
} // namespace KODI
