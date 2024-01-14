/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IButtonSequence.h"

#include <map>
#include <string>
#include <vector>

namespace KODI
{
namespace JOYSTICK
{
/*!
 * \ingroup joystick
 *
 * \brief Hush!!!
 */
class CJoystickEasterEgg : public IButtonSequence
{
public:
  explicit CJoystickEasterEgg(const std::string& controllerId);
  ~CJoystickEasterEgg() override = default;

  // implementation of IButtonSequence
  bool OnButtonPress(const FeatureName& feature) override;
  bool IsCapturing() override;

  static void OnFinish(void);

private:
  // Construction parameters
  const std::string m_controllerId;

  static const std::map<std::string, std::vector<FeatureName>> m_sequence;

  unsigned int m_state = 0;
};
} // namespace JOYSTICK
} // namespace KODI
