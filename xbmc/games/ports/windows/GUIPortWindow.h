/*
 *  Copyright (C) 2021 Team Kodi
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
class IPortList;

/*!
 * \ingroup games
 */
class CGUIPortWindow : public CGUIDialog
{
public:
  CGUIPortWindow();
  ~CGUIPortWindow() override;

  // Implementation of CGUIControl via CGUIDialog
  bool OnMessage(CGUIMessage& message) override;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  void FrameMove() override;

private:
  // Actions for port list
  void UpdatePortList();
  void FocusPortList();
  bool OnClickAction();

  // Actions for the available buttons
  void ResetPorts();
  void CloseDialog();

  // GUI parameters
  std::unique_ptr<IPortList> m_portList;

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
