/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "games/controllers/ControllerFeature.h"
#include "games/controllers/windows/IConfigurationWindow.h"
#include "guilib/GUIButtonControl.h"

#include <string>

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

    virtual ~CGUIFeatureButton(void) { }

    // implementation of CGUIControl via CGUIButtonControl
    virtual void OnUnFocus(void) override;

    // partial implementation of IFeatureButton
    virtual const CControllerFeature& Feature(void) const override { return m_feature; }
    virtual KODI::JOYSTICK::ANALOG_STICK_DIRECTION GetDirection(void) const override { return KODI::JOYSTICK::ANALOG_STICK_DIRECTION::UNKNOWN; }

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
