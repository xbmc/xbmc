/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
