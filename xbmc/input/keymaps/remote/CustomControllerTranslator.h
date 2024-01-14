/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keymaps/interfaces/IKeyMapper.h"

#include <map>
#include <string>

namespace tinyxml2
{
class XMLNode;
}

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 */
class CCustomControllerTranslator : public IKeyMapper
{
public:
  CCustomControllerTranslator() = default;

  // implementation of IKeyMapper
  void MapActions(int windowID, const tinyxml2::XMLNode* pDevice) override;
  void Clear() override;

  bool TranslateCustomControllerString(int windowId,
                                       const std::string& controllerName,
                                       int buttonId,
                                       int& action,
                                       std::string& strAction);

private:
  bool TranslateString(int windowId,
                       const std::string& controllerName,
                       int buttonId,
                       unsigned int& actionId,
                       std::string& strAction);

  // Maps button id to action
  using CustomControllerButtonMap = std::map<int, std::string>;

  // Maps window id to controller button map
  using CustomControllerWindowMap = std::map<int, CustomControllerButtonMap>;

  // Maps custom controller name to controller Window map
  std::map<std::string, CustomControllerWindowMap> m_customControllersMap;
};
} // namespace KEYMAP
} // namespace KODI
