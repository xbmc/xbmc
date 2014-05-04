/*!
\file KeyboardLayout.h
\brief
*/

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

#include "system.h"
#include <string>
#include <map>
#include <vector>

/*!
 \ingroup keyboardlayouts
 \brief
 */

enum MODIFIER_KEYS
{
  MODIFIER_KEY_NONE  = 0x00,
  MODIFIER_KEY_SHIFT = 0x01,
  MODIFIER_KEY_SYMBOL = 0x02
};

class CKeyboardLayout
{
public:
  CKeyboardLayout(const std::string &name);
  virtual ~CKeyboardLayout(void);
  std::string GetName() const;
  void AddKeyboardRow(const std::wstring &strKeyboardRow, unsigned int iModifierKeys = 0);
  WCHAR GetCharAt(unsigned int iRow, unsigned int iColumn, unsigned int iModifierKeys = 0);
  void Clear();

protected:
  typedef std::vector<std::wstring> KeyboardRows;
  typedef std::map<unsigned int, KeyboardRows> Keyboards;

  std::string m_strName;
  Keyboards m_keyboards;
};

/*!
 \ingroup keyboardlayouts
 \brief
 */
