/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

struct AInputEvent;

class IInputDeviceEventHandler
{
public:
  virtual ~IInputDeviceEventHandler() = default;

  virtual bool OnInputDeviceEvent(const AInputEvent* event) = 0;

protected:
  IInputDeviceEventHandler() = default;
};
