/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/InputCodingTable.h"

#include <map>
#include <string>
#include <vector>

namespace tinyxml2
{
class XMLElement;
}

namespace KODI
{
namespace KEYBOARD
{
/*!
 * \ingroup keyboard
 */
class CKeyboardLayout
{
public:
  CKeyboardLayout() = default;
  virtual ~CKeyboardLayout();
  IInputCodingTablePtr GetCodingTable() { return m_codingtable; }

  bool Load(const tinyxml2::XMLElement* element);

  std::string GetIdentifier() const;
  std::string GetName() const;
  const std::string& GetLanguage() const { return m_language; }
  const std::string& GetLayout() const { return m_layout; }

  enum ModifierKey
  {
    ModifierKeyNone = 0x00,
    ModifierKeyShift = 0x01,
    ModifierKeySymbol = 0x02
  };

  std::string GetCharAt(unsigned int row, unsigned int column, unsigned int modifiers = 0) const;

private:
  static std::vector<std::string> BreakCharacters(const std::string& chars);

  typedef std::vector<std::vector<std::string>> KeyboardRows;
  typedef std::map<unsigned int, KeyboardRows> Keyboards;

  std::string m_language;
  std::string m_layout;
  Keyboards m_keyboards;
  IInputCodingTablePtr m_codingtable;
};
} // namespace KEYBOARD
} // namespace KODI
