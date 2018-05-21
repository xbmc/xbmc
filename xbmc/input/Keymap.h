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

#include <memory>

class IKeymapEnvironment;

class CKeymap : public IKeymap
{
public:
  CKeymap(std::shared_ptr<const IWindowKeymap> keymap, const IKeymapEnvironment *environment);

  // implementation of IKeymap
  virtual std::string ControllerID() const override ;
  virtual const IKeymapEnvironment *Environment() const override { return m_environment; }
  const KODI::JOYSTICK::KeymapActionGroup &GetActions(const std::string& keyName) const override;

private:
  // Construction parameters
  const std::shared_ptr<const IWindowKeymap> m_keymap;
  const IKeymapEnvironment *const m_environment;
};
