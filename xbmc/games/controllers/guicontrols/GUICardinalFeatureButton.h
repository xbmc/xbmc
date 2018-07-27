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
  class CGUICardinalFeatureButton : public CGUIFeatureButton
  {
  public:
    CGUICardinalFeatureButton(const CGUIButtonControl& buttonTemplate,
                              IConfigurationWizard* wizard,
                              const CControllerFeature& feature,
                              unsigned int index);

    virtual ~CGUICardinalFeatureButton() = default;

    // implementation of IFeatureButton
    virtual bool PromptForInput(CEvent& waitEvent) override;
    virtual bool IsFinished(void) const override;
    virtual INPUT::CARDINAL_DIRECTION GetCardinalDirection(void) const override;
    virtual void Reset(void) override;

  private:
    enum class STATE
    {
      CARDINAL_DIRECTION_UP,
      CARDINAL_DIRECTION_RIGHT,
      CARDINAL_DIRECTION_DOWN,
      CARDINAL_DIRECTION_LEFT,
      FINISHED,
    };

    STATE m_state;
  };

  using CGUIAnalogStickButton = CGUICardinalFeatureButton;
  using CGUIRelativePointerButton = CGUICardinalFeatureButton;
}
}
