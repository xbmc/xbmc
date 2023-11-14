/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFeatureButton.h"

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIThrottleButton : public CGUIFeatureButton
{
public:
  CGUIThrottleButton(const CGUIButtonControl& buttonTemplate,
                     IConfigurationWizard* wizard,
                     const CPhysicalFeature& feature,
                     unsigned int index);

  ~CGUIThrottleButton() override = default;

  // implementation of IFeatureButton
  bool PromptForInput(CEvent& waitEvent) override;
  bool IsFinished() const override;
  JOYSTICK::THROTTLE_DIRECTION GetThrottleDirection() const override;
  void Reset() override;

private:
  enum class STATE
  {
    THROTTLE_UP,
    THROTTLE_DOWN,
    FINISHED,
  };

  STATE m_state;
};
} // namespace GAME
} // namespace KODI
