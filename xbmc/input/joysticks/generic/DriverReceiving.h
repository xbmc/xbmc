/*
 *      Copyright (C) 2016 Team Kodi
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

#include "input/joysticks/IInputReceiver.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>

namespace JOYSTICK
{
  class IDriverReceiver;
  class IButtonMap;

  /*!
   * \brief Class to translate input events from higher-level features to driver primitives
   *
   * A button map is used to translate controller features to driver primitives.
   * The button map has been abstracted away behind the IButtonMap interface
   * so that it can be provided by an add-on.
   */
  class CDriverReceiving : public IInputReceiver
  {
  public:
    CDriverReceiving(IDriverReceiver* receiver, IButtonMap* buttonMap);

    virtual ~CDriverReceiving(void) { }

    // implementation of IInputReceiver
    virtual bool SetRumbleState(const FeatureName& feature, float magnitude) override;

  private:
    IDriverReceiver* const m_receiver;
    IButtonMap*      const m_buttonMap;
  };
}
