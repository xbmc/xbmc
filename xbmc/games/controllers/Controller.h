/*
 *      Copyright (C) 2015-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "ControllerTypes.h"
#include "addons/Addon.h"
#include "input/joysticks/JoystickTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
class CControllerLayout;

using JOYSTICK::FEATURE_TYPE;

class CController : public ADDON::CAddon
{
public:
  static std::unique_ptr<CController> FromExtension(ADDON::CAddonInfo addonInfo, const cp_extension_t* ext);

  CController(ADDON::CAddonInfo addonInfo);

  virtual ~CController();

  static const ControllerPtr EmptyPtr;

  std::string Label(void);
  std::string ImagePath(void) const;
  void GetFeatures(std::vector<std::string>& features, FEATURE_TYPE type = FEATURE_TYPE::UNKNOWN) const;
  JOYSTICK::FEATURE_TYPE GetFeatureType(const std::string &feature) const;
  JOYSTICK::INPUT_TYPE GetInputType(const std::string& feature) const;

  bool LoadLayout(void);

  const CControllerLayout& Layout(void) const { return *m_layout; }

private:
  std::unique_ptr<CControllerLayout> m_layout;
  bool m_bLoaded = false;
};

}
}
