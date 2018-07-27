/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
