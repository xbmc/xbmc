/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerFeature.h"
#include "games/controllers/windows/IConfigurationWindow.h"
#include "guilib/GUIButtonControl.h"

#include <string>

namespace KODI
{
namespace GAME
{
  class CGUIFeatureButton : public CGUIButtonControl,
                            public IFeatureButton
  {
  public:
    CGUIFeatureButton(const CGUIButtonControl& buttonTemplate,
                      IConfigurationWizard* wizard,
                      const CControllerFeature& feature,
                      unsigned int index);

    virtual ~CGUIFeatureButton() = default;

    // implementation of CGUIControl via CGUIButtonControl
    virtual void OnUnFocus(void) override;

    // partial implementation of IFeatureButton
    virtual const CControllerFeature& Feature(void) const override { return m_feature; }
    virtual INPUT::CARDINAL_DIRECTION GetCardinalDirection(void) const override { return INPUT::CARDINAL_DIRECTION::NONE; }
    virtual JOYSTICK::WHEEL_DIRECTION GetWheelDirection(void) const override { return JOYSTICK::WHEEL_DIRECTION::NONE; }
    virtual JOYSTICK::THROTTLE_DIRECTION GetThrottleDirection(void) const override { return JOYSTICK::THROTTLE_DIRECTION::NONE; }

  protected:
    bool DoPrompt(const std::string& strPrompt, const std::string& strWarn, const std::string& strFeature, CEvent& waitEvent);

    // FSM helper
    template <typename T>
    T GetNextState(T state)
    {
      return static_cast<T>(static_cast<int>(state) + 1);
    }

    const CControllerFeature m_feature;

  private:
    IConfigurationWizard* const  m_wizard;
  };
}
}
