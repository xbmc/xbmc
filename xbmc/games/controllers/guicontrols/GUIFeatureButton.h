/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/input/PhysicalFeature.h"
#include "games/controllers/windows/IConfigurationWindow.h"
#include "guilib/GUIButtonControl.h"

#include <string>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIFeatureButton : public CGUIButtonControl, public IFeatureButton
{
public:
  CGUIFeatureButton(const CGUIButtonControl& buttonTemplate,
                    IConfigurationWizard* wizard,
                    const CPhysicalFeature& feature,
                    unsigned int index);

  ~CGUIFeatureButton() override = default;

  // implementation of CGUIControl via CGUIButtonControl
  void OnUnFocus() override;

  // partial implementation of IFeatureButton
  const CPhysicalFeature& Feature() const override { return m_feature; }
  INPUT::CARDINAL_DIRECTION GetCardinalDirection() const override
  {
    return INPUT::CARDINAL_DIRECTION::NONE;
  }
  JOYSTICK::WHEEL_DIRECTION GetWheelDirection() const override
  {
    return JOYSTICK::WHEEL_DIRECTION::NONE;
  }
  JOYSTICK::THROTTLE_DIRECTION GetThrottleDirection() const override
  {
    return JOYSTICK::THROTTLE_DIRECTION::NONE;
  }

protected:
  bool DoPrompt(const std::string& strPrompt,
                const std::string& strWarn,
                const std::string& strFeature,
                CEvent& waitEvent);

  // FSM helper
  template<typename T>
  T GetNextState(T state)
  {
    return static_cast<T>(static_cast<int>(state) + 1);
  }

  const CPhysicalFeature m_feature;

private:
  IConfigurationWizard* const m_wizard;
};
} // namespace GAME
} // namespace KODI
