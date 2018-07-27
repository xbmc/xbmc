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
  class CGUIScalarFeatureButton : public CGUIFeatureButton
  {
  public:
    CGUIScalarFeatureButton(const CGUIButtonControl& buttonTemplate,
                            IConfigurationWizard* wizard,
                            const CControllerFeature& feature,
                            unsigned int index);

    virtual ~CGUIScalarFeatureButton() = default;

    // implementation of IFeatureButton
    virtual bool PromptForInput(CEvent& waitEvent) override;
    virtual bool IsFinished(void) const override;
    virtual void Reset(void) override;

  private:
    enum class STATE
    {
      NEED_INPUT,
      FINISHED,
    };

    STATE m_state;
  };
}
}
