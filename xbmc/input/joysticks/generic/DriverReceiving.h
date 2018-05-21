/*
 *      Copyright (C) 2016-present Team Kodi
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

#include "input/joysticks/interfaces/IInputReceiver.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>

namespace KODI
{
namespace JOYSTICK
{
  class IDriverReceiver;
  class IButtonMap;

  /*!
   * \ingroup joystick
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

    virtual ~CDriverReceiving() = default;

    // implementation of IInputReceiver
    virtual bool SetRumbleState(const FeatureName& feature, float magnitude) override;

  private:
    IDriverReceiver* const m_receiver;
    IButtonMap*      const m_buttonMap;
  };
}
}
