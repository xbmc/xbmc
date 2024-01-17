/*
 *  Copyright (C) 2022-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"

#include <memory>

namespace KODI
{
namespace GAME
{
class IActivePortList;
class IAgentControllerList;

/*!
 * \ingroup games
 */
class CGUIAgentWindow : public CGUIDialog
{
public:
  CGUIAgentWindow();
  ~CGUIAgentWindow() override;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void FrameMove() override;
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  // Window actions
  void CloseDialog();

  // Actions for port list
  void UpdateActivePortList();

  // Actions for controller list
  void UpdateControllerList();
  void FocusControllerList();
  void OnControllerClick();

  // GUI parameters
  std::unique_ptr<IActivePortList> m_portList;
  std::unique_ptr<IAgentControllerList> m_controllerList;

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
