/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FeatureHandling.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/joysticks/interfaces/IDriverHandler.h"

#include <map>

namespace KODI
{
namespace JOYSTICK
{
class CDriverPrimitive;
class CGUIDialogNewJoystick;
class IInputHandler;
class IButtonMap;

/*!
 * \ingroup joystick
 *
 * \brief Class to translate input from the driver into higher-level features
 *
 * Raw driver input arrives for three elements: buttons, hats and axes. When
 * driver input is handled by this class, it translates the raw driver
 * elements into physical joystick features, such as buttons, analog sticks,
 * etc.
 *
 * A button map is used to translate driver primitives to controller features.
 * The button map has been abstracted away behind the IButtonMap
 * interface so that it can be provided by an add-on.
 */
class CInputHandling : public IDriverHandler
{
public:
  CInputHandling(IInputHandler* handler, IButtonMap* buttonMap);

  ~CInputHandling() override;

  // implementation of IDriverHandler
  bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
  bool OnHatMotion(unsigned int hatIndex, HAT_STATE state) override;
  bool OnAxisMotion(unsigned int axisIndex,
                    float position,
                    int center,
                    unsigned int range) override;
  void OnInputFrame() override;

private:
  bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed);
  bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude);

  CJoystickFeature* CreateFeature(const FeatureName& featureName);

  IInputHandler* const m_handler;
  IButtonMap* const m_buttonMap;

  std::map<FeatureName, FeaturePtr> m_features;

  static CGUIDialogNewJoystick* const m_dialog;
};
} // namespace JOYSTICK
} // namespace KODI
