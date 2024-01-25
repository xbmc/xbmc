/*
*  Copyright (C) 2024 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/mouse/interfaces/IMouseInputHandler.h"
#include "peripherals/PeripheralTypes.h"

namespace KODI
{
namespace GAME
{
class CControllerActivity;

/*!
 * \ingroup games
 *
 * \brief Handles mouse events for game agent functionality
 */
class CAgentMouse : public MOUSE::IMouseInputHandler
{
public:
  CAgentMouse(PERIPHERALS::PeripheralPtr peripheral);

  ~CAgentMouse() override;

  void Initialize();
  void Deinitialize();
  void ClearButtonState();

  // Input parameters
  float GetActivation() const;
  ControllerPtr Appearance() const { return m_controllerAppearance; }

  // Implementation of IMouseInputHandler
  std::string ControllerID() const override;
  bool OnMotion(const MOUSE::PointerName& relpointer, int differenceX, int differenceY) override;
  bool OnButtonPress(const MOUSE::ButtonName& button) override;
  void OnButtonRelease(const MOUSE::ButtonName& button) override;
  void OnInputFrame() override;

private:
  static INPUT::INTERCARDINAL_DIRECTION GetPointerDirection(int differenceX, int differenceY);

  // Construction parameters
  const PERIPHERALS::PeripheralPtr m_peripheral;

  // Input properties
  std::unique_ptr<CControllerActivity> m_mouseActivity;
  ControllerPtr m_controllerAppearance;

  // Input state
  bool m_bStarted{false};
  int m_startX{0};
  int m_startY{0};
  int m_frameCount{0};
};
} // namespace GAME
} // namespace KODI
