/*
 *      Copyright (C) 2017 Team Kodi
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

#include "IButtonMapper.h"

#include <map>
#include <string>

class TiXmlNode;

class CCustomControllerTranslator : public IButtonMapper
{
public:
  CCustomControllerTranslator() = default;

  // implementation of IButtonMapper
  virtual void MapActions(int windowID, const TiXmlNode *pDevice) override;
  virtual void Clear() override;

  bool TranslateCustomControllerString(int windowId, const std::string& controllerName, int buttonId, int& action, std::string& strAction);

private:
  bool TranslateString(int windowId, const std::string& controllerName, int buttonId, unsigned int& actionId, std::string& strAction);

  // Maps button id to action
  using CustomControllerButtonMap = std::map<int, std::string>;

  // Maps window id to controller button map
  using CustomControllerWindowMap = std::map<int, CustomControllerButtonMap>;

  // Maps custom controller name to controller Window map
  std::map<std::string, CustomControllerWindowMap> m_customControllersMap;
};
