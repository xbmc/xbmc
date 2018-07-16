/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IButtonMapper.h"
#include "input/joysticks/JoystickTypes.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class IWindowKeymap;
class TiXmlElement;
class TiXmlNode;

class CJoystickMapper : public IButtonMapper
{
public:
  CJoystickMapper() = default;
  virtual ~CJoystickMapper();

  // implementation of IButtonMapper
  virtual void MapActions(int windowID, const TiXmlNode *pDevice) override;
  virtual void Clear() override;

  std::vector<std::shared_ptr<const IWindowKeymap>> GetJoystickKeymaps() const;

private:
  void DeserializeJoystickNode(const TiXmlNode* pDevice, std::string &controllerId);
  bool DeserializeButton(const TiXmlElement *pButton, std::string &feature, KODI::JOYSTICK::ANALOG_STICK_DIRECTION &dir, unsigned int& holdtimeMs, std::set<std::string>& hotkeys, std::string &actionStr);

  using ControllerID = std::string;
  std::map<ControllerID, std::shared_ptr<IWindowKeymap>> m_joystickKeymaps;

  std::vector<std::string> m_controllerIds;
};
