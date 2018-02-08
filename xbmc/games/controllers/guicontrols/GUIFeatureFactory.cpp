/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GUIFeatureFactory.h"
#include "GUICardinalFeatureButton.h"
#include "GUIScalarFeatureButton.h"
#include "GUISelectKeyButton.h"
#include "GUIThrottleButton.h"
#include "GUIWheelButton.h"

using namespace KODI;
using namespace GAME;

CGUIButtonControl* CGUIFeatureFactory::CreateButton(BUTTON_TYPE type,
                                                    const CGUIButtonControl& buttonTemplate,
                                                    IConfigurationWizard* wizard,
                                                    const CControllerFeature& feature,
                                                    unsigned int index)
{
  switch (type)
  {
  case BUTTON_TYPE::BUTTON:
    return new CGUIScalarFeatureButton(buttonTemplate, wizard, feature, index);

  case BUTTON_TYPE::ANALOG_STICK:
    return new CGUIAnalogStickButton(buttonTemplate, wizard, feature, index);

  case BUTTON_TYPE::WHEEL:
    return new CGUIWheelButton(buttonTemplate, wizard, feature, index);

  case BUTTON_TYPE::THROTTLE:
    return new CGUIThrottleButton(buttonTemplate, wizard, feature, index);

  case BUTTON_TYPE::SELECT_KEY:
    return new CGUISelectKeyButton(buttonTemplate, wizard, index);

  case BUTTON_TYPE::RELATIVE_POINTER:
    return new CGUIRelativePointerButton(buttonTemplate, wizard, feature, index);

  default:
    break;
  }

  return nullptr;
}
