/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IKeymap.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>
#include <string>

class CWindowKeymap : public IWindowKeymap
{
public:
  explicit CWindowKeymap(const std::string &controllerId);

  // implementation of IWindowKeymap
  std::string ControllerID() const override { return m_controllerId; }
  void MapAction(int windowId, const std::string& keyName, KODI::JOYSTICK::KeymapAction action) override;
  const KODI::JOYSTICK::KeymapActionGroup& GetActions(int windowId, const std::string& keyName) const override;

private:
  // Construction parameter
  const std::string m_controllerId;

  using KeyName = std::string;
  using Keymap = std::map<KeyName, KODI::JOYSTICK::KeymapActionGroup>;

  using WindowID = int;
  using WindowMap = std::map<WindowID, Keymap>;

  WindowMap m_windowKeymap;
};
