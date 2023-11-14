/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IActivePortList.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "games/ports/types/PortNode.h"
#include "utils/Observer.h"

#include <memory>

class CFileItemList;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
class CController;
class IActivePortList;

/*!
 * \ingroup games
 */
class CGUIActivePortList : public IActivePortList, public Observer
{
public:
  CGUIActivePortList(CGUIWindow& window, int controlId, bool showInputDisabled);
  ~CGUIActivePortList() override;

  // Implementation of IActivePortList
  bool Initialize(GameClientPtr gameClient) override;
  void Deinitialize() override;
  void Refresh() override;

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  // GUI helpers
  void InitializeGUI();
  void DeinitializeGUI();
  void AddInputDisabled();
  void AddItems(const PortVec& ports);
  void AddItem(const ControllerPtr& controller, const std::string& controllerAddress);
  void AddPadding();
  void CleanupItems();

  // Construction parameters
  CGUIWindow& m_guiWindow;
  const int m_controlId;
  const bool m_showInputDisabled;

  // GUI parameters
  std::unique_ptr<CFileItemList> m_vecItems;
  uint32_t m_alignment{0};

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
