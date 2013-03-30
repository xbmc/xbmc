/*
 *      Copyright (C) 2007-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "Joystick.h"

#include <vector>
#include <boost/shared_ptr.hpp>

/**
 * Interface IJoystick
 *
 * Joysticks are abstracted as devices that repeatedly refresh and report their
 * ineternal state. Update() is called by CJoystickManager once per FrameMove()
 * to poll for input and should sync the SJoystick struct returned by GetState()
 * to the joystick's current state.
 */
class IJoystick
{
public:
  /**
   * Implementers should provide the following factories to create IJoystick objects
   * (See CJoystickManager::Initialize()):
   *
   * static void Initialize(JoystickArray &joysticks);
   * static void DeInitialize(JoystickArray &joysticks);
   */

  virtual ~IJoystick() { }

  virtual void Update() = 0;

  virtual const SJoystick &GetState() const = 0;
};

typedef std::vector<boost::shared_ptr<IJoystick> > JoystickArray;
