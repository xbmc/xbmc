/*
 *      Copyright (C) 2017-present Team Kodi
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

#include "IKeymap.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>
#include <string>

class CWindowKeymap : public IWindowKeymap
{
public:
  explicit CWindowKeymap(const std::string &controllerId);

  // implementation of IWindowKeymap
  virtual std::string ControllerID() const override { return m_controllerId; }
  virtual void MapAction(int windowId, const std::string &keyName, KODI::JOYSTICK::KeymapAction action) override;
  virtual const KODI::JOYSTICK::KeymapActionGroup &GetActions(int windowId, const std::string& keyName) const override;

private:
  // Construction parameter
  const std::string m_controllerId;

  using KeyName = std::string;
  using Keymap = std::map<KeyName, KODI::JOYSTICK::KeymapActionGroup>;

  using WindowID = int;
  using WindowMap = std::map<WindowID, Keymap>;

  WindowMap m_windowKeymap;
};
