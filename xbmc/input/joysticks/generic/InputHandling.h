/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "FeatureHandling.h"
#include "input/joysticks/IDriverHandler.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>

namespace JOYSTICK
{
  class CDriverPrimitive;
  class CGUIDialogNewJoystick;
  class IInputHandler;
  class IButtonMap;

  /*!
   * \ingroup joystick
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

    virtual ~CInputHandling(void);

    // implementation of IDriverHandler
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed) override;
    virtual bool OnHatMotion(unsigned int hatIndex, HAT_STATE state) override;
    virtual bool OnAxisMotion(unsigned int axisIndex, float position, int center, unsigned int range) override;
    virtual void ProcessAxisMotions(void) override;

  private:
    bool OnDigitalMotion(const CDriverPrimitive& source, bool bPressed);
    bool OnAnalogMotion(const CDriverPrimitive& source, float magnitude);

    CJoystickFeature* CreateFeature(const FeatureName& featureName);

    IInputHandler* const m_handler;
    IButtonMap* const    m_buttonMap;

    std::map<FeatureName, FeaturePtr> m_features;

    static CGUIDialogNewJoystick* const m_dialog;
  };
}
