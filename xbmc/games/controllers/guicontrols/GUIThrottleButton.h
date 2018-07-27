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
  class CGUIThrottleButton : public CGUIFeatureButton
  {
  public:
    CGUIThrottleButton(const CGUIButtonControl& buttonTemplate,
                       IConfigurationWizard* wizard,
                       const CControllerFeature& feature,
                       unsigned int index);

    virtual ~CGUIThrottleButton() = default;

    // implementation of IFeatureButton
    virtual bool PromptForInput(CEvent& waitEvent) override;
    virtual bool IsFinished(void) const override;
    virtual JOYSTICK::THROTTLE_DIRECTION GetThrottleDirection(void) const override;
    virtual void Reset(void) override;

  private:
    enum class STATE
    {
      THROTTLE_UP,
      THROTTLE_DOWN,
      FINISHED,
    };

    STATE m_state;
  };
}
}
