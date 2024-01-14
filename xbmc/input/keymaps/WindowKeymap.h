/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"
#include "input/keymaps/interfaces/IKeymap.h"

#include <map>
#include <string>

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 */
class CWindowKeymap : public IWindowKeymap
{
public:
  explicit CWindowKeymap(const std::string& controllerId);

  // implementation of IWindowKeymap
  std::string ControllerID() const override { return m_controllerId; }
  void MapAction(int windowId, const std::string& keyName, KeymapAction action) override;
  const KeymapActionGroup& GetActions(int windowId, const std::string& keyName) const override;

private:
  // Construction parameter
  const std::string m_controllerId;

  using KeyName = std::string;
  using Keymap = std::map<KeyName, KeymapActionGroup>;

  using WindowID = int;
  using WindowMap = std::map<WindowID, Keymap>;

  WindowMap m_windowKeymap;
};
} // namespace KEYMAP
} // namespace KODI
