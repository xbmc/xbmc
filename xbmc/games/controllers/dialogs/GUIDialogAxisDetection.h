/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogButtonCapture.h"

#include <string>
#include <utility>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIDialogAxisDetection : public CGUIDialogButtonCapture
{
public:
  CGUIDialogAxisDetection() = default;

  ~CGUIDialogAxisDetection() override = default;

  // specialization of IButtonMapper via CGUIDialogButtonCapture
  bool AcceptsPrimitive(JOYSTICK::PRIMITIVE_TYPE type) const override;
  void OnLateAxis(const JOYSTICK::IButtonMap* buttonMap, unsigned int axisIndex) override;

protected:
  // implementation of CGUIDialogButtonCapture
  std::string GetDialogText() override;
  std::string GetDialogHeader() override;
  bool MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                            KEYMAP::IKeymap* keymap,
                            const JOYSTICK::CDriverPrimitive& primitive) override;
  void OnClose(bool bAccepted) override {}

private:
  void AddAxis(const std::string& deviceLocation, unsigned int axisIndex);

  // Axis types
  using DeviceName = std::string;
  using AxisIndex = unsigned int;
  using AxisEntry = std::pair<DeviceName, AxisIndex>;
  using AxisVector = std::vector<AxisEntry>;

  // Axis detection
  AxisVector m_detectedAxes;
};
} // namespace GAME
} // namespace KODI
