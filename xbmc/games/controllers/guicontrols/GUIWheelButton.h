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
  class CGUIWheelButton : public CGUIFeatureButton
  {
  public:
    CGUIWheelButton(const CGUIButtonControl& buttonTemplate,
                    IConfigurationWizard* wizard,
                    const CControllerFeature& feature,
                    unsigned int index);

    virtual ~CGUIWheelButton() = default;

    // implementation of IFeatureButton
    virtual bool PromptForInput(CEvent& waitEvent) override;
    virtual bool IsFinished(void) const override;
    virtual JOYSTICK::WHEEL_DIRECTION GetWheelDirection(void) const override;
    virtual void Reset(void) override;

  private:
    enum class STATE
    {
      WHEEL_LEFT,
      WHEEL_RIGHT,
      FINISHED,
    };

    STATE m_state;
  };
}
}
