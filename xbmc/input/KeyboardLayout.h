#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

class TiXmlElement;

class CKeyboardLayout
{
public:
  CKeyboardLayout();
  virtual ~CKeyboardLayout();

  bool Load(const TiXmlElement* element);

  const std::string& GetName() const { return m_name; }

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

  std::string m_name;
  Keyboards m_keyboards;
};
