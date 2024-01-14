/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/JoystickTypes.h"
#include "input/keymaps/interfaces/IKeyMapper.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace tinyxml2
{
class XMLElement;
class XMLNode;
} // namespace tinyxml2

namespace KODI
{
namespace KEYMAP
{
class IWindowKeymap;

/*!
 * \ingroup keymap
 */
class CJoystickMapper : public IKeyMapper
{
public:
  CJoystickMapper() = default;
  ~CJoystickMapper() override = default;

  // implementation of IKeyMapper
  void MapActions(int windowID, const tinyxml2::XMLNode* pDevice) override;
  void Clear() override;

  std::vector<std::shared_ptr<const KEYMAP::IWindowKeymap>> GetJoystickKeymaps() const;

private:
  void DeserializeJoystickNode(const tinyxml2::XMLNode* pDevice, std::string& controllerId);
  bool DeserializeButton(const tinyxml2::XMLElement* pButton,
                         std::string& feature,
                         KODI::JOYSTICK::ANALOG_STICK_DIRECTION& dir,
                         unsigned int& holdtimeMs,
                         std::set<std::string>& hotkeys,
                         std::string& actionStr);

  using ControllerID = std::string;
  std::map<ControllerID, std::shared_ptr<IWindowKeymap>> m_joystickKeymaps;

  std::vector<std::string> m_controllerIds;
};
} // namespace KEYMAP
} // namespace KODI
