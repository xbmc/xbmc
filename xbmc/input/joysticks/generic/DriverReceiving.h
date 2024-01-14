/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"
#include "input/joysticks/interfaces/IInputReceiver.h"

#include <map>

namespace KODI
{
namespace JOYSTICK
{
class IDriverReceiver;
class IButtonMap;

/*!
 * \ingroup joystick
 *
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

  ~CDriverReceiving() override = default;

  // implementation of IInputReceiver
  bool SetRumbleState(const FeatureName& feature, float magnitude) override;

private:
  IDriverReceiver* const m_receiver;
  IButtonMap* const m_buttonMap;
};
} // namespace JOYSTICK
} // namespace KODI
