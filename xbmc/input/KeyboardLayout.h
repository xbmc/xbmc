#pragma once
/*
 *      Copyright (C) 2005-2013 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <string>
#include <vector>

#include "InputCodingTable.h"

class TiXmlElement;

class CKeyboardLayout
{
public:
  CKeyboardLayout();
  virtual ~CKeyboardLayout();
  IInputCodingTablePtr GetCodingTable() { return m_codingtable; }

  bool Load(const TiXmlElement* element);

  std::string GetIdentifier() const;
  std::string GetName() const;
  const std::string& GetLanguage() const { return m_language; }
  const std::string& GetLayout() const { return m_layout; }

  enum ModifierKey
  {
    ModifierKeyNone   = 0x00,
    ModifierKeyShift  = 0x01,
    ModifierKeySymbol = 0x02
  };

  std::string GetCharAt(unsigned int row, unsigned int column, unsigned int modifiers = 0) const;

private:
  static std::vector<std::string> BreakCharacters(const std::string &chars);

  typedef std::vector< std::vector<std::string> > KeyboardRows;
  typedef std::map<unsigned int, KeyboardRows> Keyboards;

  std::string m_language;
  std::string m_layout;
  Keyboards m_keyboards;
  IInputCodingTablePtr m_codingtable;
};
