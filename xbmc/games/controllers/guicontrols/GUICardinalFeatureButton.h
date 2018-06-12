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
